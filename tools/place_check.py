#!/usr/bin/env python3
"""Finds things out of place in the bar (docs/AUTOMATION.md): every separate
piece of every model (a coat, a bottle, a frame: the models join pieces by
material, so they are split back into connected pieces here) is checked
against everything around it:

  floating   a piece that touches nothing at all (a coat off its hook), or a
             thing placed on its own (a stool, a glass) above what's under it
  sunk       a thing placed on its own whose bottom is inside what's under it

  python3 tools/place_check.py              the report (worst first)
  python3 tools/place_check.py --json OUT   also as JSON (for tools)
  python3 tools/place_check.py --min 0.03   only gaps of 3 cm or more (default 2 cm)

Reads assets/world/places/bar.json and kinds/bar.json for where models
stand, the .glb files for their shapes. Needs numpy.
"""
import json
import math
import struct
import sys
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).parent))
from albedo_audit import read_glb  # noqa: E402

MODELS = Path('assets/models')
WORLD = Path('assets/world')
COMP = {5120: 'b', 5121: 'B', 5122: 'h', 5123: 'H', 5125: 'I', 5126: 'f'}
COUNT = {'SCALAR': 1, 'VEC2': 2, 'VEC3': 3, 'VEC4': 4, 'MAT4': 16}


def accessor(gltf, binary, i):
    a = gltf['accessors'][i]
    v = gltf['bufferViews'][a['bufferView']]
    n, c = a['count'], COUNT[a['type']]
    fmt = COMP[a['componentType']]
    size = struct.calcsize(fmt)
    stride = v.get('byteStride', size * c)
    start = v.get('byteOffset', 0) + a.get('byteOffset', 0)
    if stride == size * c:
        out = np.frombuffer(binary, dtype=np.dtype(fmt), count=n * c, offset=start)
    else:
        out = np.stack([np.frombuffer(binary, dtype=np.dtype(fmt), count=c, offset=start + k * stride) for k in range(n)])
    return out.reshape(n, c) if c > 1 else out


def node_matrix(node):
    if 'matrix' in node:
        return np.array(node['matrix'], dtype=np.float64).reshape(4, 4).T
    t = node.get('translation', [0, 0, 0])
    r = node.get('rotation', [0, 0, 0, 1])
    s = node.get('scale', [1, 1, 1])
    x, y, z, w = r
    rot = np.array([[1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w)],
                    [2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w)],
                    [2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y)]])
    m = np.eye(4)
    m[:3, :3] = rot * np.array(s)
    m[:3, 3] = t
    return m


def model_pieces(file):
    """(name, triangles (n, 3, 3)) for each connected piece of a model, in its own space."""
    gltf, binary = read_glb(MODELS / file)
    out = []

    def visit(i, parent):
        node = gltf['nodes'][i]
        m = parent @ node_matrix(node)
        if 'mesh' in node:
            mesh = gltf['meshes'][node['mesh']]
            for p in mesh['primitives']:
                if p.get('mode', 4) != 4:
                    continue
                pos = accessor(gltf, binary, p['attributes']['POSITION']).astype(np.float64)
                idx = accessor(gltf, binary, p['indices']).astype(np.int64).reshape(-1, 3) if 'indices' in p \
                    else np.arange(len(pos)).reshape(-1, 3)
                mat = gltf['materials'][p['material']].get('name', '') if 'material' in p else ''
                world = pos @ m[:3, :3].T + m[:3, 3]
                for tri_idx in split(world, idx):
                    out.append((f"{node.get('name', mesh.get('name', '?'))} ({mat})", world[tri_idx]))
        for c in node.get('children', []):
            visit(c, m)

    scene = gltf['scenes'][gltf.get('scene', 0)]
    for n in scene['nodes']:
        visit(n, np.eye(4))
    return out


def split(pos, idx):
    """Connected pieces (corners at the same place count as joined)."""
    key = np.round(pos / 1e-4).astype(np.int64)
    _, vid = np.unique(key, axis=0, return_inverse=True)
    vid = vid.reshape(-1)
    t = vid[idx]
    # Every corner takes the smallest label of the triangles it's in, until
    # nothing changes: each connected piece ends up with one label.
    label = np.arange(vid.max() + 1)
    while True:
        m = label[t].min(axis=1)
        new = label.copy()
        np.minimum.at(new, t[:, 0], m)
        np.minimum.at(new, t[:, 1], m)
        np.minimum.at(new, t[:, 2], m)
        new = new[new]  # jump along chains
        if np.array_equal(new, label):
            break
        label = new
    roots = label[t[:, 0]]
    order = np.argsort(roots, kind='stable')
    cuts = np.flatnonzero(np.diff(roots[order])) + 1
    return [idx[g] for g in np.split(order, cuts)]


def thing_matrix(t, scale=1.0):
    a = math.radians(t.get('turn', 0))
    c, s = math.cos(a), math.sin(a)
    m = np.eye(4)
    m[:3, :3] = np.array([[c, 0, s], [0, 1, 0], [-s, 0, c]]) * t.get('scale', scale)
    m[:3, 3] = t.get('at', [0, 0, 0])
    return m


class Scene:
    def __init__(self):
        place = json.loads((WORLD / 'places/bar.json').read_text())
        kinds = {k['id']: k for k in json.loads((WORLD / 'kinds/bar.json').read_text())['kinds']}
        cache = {}
        self.pieces = []  # (thing id, name, tris)
        for t in place['things']:
            k = kinds.get(t['kind'], {})
            file = k.get('parts', {}).get('model', {}).get('file')
            if not file or not (MODELS / file).exists():
                continue
            if file not in cache:
                cache[file] = model_pieces(file)
            m = thing_matrix(t)
            for name, tris in cache[file]:
                w = tris @ m[:3, :3].T + m[:3, 3]
                self.pieces.append((t['id'], name, w))
        self.tris = np.concatenate([p[2] for p in self.pieces])
        self.owner = np.concatenate([np.full(len(p[2]), i) for i, p in enumerate(self.pieces)])
        self.lo = self.tris.min(axis=1)
        self.hi = self.tris.max(axis=1)
        # A grid over the floor (25 cm cells): which triangles touch each cell.
        self.cell = 0.25
        self.x0, self.z0 = self.lo[:, 0].min(), self.lo[:, 2].min()
        cx0 = ((self.lo[:, 0] - self.x0) / self.cell).astype(int)
        cx1 = ((self.hi[:, 0] - self.x0) / self.cell).astype(int)
        cz0 = ((self.lo[:, 2] - self.z0) / self.cell).astype(int)
        cz1 = ((self.hi[:, 2] - self.z0) / self.cell).astype(int)
        self.nz = int(cz1.max()) + 1
        keys, tris = [], []
        for k in range(len(self.tris)):
            for x in range(cx0[k], cx1[k] + 1):
                keys.append(np.arange(cz0[k], cz1[k] + 1) + x * self.nz)
                tris.append(np.full(cz1[k] - cz0[k] + 1, k))
        keys, tris = np.concatenate(keys), np.concatenate(tris)
        order = np.argsort(keys, kind='stable')
        self.keys, self.cellTris = keys[order], tris[order]

    def near(self, blo, bhi):
        """Triangles in the grid cells a box covers."""
        x0 = int((blo[0] - self.x0) // self.cell)
        x1 = int((bhi[0] - self.x0) // self.cell)
        z0 = max(int((blo[2] - self.z0) // self.cell), 0)
        z1 = min(int((bhi[2] - self.z0) // self.cell), self.nz - 1)
        out = []
        for x in range(max(x0, 0), x1 + 1):
            a = np.searchsorted(self.keys, x * self.nz + z0)
            b = np.searchsorted(self.keys, x * self.nz + z1, side='right')
            out.append(self.cellTris[a:b])
        return np.unique(np.concatenate(out)) if out else np.array([], dtype=int)

    def ray(self, o, d, skip, tmax):
        """The nearest hit of another piece: (t, piece) or (None, None)."""
        o = np.asarray(o, dtype=np.float64)
        d = np.asarray(d, dtype=np.float64)
        end = o + d * tmax
        blo, bhi = np.minimum(o, end) - 1e-6, np.maximum(o, end) + 1e-6
        cand = self.near(blo, bhi)
        if len(cand) == 0:
            return None, None
        mask = np.all(self.hi[cand] >= blo, axis=1) & np.all(self.lo[cand] <= bhi, axis=1) & (self.owner[cand] != skip)
        cand = cand[mask]
        if len(cand) == 0:
            return None, None
        tri = self.tris[cand]
        own = self.owner[cand]
        e1 = tri[:, 1] - tri[:, 0]
        e2 = tri[:, 2] - tri[:, 0]
        p = np.cross(d, e2)
        det = np.einsum('ij,ij->i', e1, p)
        ok = np.abs(det) > 1e-12
        inv = np.where(ok, 1.0 / np.where(ok, det, 1), 0)
        s = o - tri[:, 0]
        u = np.einsum('ij,ij->i', s, p) * inv
        q = np.cross(s, e1)
        v = (q @ d) * inv
        t = np.einsum('ij,ij->i', e2, q) * inv
        hit = ok & (u >= 0) & (v >= 0) & (u + v <= 1) & (t > 0) & (t <= tmax)
        if not hit.any():
            return None, None
        k = np.argmin(np.where(hit, t, np.inf))
        return float(t[k]), int(own[k])


def seg_dist(p, a, b):
    """Distance from points p (n, 3) to segments a-b (m, 3): (n, m)."""
    ab = b - a
    t = np.einsum('nmk,mk->nm', p[:, None, :] - a[None], ab) / np.maximum(np.einsum('mk,mk->m', ab, ab), 1e-12)
    t = np.clip(t, 0, 1)
    q = a[None] + t[..., None] * ab[None]
    return np.linalg.norm(p[:, None, :] - q, axis=2)


def point_tri_dist(p, tri):
    """Distance from points p (n, 3) to triangles (m, 3, 3): (n, m)."""
    a, b, c = tri[:, 0], tri[:, 1], tri[:, 2]
    n = np.cross(b - a, c - a)
    nn = np.linalg.norm(n, axis=1)
    n = n / np.maximum(nn, 1e-12)[:, None]
    d = np.einsum('nmk,mk->nm', p[:, None, :] - a[None], n)
    q = p[:, None, :] - d[..., None] * n[None]
    # Inside the triangle? (same side of all three edges)
    def side(u, v):
        return np.einsum('nmk,mk->nm', np.cross(v[None] - u[None], q - u[None]), n)
    inside = (side(a, b) >= 0) & (side(b, c) >= 0) & (side(c, a) >= 0) & (nn[None] > 1e-12)
    edge = np.minimum(np.minimum(seg_dist(p, a, b), seg_dist(p, b, c)), seg_dist(p, c, a))
    return np.where(inside, np.abs(d), edge)


def touch_gap(S, i, tris, reach):
    """How far piece i is from everything else (up to `reach`; None = nothing that near)."""
    pts = np.unique(tris.reshape(-1, 3), axis=0)
    if len(pts) > 64:
        pts = pts[np.linspace(0, len(pts) - 1, 64).astype(int)]
    lo, hi = pts.min(axis=0) - reach, pts.max(axis=0) + reach
    cand = S.near(lo, hi)
    if len(cand) == 0:
        return None
    cand = cand[np.all(S.hi[cand] >= lo, axis=1) & np.all(S.lo[cand] <= hi, axis=1) & (S.owner[cand] != i)]
    if len(cand) == 0:
        return None
    best = np.inf
    for k in range(0, len(cand), 4096):
        best = min(best, float(point_tri_dist(pts, S.tris[cand[k:k + 4096]]).min()))
    return best if best <= reach else None


def check(min_gap):
    S = Scene()
    found = []
    # 1. Pieces that touch nothing at all (a coat off its hook, a frame off its wall).
    for i, (thing, name, tris) in enumerate(S.pieces):
        lo, hi = tris.reshape(-1, 3).min(axis=0), tris.reshape(-1, 3).max(axis=0)
        if max(hi - lo) > 4 or 'glow' in name.lower() or 'flame' in name.lower():
            continue
        g = touch_gap(S, i, tris, 0.5)
        if g is None or g > min_gap:
            c = (lo + hi) / 2
            found.append((g or 0.5, thing, name, 'floating',
                          f"touches nothing ({'over 50' if g is None else f'{g * 100:.0f}'} cm from the nearest thing)", c))
    # 2. Things placed on their own (stools, tables, glasses, candles): their
    #    bottom against what's under them.
    things = {}
    for i, (thing, name, tris) in enumerate(S.pieces):
        things.setdefault(thing, []).append(i)
    for thing, ids in things.items():
        allt = np.concatenate([S.pieces[i][2] for i in ids]).reshape(-1, 3)
        lo, hi = allt.min(axis=0), allt.max(axis=0)
        if max(hi - lo) > 3:
            continue
        own = set(ids)
        under = []
        for fx in (0.3, 0.5, 0.7):
            for fz in (0.3, 0.5, 0.7):
                o = np.array([lo[0] + (hi[0] - lo[0]) * fx, hi[1] + 0.01, lo[2] + (hi[2] - lo[2]) * fz])
                # Down through the thing itself to the first other surface.
                cand = S.near(o - [0, 3, 0], o)
                cand = cand[[S.owner[k] not in own for k in cand]] if len(cand) else cand
                t = ray_hit(S, o, cand)
                if t is not None:
                    under.append(o[1] - t)
        if not under:
            continue
        top = max(under)
        gap = lo[1] - top
        if abs(gap) > min_gap and lo[1] > -0.01:
            c = (lo + hi) / 2
            found.append((abs(gap), thing, '(the whole thing)', 'floating' if gap > 0 else 'sunk',
                          f"{abs(gap) * 100:.0f} cm {'above' if gap > 0 else 'into'} what's under it (its bottom {lo[1]:.3f} m, the surface {top:.3f} m)", c))
    found.sort(key=lambda f: -f[0])
    return S, found


def ray_hit(S, o, cand):
    """Distance straight down from o to the nearest of the triangles `cand`."""
    if len(cand) == 0:
        return None
    tri = S.tris[cand]
    d = np.array([0.0, -1.0, 0.0])
    e1, e2 = tri[:, 1] - tri[:, 0], tri[:, 2] - tri[:, 0]
    p = np.cross(d, e2)
    det = np.einsum('ij,ij->i', e1, p)
    ok = np.abs(det) > 1e-12
    inv = np.where(ok, 1.0 / np.where(ok, det, 1), 0)
    s = o - tri[:, 0]
    u = np.einsum('ij,ij->i', s, p) * inv
    q = np.cross(s, e1)
    v = (q @ d) * inv
    t = np.einsum('ij,ij->i', e2, q) * inv
    hit = ok & (u >= 0) & (v >= 0) & (u + v <= 1) & (t > 0)
    return float(t[hit].min()) if hit.any() else None


if __name__ == '__main__':
    mg = float(sys.argv[sys.argv.index('--min') + 1]) if '--min' in sys.argv else 0.02
    S, found = check(mg)
    print(f"{len(S.pieces)} pieces checked; {len(found)} out of place")
    for amount, thing, name, what, why, c in found:
        print(f"{what:9} {thing:18} {name[:40]:40} {why}  at [{c[0]:.2f}, {c[1]:.2f}, {c[2]:.2f}]")
    if '--json' in sys.argv:
        out = sys.argv[sys.argv.index('--json') + 1]
        Path(out).write_text(json.dumps([{'thing': t, 'piece': n, 'what': w, 'why': y, 'at': [round(float(v), 3) for v in c],
                                          'amount': round(a, 3)} for a, t, n, w, y, c in found], indent=1))
