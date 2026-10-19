#!/usr/bin/env python3
"""Makes the phone version of the models: every image becomes a GPU-ready
ASTC texture (4x smaller in GPU memory than plain RGBA, nothing to decode on
the phone), with all its mipmaps made here. Run by the build (CMakeLists.txt):

  python3 tools/assets/astc_pack.py <astcenc> <cache dir> <site assets dir> <model.glb>...

For each model it writes <site>/models/<name>.astc.glb.gz, where each image's
bytes are only a reference ("BA3R" + a 16-letter key, mimeType
"image/x-bar3d-astc"); the texture itself is <site>/tex/<key>.ba3c.gz, shared
by every model that uses the same image, so it downloads once. The key is a
hash of the source image and how it's used, so unchanged images are never
re-encoded (the cache dir keeps them between builds).

Images are scaled the way the game scales PNG/JPEG ones (engine/model.cpp
afterParse: colors and normal maps up to 1024, roughness/occlusion maps up
to 256). Color images (base color, emissive) are encoded and mipmapped as
sRGB, the rest as linear data. A .ba3c file (read by engine/gpu_upload.cpp):

  "BA3C", u32 width, height, mip count, block width, block height (4 or 8:
  8x8 when the image still measures near-identical, MIN_PSNR),
  then per mip: u32 byte count + the ASTC blocks (no .astc header).

astcenc is ARM's encoder (Apache 2.0), a build tool only (tools/setup.sh).
"""
import gzip
import hashlib
import io
import json
import os
import struct
import subprocess
import sys
import tempfile
from concurrent.futures import ThreadPoolExecutor

import numpy as np
from PIL import Image

# Block sizes that divide power-of-two textures: 8x8 (2 bits per pixel) when
# the image still looks the same, else 4x4 (8 bits per pixel).
FINE, COARSE = 4, 8
MIN_PSNR = 40.0  # dB, measured on the full-size level: near visually lossless
MIME = 'image/x-bar3d-astc'
VERSION = b'2'  # bump to re-encode everything


def read_glb(path):
    data = open(path, 'rb').read()
    magic, version, length = struct.unpack_from('<III', data, 0)
    assert magic == 0x46546C67, 'not a glb'
    off = 12
    js, binchunk = None, b''
    while off < length:
        clen, ctype = struct.unpack_from('<II', data, off)
        chunk = data[off + 8:off + 8 + clen]
        if ctype == 0x4E4F534A:
            js = json.loads(chunk.decode('utf-8'))
        elif ctype == 0x004E4942:
            binchunk = chunk
        off += 8 + clen
    return js, binchunk


def write_glb(path, js, binchunk):
    jbytes = json.dumps(js, separators=(',', ':')).encode('utf-8')
    jbytes += b' ' * ((4 - len(jbytes) % 4) % 4)
    binchunk += b'\0' * ((4 - len(binchunk) % 4) % 4)
    total = 12 + 8 + len(jbytes) + (8 + len(binchunk) if binchunk else 0)
    with open(path, 'wb') as f:
        f.write(struct.pack('<III', 0x46546C67, 2, total))
        f.write(struct.pack('<II', len(jbytes), 0x4E4F534A))
        f.write(jbytes)
        if binchunk:
            f.write(struct.pack('<II', len(binchunk), 0x004E4942))
            f.write(binchunk)


def srgb_to_linear(c):
    return np.where(c <= 0.04045, c / 12.92, ((c + 0.055) / 1.055) ** 2.4)


def linear_to_srgb(c):
    c = np.clip(c, 0, 1)
    return np.where(c <= 0.0031308, c * 12.92, 1.055 * np.power(c, 1 / 2.4) - 0.055)


def half(a):
    """Halves an HxWx4 float image (box filter; odd sides keep their last row)."""
    h, w = a.shape[:2]
    nh, nw = max(1, h // 2), max(1, w // 2)
    if h > 1:
        a = (a[0:nh * 2:2] + a[1:nh * 2:2]) * 0.5
    if w > 1:
        a = (a[:, 0:nw * 2:2] + a[:, 1:nw * 2:2]) * 0.5
    return a


def mip_chain(img, srgb):
    """Level 0 down to 1x1 as uint8 RGBA arrays, averaged in linear light for colors."""
    a = np.asarray(img, dtype=np.float64) / 255.0
    if srgb:
        a = np.concatenate([srgb_to_linear(a[..., :3]), a[..., 3:]], axis=-1)
    levels = []
    while True:
        out = a
        if srgb:
            out = np.concatenate([linear_to_srgb(a[..., :3]), a[..., 3:]], axis=-1)
        levels.append(np.clip(np.round(out * 255), 0, 255).astype(np.uint8))
        if a.shape[0] == 1 and a.shape[1] == 1:
            break
        a = half(a)
    return levels


def encode(astcenc, rgba, srgb, block, tmp, tag):
    src = os.path.join(tmp, f'{tag}.png')
    dst = os.path.join(tmp, f'{tag}.astc')
    Image.fromarray(rgba, 'RGBA').save(src)
    subprocess.run([astcenc, '-cs' if srgb else '-cl', src, dst, f'{block}x{block}', '-medium', '-silent'],
                   check=True, stdout=subprocess.DEVNULL)
    data = open(dst, 'rb').read()[16:]  # skip the .astc header
    h, w = rgba.shape[:2]
    expect = ((w + block - 1) // block) * ((h + block - 1) // block) * 16
    assert len(data) == expect, (tag, len(data), expect)
    return data


def psnr(astcenc, rgba, srgb, block, tmp, tag):
    """How close an encode-then-decode comes to the original (dB, RGB)."""
    src = os.path.join(tmp, f'{tag}_t.png')
    dst = os.path.join(tmp, f'{tag}_t_out.png')
    Image.fromarray(rgba, 'RGBA').save(src)
    subprocess.run([astcenc, '-ts' if srgb else '-tl', src, dst, f'{block}x{block}', '-medium', '-silent'],
                   check=True, stdout=subprocess.DEVNULL)
    out = np.asarray(Image.open(dst).convert('RGBA'), dtype=np.float64)
    mse = ((out[..., :3] - rgba[..., :3].astype(np.float64)) ** 2).mean()
    return 10 * np.log10(255.0 ** 2 / max(mse, 1e-9))


def pack_image(astcenc, raw, srgb, cap, tmp, tag):
    img = Image.open(io.BytesIO(raw)).convert('RGBA')
    w, h = img.size
    big = max(w, h)
    if big > cap:  # like createImageBitmap's resize in engine/gpu_upload.cpp
        s = cap / big
        w, h = max(1, round(w * s)), max(1, round(h * s))
    # The GPU wants whole blocks at full size: sizes that are multiples of 8
    # can use either block size.
    w, h = max(COARSE, (w + COARSE - 1) // COARSE * COARSE), max(COARSE, (h + COARSE - 1) // COARSE * COARSE)
    if (w, h) != img.size:
        img = img.resize((w, h), Image.LANCZOS)
    levels = mip_chain(img, srgb)
    block = COARSE if psnr(astcenc, levels[0], srgb, COARSE, tmp, tag) >= MIN_PSNR else FINE
    out = bytearray(b'BA3C' + struct.pack('<5I', w, h, len(levels), block, block))
    for i, lv in enumerate(levels):
        blocks = encode(astcenc, lv, srgb, block, tmp, f'{tag}_{i}')
        out += struct.pack('<I', len(blocks)) + blocks
    return bytes(out)


def gz_write(path, data):
    with open(path, 'wb') as f, gzip.GzipFile(fileobj=f, mode='wb', compresslevel=9, mtime=0, filename='') as g:
        g.write(data)


def image_uses(js):
    """image index -> (as sRGB color?, largest size kept)"""
    textures = js.get('textures', [])
    use = {}

    def mark(tex_info, srgb, cap):
        if not tex_info:
            return
        src_img = textures[tex_info['index']].get('source')
        if src_img is None:
            return
        s, c = use.get(src_img, (False, 0))
        use[src_img] = (s or srgb, max(c, cap))

    for m in js.get('materials', []):
        pbr = m.get('pbrMetallicRoughness', {})
        mark(pbr.get('baseColorTexture'), True, 1024)
        mark(m.get('emissiveTexture'), True, 1024)
        mark(pbr.get('metallicRoughnessTexture'), False, 256)
        mark(m.get('normalTexture'), False, 1024)
        mark(m.get('occlusionTexture'), False, 256)
    return use


def main():
    astcenc, cache, site = sys.argv[1:4]
    models = sys.argv[4:]
    os.makedirs(cache, exist_ok=True)
    os.makedirs(os.path.join(site, 'models'), exist_ok=True)
    os.makedirs(os.path.join(site, 'tex'), exist_ok=True)
    parsed = []
    todo = {}  # key -> (raw, srgb, cap)
    for path in models:
        js, binchunk = read_glb(path)
        refs = {}
        views = js.get('bufferViews', [])
        for i, (srgb, cap) in image_uses(js).items():
            im = js['images'][i]
            if 'bufferView' not in im:
                continue
            v = views[im['bufferView']]
            raw = binchunk[v.get('byteOffset', 0):v.get('byteOffset', 0) + v['byteLength']]
            key = hashlib.sha1(VERSION + bytes([srgb]) + struct.pack('<I', cap) + raw).hexdigest()[:16]
            refs[i] = key
            if not os.path.exists(os.path.join(cache, key + '.ba3c')):
                todo[key] = (raw, srgb, cap)
        parsed.append((path, js, binchunk, refs))

    with tempfile.TemporaryDirectory() as tmp, ThreadPoolExecutor(os.cpu_count() or 4) as pool:
        jobs = {k: pool.submit(pack_image, astcenc, *v, tmp, k) for k, v in todo.items()}
        for k, job in jobs.items():
            with open(os.path.join(cache, k + '.ba3c.tmp'), 'wb') as f:
                f.write(job.result())
            os.replace(os.path.join(cache, k + '.ba3c.tmp'), os.path.join(cache, k + '.ba3c'))

    written = set()
    for path, js, binchunk, refs in parsed:
        views = js.get('bufferViews', [])
        replaced = {}
        for i, key in refs.items():
            replaced[js['images'][i]['bufferView']] = b'BA3R' + key.encode('ascii')
            js['images'][i]['mimeType'] = MIME
            if key not in written:
                written.add(key)
                with open(os.path.join(cache, key + '.ba3c'), 'rb') as f:
                    gz_write(os.path.join(site, 'tex', key + '.ba3c.gz'), f.read())
        # Rebuild the binary chunk with the references in place of the images.
        out = bytearray()
        for vi, v in enumerate(views):
            start = v.get('byteOffset', 0)
            data = replaced.get(vi, binchunk[start:start + v['byteLength']])
            out += b'\0' * ((4 - len(out) % 4) % 4)
            v['byteOffset'] = len(out)
            v['byteLength'] = len(data)
            out += data
        if js.get('buffers'):
            js['buffers'][0]['byteLength'] = len(out)
        name = os.path.basename(path)[:-4]
        tmp_glb = os.path.join(cache, name + '.astc.glb')
        write_glb(tmp_glb, js, bytes(out))
        with open(tmp_glb, 'rb') as f:
            gz_write(os.path.join(site, 'models', name + '.astc.glb.gz'), f.read())
    print(f'astc_pack: {len(models)} models, {len(written)} textures ({len(todo)} newly encoded)')


if __name__ == '__main__':
    main()
