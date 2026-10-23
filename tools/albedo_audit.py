#!/usr/bin/env python3
"""Real material brightness (L1.4, docs/manual/light.md) and roughness (M0):
measures how much
light every material of every model reflects (its albedo: the average of its
base colour texture in linear light, times its colour factor) and how much
it glows, and writes assets/models/albedo.json. The engine's bounced light
(L1.6) reads it (the GPU's textures can't be read back cheaply), and
tools/albedo_test.mjs checks it covers every model and stays within real
values. Needs Pillow (python3 -m pip install pillow); run after changing a
model:

  python3 tools/albedo_audit.py            write assets/models/albedo.json, print the report
  python3 tools/albedo_audit.py --report   only print

Real albedo (Rec. 709 luminance of the linear colour): fresh snow 0.8-0.9,
white plaster/paint 0.7-0.8, light wood 0.4-0.5, brick 0.25-0.35, dark oak
0.1-0.15, black leather 0.04-0.06, charcoal 0.03. The ranges per material
name are in assets/models/albedo_rules.json; a material outside its range
gets a "fix" (a multiplier on its colour, new lighting only).
"""
import io
import json
import struct
import sys
from pathlib import Path

from PIL import Image

MODELS = Path('assets/models')
OUT = MODELS / 'albedo.json'
RULES = json.loads((MODELS / 'albedo_rules.json').read_text())['rules']
ROUGH_RULES = json.loads((MODELS / 'rough_rules.json').read_text())['rules']


def rule_for(name):
    low = name.lower()
    for word, lo, hi, _why in RULES:
        if word in low:
            return word, lo, hi
    return '', 0.0, 1.0


def rough_for(name, metal):
    """The least roughness a material may have (assets/models/rough_rules.json)."""
    low = name.lower()
    for word, least, _why in ROUGH_RULES:
        if word in low:
            # Metals no rule names: polished, but not mirrors.
            return (0.3 if metal >= 0.5 else least) if word == '' else least
    return 0.0


def srgb_to_linear(c):
    c = c / 255.0
    return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4


LUT = [srgb_to_linear(i) for i in range(256)]


def read_glb(path):
    data = path.read_bytes()
    magic, _version, _length = struct.unpack_from('<III', data, 0)
    assert magic == 0x46546C67, f'{path}: not a glb'
    pos, gltf, binary = 12, None, b''
    while pos < len(data):
        clen, ctype = struct.unpack_from('<II', data, pos)
        chunk = data[pos + 8:pos + 8 + clen]
        if ctype == 0x4E4F534A:
            gltf = json.loads(chunk)
        elif ctype == 0x004E4942:
            binary = chunk
        pos += 8 + clen
    return gltf, binary


def image_bytes(gltf, binary, index):
    img = gltf['images'][index]
    view = gltf['bufferViews'][img['bufferView']]
    start = view.get('byteOffset', 0)
    return binary[start:start + view['byteLength']]


def mean_channel(gltf, binary, tex_index, channel, cache):
    """The average of one channel (0..1, as stored) of a data texture."""
    if tex_index is None:
        return 1.0
    src = gltf['textures'][tex_index]['source']
    key = (src, channel)
    if key not in cache:
        im = Image.open(io.BytesIO(image_bytes(gltf, binary, src))).convert('RGB')
        im.thumbnail((64, 64))
        px = list(im.get_flattened_data()) if hasattr(im, 'get_flattened_data') else list(im.getdata())
        cache[key] = sum(p[channel] for p in px) / (255.0 * max(len(px), 1))
    return cache[key]


def mean_linear(gltf, binary, tex_index, cache):
    """The average colour of a texture in linear light (alpha-weighted)."""
    if tex_index is None:
        return (1.0, 1.0, 1.0)
    src = gltf['textures'][tex_index]['source']
    if src in cache:
        return cache[src]
    im = Image.open(io.BytesIO(image_bytes(gltf, binary, src))).convert('RGBA')
    im.thumbnail((128, 128))
    px = list(im.get_flattened_data()) if hasattr(im, 'get_flattened_data') else list(im.getdata())
    tot = [0.0, 0.0, 0.0]
    wsum = 0.0
    for r, g, b, a in px:
        w = a / 255.0
        tot[0] += LUT[r] * w
        tot[1] += LUT[g] * w
        tot[2] += LUT[b] * w
        wsum += w
    m = tuple(t / max(wsum, 1e-6) for t in tot)
    cache[src] = m
    return m


def lum(c):
    return 0.2126 * c[0] + 0.7152 * c[1] + 0.0722 * c[2]


def audit():
    result, report = {}, []
    for path in sorted(MODELS.glob('*.glb')):
        if '.astc.' in path.name:
            continue
        gltf, binary = read_glb(path)
        cache = {}
        mats = {}
        for i, m in enumerate(gltf.get('materials', [])):
            pbr = m.get('pbrMetallicRoughness', {})
            factor = pbr.get('baseColorFactor', [1, 1, 1, 1])
            tex = pbr.get('baseColorTexture', {}).get('index')
            t = mean_linear(gltf, binary, tex, cache)
            albedo = [round(t[k] * factor[k], 4) for k in range(3)]
            metal = pbr.get('metallicFactor', 1.0) * mean_channel(
                gltf, binary, pbr.get('metallicRoughnessTexture', {}).get('index'), 2, cache)
            em = m.get('emissiveFactor', [0, 0, 0])
            strength = m.get('extensions', {}).get('KHR_materials_emissive_strength', {}).get('emissiveStrength', 1.0)
            emissive = [0.0, 0.0, 0.0]
            if any(em):
                et = mean_linear(gltf, binary, m.get('emissiveTexture', {}).get('index'), cache)
                emissive = [round(em[k] * et[k] * strength, 4) for k in range(3)]
            name = m.get('name', f'material{i}')
            entry = {'albedo': albedo, 'metal': round(metal, 3)}
            # Roughness (M0): as made (factor x the texture's average) and the
            # least a real one has; the new lighting draws it no smoother.
            rough = pbr.get('roughnessFactor', 1.0) * mean_channel(
                gltf, binary, pbr.get('metallicRoughnessTexture', {}).get('index'), 1, cache)
            least = rough_for(name, metal)
            if least > rough + 0.02:
                entry['rough'] = round(least, 2)
            if any(emissive):
                entry['emissive'] = emissive
            if m.get('alphaMode') == 'BLEND':
                entry['seeThrough'] = True
            mats[name] = entry
            y = lum(albedo)
            flag = ''
            fixable = metal < 0.5 and not entry.get('seeThrough') and not any(emissive)
            if fixable and y > 0:
                word, lo, hi = rule_for(name)
                target = min(max(y, lo), hi)
                if abs(target - y) > 1e-4:
                    entry['fix'] = round(target / y, 3)
                    flag = f"{y:.3f} → {target:.3f} ({word or 'any'} {lo}-{hi})"
            report.append(f"{'fix' if flag else '   '} {path.name:24} {name[:28]:28} albedo {y:.3f}"
                          f"{' metal' if metal >= 0.5 else ''}{' glows' if any(emissive) else ''}"
                          f"{' ' + flag if flag else ''}"
                          f"{f' rough {rough:.2f} → {least:.2f}' if 'rough' in entry else ''}")
        result[path.name] = mats
    return result, report


if __name__ == '__main__':
    data, report = audit()
    print('\n'.join(report))
    if '--report' not in sys.argv:
        OUT.write_text(json.dumps(data, indent=1, sort_keys=True) + '\n')
        print(f'wrote {OUT} ({sum(len(v) for v in data.values())} materials in {len(data)} models)')
