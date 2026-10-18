# Pack Poly Haven CC0 surface textures into a JS module of WebP data URIs.
# Albedo maps are normalised to a neutral mean (detail maps) so the game's biome palettes still drive colour.
from PIL import Image
import numpy as np, base64, io
SRC = '/tmp/ph/tex'
sets = {
  'asphalt': 'asphalt_02', 'gravel': 'gravelly_sand',
  'mojave': 'dry_ground_rocks', 'sierra': 'forest_ground_04', 'glacier': 'snow_02', 'coast': 'leafy_grass',
}
def uri(im, q=80):
    b = io.BytesIO(); im.save(b, 'WEBP', quality=q, method=6); return 'data:image/webp;base64,' + base64.b64encode(b.getvalue()).decode(), len(b.getvalue())
out = []; tot = 0
for key, n in sets.items():
    d = np.array(Image.open(f'{SRC}/{n}_Diffuse.jpg').convert('RGB').resize((512, 512), Image.LANCZOS)).astype(np.float32)
    mean = d.reshape(-1, 3).mean(0)
    det = np.clip(d / mean * 127.5, 0, 255).astype(np.uint8)
    u, s = uri(Image.fromarray(det)); out.append(f'export const {key.upper()}_DET = "{u}";'); tot += s
    nm = Image.open(f'{SRC}/{n}_nor_gl.jpg').convert('RGB').resize((512, 512), Image.LANCZOS)
    u, s = uri(nm, 85); out.append(f'export const {key.upper()}_NOR = "{u}";'); tot += s
    r = Image.open(f'{SRC}/{n}_Rough.jpg').convert('L').resize((256, 256), Image.LANCZOS)
    u, s = uri(r, 75); out.append(f'export const {key.upper()}_RGH = "{u}";'); tot += s
open('/home/user/workspace/meridian/textures.js', 'w').write('// Poly Haven CC0 textures (polyhaven.com), packed as WebP\n' + '\n'.join(out) + '\n')
print('bytes', tot)
