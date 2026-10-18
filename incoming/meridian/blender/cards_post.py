# Post-process rendered foliage cards: colour dilation into transparent texels (prevents dark fringes with mipmaps),
# and pack the scanned grass atlas into RGBA.
from PIL import Image, ImageFilter
import numpy as np, json
def dilate(rgb, alpha, iters=24):
    rgb = rgb.astype(np.float32); m = (alpha > 0.5).astype(np.float32)
    acc = rgb * m[..., None]; w = m.copy()
    out = acc.copy(); filled = m.copy()
    for _ in range(iters):
        k = np.zeros_like(acc); kw = np.zeros_like(w)
        for dy, dx in ((0, 1), (0, -1), (1, 0), (-1, 0), (1, 1), (-1, -1), (1, -1), (-1, 1)):
            k += np.roll(np.roll(out * filled[..., None], dy, 0), dx, 1); kw += np.roll(np.roll(filled, dy, 0), dx, 1)
        new = (filled == 0) & (kw > 0)
        out[new] = k[new] / kw[new][..., None]; filled[new] = 1
    out[filled == 0] = out[filled > 0].mean(0)
    return out.clip(0, 255).astype(np.uint8)
for n in ('fir', 'leaf'):
    a = np.array(Image.open(f'/tmp/cards/{n}_albedo.png').convert('RGBA'))
    al = a[..., 3].astype(np.float32) / 255
    rgb = dilate(a[..., :3], al)
    # crisp-ish alpha for alpha testing
    alpha = (np.clip((al - 0.25) * 2.0, 0, 1) * 255).astype(np.uint8)
    Image.fromarray(np.dstack([rgb, alpha])).save(f'/tmp/cards/{n}_albedo_d.png')
    nm = np.array(Image.open(f'/tmp/cards/{n}_normal.png').convert('RGBA'))
    nrgb = nm[..., :3].copy(); nrgb[al < 0.05] = (128, 128, 255)
    nrgb = dilate(nrgb, al)
    Image.fromarray(nrgb).save(f'/tmp/cards/{n}_normal_d.png')
d = np.array(Image.open('/tmp/ph/mdl/grass_medium_01/textures/grass_medium_01_diff_1k.jpg').convert('RGB'))
al = np.array(Image.open('/tmp/ph/mdl/grass_medium_01/textures/grass_medium_01_alpha_1k.png').convert('L')).astype(np.float32) / 255
rgb = dilate(d, al, 8)
Image.fromarray(np.dstack([rgb, (al * 255).astype(np.uint8)])).save('/tmp/cards/grass_rgba.png')
c = json.load(open('/tmp/ph/crops.json'))
c['grass_card'] = [[190, 772, 440, 895], [585, 772, 815, 885], [590, 885, 809, 1015]]
json.dump(c, open('/tmp/ph/crops.json', 'w'))
# preview sheet
sheet = Image.new('RGB', (1024, 1024), (90, 110, 140))
for i, n in enumerate(('fir', 'leaf')):
    im = Image.open(f'/tmp/cards/{n}_albedo_d.png'); sheet.paste(im.resize((512, 256)), (0, i * 256), im.resize((512, 256)))
    sheet.paste(Image.open(f'/tmp/cards/{n}_normal_d.png').resize((512, 256)), (512, i * 256))
g = Image.open('/tmp/cards/grass_rgba.png').resize((512, 512)); sheet.paste(g, (0, 512), g)
sheet.save('/tmp/cards/sheet.jpg')
print('ok')
