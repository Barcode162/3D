# Meridian GT — procedural coupe modelled in Blender (front = +Y, right = +X, up = +Z)
import bpy, bmesh, math, sys
from mathutils import Vector, Matrix

OUT = sys.argv[sys.argv.index('--') + 1] if '--' in sys.argv else '/tmp/car'

def clear():
    bpy.ops.object.select_all(action='SELECT'); bpy.ops.object.delete()
    for c in (bpy.data.meshes, bpy.data.materials, bpy.data.curves):
        for b in list(c): c.remove(b)
clear()

def lerp(a, b, t): return a + (b - a) * t
def sstep(a, b, x):
    t = max(0.0, min(1.0, (x - a) / (b - a))); return t * t * (3 - 2 * t)
def interp(tbl, y):
    # tbl sorted by y ascending: [(y, v)], smooth (cosine) interpolation
    if y <= tbl[0][0]: return tbl[0][1]
    if y >= tbl[-1][0]: return tbl[-1][1]
    for (y0, v0), (y1, v1) in zip(tbl, tbl[1:]):
        if y0 <= y <= y1:
            t = (y - y0) / (y1 - y0); t = 0.5 - 0.5 * math.cos(math.pi * t)
            return lerp(v0, v1, t)

# ------------------------------------------------------------ materials
def mat(name, color, metal=0.0, rough=0.5, emit=None, emit_s=0.0, coat=0.0, alpha=1.0, trans=0.0):
    m = bpy.data.materials.new(name); m.use_nodes = True
    b = m.node_tree.nodes['Principled BSDF']
    b.inputs['Base Color'].default_value = (*color, 1)
    b.inputs['Metallic'].default_value = metal
    b.inputs['Roughness'].default_value = rough
    b.inputs['Coat Weight'].default_value = coat
    b.inputs['Coat Roughness'].default_value = 0.05
    if emit:
        b.inputs['Emission Color'].default_value = (*emit, 1); b.inputs['Emission Strength'].default_value = emit_s
    if alpha < 1:
        b.inputs['Alpha'].default_value = alpha
        m.blend_method = 'BLEND'
    return m
M = {
    'Paint': mat('Paint', (0.80, 0.77, 0.72), metal=0.35, rough=0.28, coat=1.0),
    'Glass': mat('Glass', (0.02, 0.025, 0.035), metal=0.2, rough=0.04, alpha=0.82),
    'Trim': mat('Trim', (0.018, 0.018, 0.02), rough=0.55),
    'Gap': mat('Gap', (0.005, 0.005, 0.005), rough=0.9),
    'Chrome': mat('Chrome', (0.9, 0.9, 0.92), metal=1.0, rough=0.12),
    'Head': mat('Head', (0.9, 0.9, 0.88), rough=0.2, emit=(1.0, 0.95, 0.86), emit_s=6.0),
    'HeadHousing': mat('HeadHousing', (0.06, 0.06, 0.07), metal=0.8, rough=0.2),
    'Tail': mat('Tail', (0.25, 0.0, 0.01), rough=0.25, emit=(1.0, 0.06, 0.1), emit_s=4.0),
    'TailLens': mat('TailLens', (0.12, 0.0, 0.01), rough=0.1),
    'Interior': mat('Interior', (0.035, 0.032, 0.03), rough=0.8),
    'Leather': mat('Leather', (0.12, 0.07, 0.045), rough=0.55),
    'Rubber': mat('Rubber', (0.025, 0.025, 0.025), rough=0.92),
    'Rim': mat('Rim', (0.55, 0.56, 0.58), metal=1.0, rough=0.28),
    'RimDark': mat('RimDark', (0.08, 0.08, 0.085), metal=0.9, rough=0.35),
    'Disc': mat('Disc', (0.32, 0.31, 0.3), metal=1.0, rough=0.45),
    'Caliper': mat('Caliper', (0.85, 0.42, 0.08), metal=0.2, rough=0.35, coat=0.6),
    'Plate': mat('Plate', (0.85, 0.83, 0.78), rough=0.4),
}
def add_mats(obj, names):
    for n in names: obj.data.materials.append(M[n])
    return {n: i for i, n in enumerate(names)}

# ------------------------------------------------------------ body profile tables (y: rear -2.24 .. front 2.24)
Y0, Y1 = -2.24, 2.24
ZB = [(-2.24, 0.86), (-2.12, 0.95), (-1.9, 0.975), (-1.5, 0.965), (-0.6, 0.93), (0.3, 0.905), (0.85, 0.86), (1.4, 0.76), (1.85, 0.68), (2.12, 0.6), (2.24, 0.52)]
ROOF = [(-2.24, 0.0), (-2.05, 0.0), (-1.80, 0.02), (-1.45, 0.17), (-1.0, 0.31), (-0.6, 0.37), (-0.15, 0.385), (0.15, 0.37), (0.5, 0.24), (0.85, 0.035), (1.0, 0.0), (2.24, 0.0)]
Z0 = [(-2.24, 0.34), (-2.12, 0.27), (-1.95, 0.2), (-1.7, 0.17), (1.7, 0.16), (2.0, 0.19), (2.15, 0.25), (2.24, 0.32)]
ZS = [(-2.24, 0.62), (-2.0, 0.64), (-1.4, 0.64), (0.0, 0.6), (1.4, 0.57), (2.0, 0.5), (2.24, 0.45)]
WR = [(-2.24, 0.5), (-1.8, 0.66), (-1.3, 0.6), (-0.4, 0.56), (0.4, 0.58), (0.85, 0.72), (1.2, 0.68), (2.24, 0.55)]

def width(y):
    w = 0.885
    if y > 1.45: w -= 0.12 * ((y - 1.45) / 0.79) ** 2
    if y < -1.65: w -= 0.07 * ((-1.65 - y) / 0.59) ** 2
    w += 0.045 * math.exp(-((y + 1.36) / 0.5) ** 2) + 0.022 * math.exp(-((y - 1.24) / 0.42) ** 2)
    return w

def ring(y, endf=1.0):
    w = width(y) * endf
    z0 = interp(Z0, y); zs = interp(ZS, y); zb = interp(ZB, y)
    cab = interp(ROOF, y)
    crown = 0.045
    zt = zb + max(crown, cab + 0.02) if cab > 0.0 else zb + crown
    wr = interp(WR, y) * endf
    # shrink vertically at the very ends so caps close cleanly
    if endf < 1.0:
        mid = (z0 + zt) * 0.5; k = 0.35 + 0.65 * endf
        z0 = lerp(mid, z0, k); zt = lerp(mid, zt, k); zs = lerp(mid, zs, k); zb = lerp(mid, zb, k)
    pts = []
    # bottom
    pts += [(0.0, z0), (0.35 * w, z0), (0.7 * w, z0), (0.9 * w, z0 + 0.012)]
    # lower rounding to shoulder
    pts += [(0.975 * w, z0 + 0.06), (0.998 * w, lerp(z0, zs, 0.55)), (w, zs)]
    # shoulder to belt (slight tuck)
    pts += [(0.985 * w, lerp(zs, zb, 0.5)), (0.95 * w, zb - 0.018), (0.915 * w, zb)]
    # greenhouse / hood crown (6 pts)
    cabin = cab > 0.06
    wd = (0.84 if cabin else 0.9) * w
    gh = zt - zb
    for k in range(1, 7):
        s = k / 6.0
        a = s * math.pi / 2
        n = 2.6 if cabin else 3.0
        x = wd * (math.cos(a) ** (2 / n)) if k < 6 else 0.0
        z = zb + gh * (math.sin(a) ** (2 / n))
        x *= lerp(1.0, wr / max(wd, 1e-3), s ** 1.25) if cabin else lerp(1.0, 0.82, s)
        if k == 1 and cabin: z = zb + 0.012
        pts.append((x, z))
    return pts  # 17 points, x>=0 half from bottom centre to top centre

def stations():
    ys = []
    N = 46
    for i in range(N + 1):
        t = i / N
        t = 0.5 - 0.5 * math.cos(math.pi * t)  # denser at ends
        ys.append(lerp(Y0, Y1, t))
    return ys

def build_body():
    bm = bmesh.new()
    ys = stations()
    rings = []
    for i, y in enumerate(ys):
        endf = 1.0
        d = min(y - Y0, Y1 - y)
        R = 0.22
        if d < R: endf = max(0.18, math.sqrt(max(0.0, 1 - ((R - d) / R) ** 2)))
        half = ring(y, endf)
        full = half + [(-x, z) for (x, z) in reversed(half[1:-1])]
        rings.append([bm.verts.new((x, y, z)) for (x, z) in full])
    H = len(ring(0.0)); n = len(rings[0])
    zones = []
    for a, b, ya, yb in zip(rings, rings[1:], ys, ys[1:]):
        ym = 0.5 * (ya + yb)
        cabin = interp(ROOF, ym) > 0.06
        for j in range(n):
            f = bm.faces.new((a[j], a[(j + 1) % n], b[(j + 1) % n], b[j]))
            seg = j if j < H - 1 else n - 1 - j
            z = 'Paint'
            if seg <= 3: z = 'Trim'
            if cabin:
                if seg == 9: z = 'Trim'
                if 0.15 < ym < 0.86 and seg >= 10: z = 'Glass'
                if -1.42 < ym <= 0.15 and 10 <= seg <= 12: z = 'Glass'
                if -1.78 < ym < -0.55 and seg >= 13: z = 'Glass'
            zones.append((f, z))
    for rr, flip in ((rings[0], True), (rings[-1], False)):
        c = Vector((0, 0, 0))
        for v in rr: c += v.co
        c /= len(rr)
        cv = bm.verts.new(c)
        for j in range(n):
            ff = (rr[j], rr[(j + 1) % n], cv)
            f = bm.faces.new(tuple(reversed(ff)) if flip else ff)
            zones.append((f, 'Paint'))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    names = ['Paint', 'Glass', 'Trim']
    for f, z in zones: f.material_index = names.index(z)
    me = bpy.data.meshes.new('Body'); bm.to_mesh(me); bm.free()
    ob = bpy.data.objects.new('Body', me); bpy.context.collection.objects.link(ob)
    add_mats(ob, names)
    return ob

def apply_mods(ob):
    bpy.context.view_layer.objects.active = ob
    for m in list(ob.modifiers):
        bpy.ops.object.modifier_apply(modifier=m.name)

body = build_body()
sub = body.modifiers.new('sub', 'SUBSURF'); sub.levels = 2; sub.render_levels = 2
apply_mods(body)

AXLES = [(1.24, 0.79), (-1.36, 0.80)]
for (ay, _) in AXLES:
    for side in (1, -1):
        bpy.ops.mesh.primitive_cylinder_add(vertices=72, radius=0.385, depth=0.7, location=(side * 0.9, ay, 0.33), rotation=(0, math.pi / 2, 0))
        c = bpy.context.active_object; c.data.materials.append(M['Trim'])
        mod = body.modifiers.new('arch', 'BOOLEAN'); mod.operation = 'DIFFERENCE'; mod.object = c; mod.solver = 'EXACT'
        mod.material_mode = 'TRANSFER'
        apply_mods(body)
        bpy.data.objects.remove(c, do_unlink=True)
for p in body.data.polygons: p.use_smooth = True
bpy.context.view_layer.objects.active = body
body.select_set(True)
try:
    bpy.ops.object.shade_smooth_by_angle(angle=math.radians(50))
except Exception:
    bpy.ops.object.shade_smooth()

# ------------------------------------------------------------ surface-projected decals (lights, seams, intakes)
from mathutils.bvhtree import BVHTree
dg = bpy.context.evaluated_depsgraph_get()
bvh = BVHTree.FromObject(body, dg)
decals = []
def strip(name, mat_name, path, ray_dir, h, off, ends_round=False):
    hits = []
    for p in path:
        loc, nrm, _, _ = bvh.ray_cast(Vector(p), Vector(ray_dir))
        if loc is not None: hits.append((loc, nrm))
    if len(hits) < 2: return
    bm = bmesh.new()
    rows = []
    for i, (loc, nrm) in enumerate(hits):
        a = hits[max(0, i - 1)][0]; b = hits[min(len(hits) - 1, i + 1)][0]
        t = (b - a).normalized()
        bn = nrm.cross(t).normalized()
        hh = h * 0.5
        if ends_round:
            u = i / (len(hits) - 1); hh *= max(0.25, math.sin(math.pi * u) ** 0.35)
        base = loc + nrm * off
        rows.append((bm.verts.new(base + bn * hh), bm.verts.new(base - bn * hh)))
    for (a1, a2), (b1, b2) in zip(rows, rows[1:]):
        bm.faces.new((a1, a2, b2, b1))
    me = bpy.data.meshes.new(name); bm.to_mesh(me); bm.free()
    ob = bpy.data.objects.new(name, me); bpy.context.collection.objects.link(ob)
    ob.data.materials.append(M[mat_name])
    nsum = Vector()
    for pp in me.polygons: nsum += pp.normal
    if nsum.dot(hits[len(hits) // 2][1]) < 0:
        me.flip_normals()
    for pp in me.polygons: pp.use_smooth = True
    decals.append(ob)

def linspace(a, b, n): return [lerp(a, b, i / (n - 1)) for i in range(n)]
for s in (1, -1):
    xs = linspace(0.30, 0.80, 26)
    strip('HeadHousing', 'HeadHousing', [(s * x, 6, 0.585 + 0.06 * sstep(0.3, 0.8, x)) for x in xs], (0, -1, 0), 0.085, 0.003, True)
    strip('HeadLED', 'Head', [(s * x, 6, 0.598 + 0.06 * sstep(0.3, 0.8, x)) for x in linspace(0.34, 0.78, 24)], (0, -1, 0), 0.02, 0.007, True)
    strip('HeadDRL', 'Head', [(s * x, 6, 0.565 + 0.05 * sstep(0.3, 0.8, x)) for x in linspace(0.36, 0.7, 20)], (0, -1, 0), 0.008, 0.007, True)
    for gy in (0.93, -0.6):
        strip('Seam', 'Gap', [(s * 3, gy, z) for z in linspace(0.26, interp(ZB, gy) - 0.03, 18)], (-s, 0, 0), 0.007, 0.0015)
    strip('Handle', 'Trim', [(s * 3, y, 0.8) for y in linspace(-0.48, -0.3, 6)], (-s, 0, 0), 0.025, 0.004, True)
    strip('Vent', 'Trim', [(s * 3, y, 0.5 + (y - 0.75) * 0.25) for y in linspace(0.62, 0.86, 8)], (-s, 0, 0), 0.05, 0.002, True)
    strip('HoodSeam', 'Gap', [(s * 0.66 + s * 0.06 * sstep(1.0, 2.0, y), y, 3) for y in linspace(0.88, 2.0, 20)], (0, 0, -1), 0.007, 0.0015)
strip('HoodSeamX', 'Gap', [(x, 0.885, 3) for x in linspace(-0.66, 0.66, 24)], (0, 0, -1), 0.007, 0.0015)
strip('TrunkSeam', 'Gap', [(x, -1.84, 3) for x in linspace(-0.78, 0.78, 24)], (0, 0, -1), 0.007, 0.0015)
strip('TailLens', 'TailLens', [(x, -6, 0.84) for x in linspace(-0.84, 0.84, 40)], (0, 1, 0), 0.075, 0.003)
strip('TailBar', 'Tail', [(x, -6, 0.845) for x in linspace(-0.82, 0.82, 40)], (0, 1, 0), 0.02, 0.007)
strip('TailLow', 'Tail', [(x, -6, 0.81) for x in linspace(-0.8, 0.8, 40)], (0, 1, 0), 0.006, 0.007)
strip('Plate', 'Plate', [(x, -6, 0.6) for x in linspace(-0.26, 0.26, 8)], (0, 1, 0), 0.12, 0.004)
strip('Diffuser', 'Trim', [(x, -6, 0.37) for x in linspace(-0.72, 0.72, 26)], (0, 1, 0), 0.12, 0.003)
strip('Intake', 'Trim', [(x, 6, 0.36) for x in linspace(-0.62, 0.62, 28)], (0, -1, 0), 0.14, 0.003, True)
strip('IntakeMesh', 'Gap', [(x, 6, 0.36) for x in linspace(-0.58, 0.58, 28)], (0, -1, 0), 0.1, 0.005, True)

# ------------------------------------------------------------ details
def obj_from_bm(name, bm, mats):
    me = bpy.data.meshes.new(name); bm.to_mesh(me); bm.free()
    ob = bpy.data.objects.new(name, me); bpy.context.collection.objects.link(ob)
    add_mats(ob, mats)
    for p in me.polygons: p.use_smooth = True
    return ob

parts = []
def prim(fn, mats, name, **kw):
    fn(**kw); o = bpy.context.active_object; o.name = name
    o.data.materials.clear()
    for n in mats: o.data.materials.append(M[n])
    bpy.ops.object.shade_smooth()
    parts.append(o); return o

# mirrors
for s in (1, -1):
    stalk = prim(bpy.ops.mesh.primitive_cube_add, ['Trim'], 'MirrorStalk', size=1, location=(s * 0.86, 0.66, 0.97))
    stalk.scale = (0.08, 0.05, 0.02)
    cap = prim(bpy.ops.mesh.primitive_uv_sphere_add, ['Paint'], 'MirrorCap', segments=24, ring_count=12, radius=1, location=(s * 0.95, 0.63, 1.0))
    cap.scale = (0.1, 0.065, 0.055); cap.rotation_euler = (0, 0, s * -0.15)
    glassm = prim(bpy.ops.mesh.primitive_cylinder_add, ['Chrome'], 'MirrorGlass', vertices=24, radius=1, depth=0.005, location=(s * 0.95, 0.575, 1.0), rotation=(math.pi / 2, 0, 0))
    glassm.scale = (0.085, 0.045, 1)

# exhaust tips
for s in (1, -1):
    prim(bpy.ops.mesh.primitive_cylinder_add, ['Chrome'], 'Exhaust', vertices=24, radius=0.045, depth=0.16, location=(s * 0.52, -2.2, 0.3), rotation=(math.pi / 2, 0, 0))
    prim(bpy.ops.mesh.primitive_cylinder_add, ['Gap'], 'ExhaustIn', vertices=24, radius=0.035, depth=0.02, location=(s * 0.52, -2.285, 0.3), rotation=(math.pi / 2, 0, 0))

# subtle rear wing lip
lip = prim(bpy.ops.mesh.primitive_cube_add, ['Paint'], 'Lip', size=1, location=(0, -2.04, 0.99))
lip.scale = (1.5, 0.22, 0.03); lip.rotation_euler = (-0.12, 0, 0)
bev = lip.modifiers.new('b', 'BEVEL'); bev.width = 0.012; bev.segments = 3

# underbody
ub = prim(bpy.ops.mesh.primitive_cube_add, ['Trim'], 'Under', size=1, location=(0, -0.05, 0.2))
ub.scale = (1.55, 4.1, 0.06)

# interior: dash, seats, steering wheel, tunnel
dash = prim(bpy.ops.mesh.primitive_cube_add, ['Interior'], 'Dash', size=1, location=(0, 0.62, 0.88))
dash.scale = (1.5, 0.35, 0.16); b2 = dash.modifiers.new('b', 'BEVEL'); b2.width = 0.04; b2.segments = 3
hood_cl = prim(bpy.ops.mesh.primitive_cube_add, ['Interior'], 'Cluster', size=1, location=(-0.38, 0.5, 0.98))
hood_cl.scale = (0.4, 0.18, 0.06); b3 = hood_cl.modifiers.new('b', 'BEVEL'); b3.width = 0.02; b3.segments = 3
sw = prim(bpy.ops.mesh.primitive_torus_add, ['Interior'], 'SteeringWheel', major_radius=0.17, minor_radius=0.018, major_segments=40, minor_segments=10, location=(-0.38, 0.36, 0.93), rotation=(math.radians(68), 0, 0))
hub = prim(bpy.ops.mesh.primitive_cylinder_add, ['Interior'], 'SWHub', vertices=20, radius=0.05, depth=0.06, location=(-0.38, 0.38, 0.92), rotation=(math.radians(68), 0, 0))
for s in (1, -1):
    seat = prim(bpy.ops.mesh.primitive_cube_add, ['Leather'], 'SeatBase', size=1, location=(s * 0.38, -0.25, 0.42))
    seat.scale = (0.48, 0.5, 0.12); b4 = seat.modifiers.new('b', 'BEVEL'); b4.width = 0.04; b4.segments = 3
    back = prim(bpy.ops.mesh.primitive_cube_add, ['Leather'], 'SeatBack', size=1, location=(s * 0.38, -0.55, 0.72))
    back.scale = (0.46, 0.12, 0.6); back.rotation_euler = (-0.25, 0, 0); b5 = back.modifiers.new('b', 'BEVEL'); b5.width = 0.05; b5.segments = 3
tun = prim(bpy.ops.mesh.primitive_cube_add, ['Interior'], 'Tunnel', size=1, location=(0, 0.0, 0.45))
tun.scale = (0.22, 1.2, 0.22)
floor = prim(bpy.ops.mesh.primitive_cube_add, ['Interior'], 'Floor', size=1, location=(0, -0.1, 0.3))
floor.scale = (1.6, 2.6, 0.04)
shelf = prim(bpy.ops.mesh.primitive_cube_add, ['Interior'], 'Shelf', size=1, location=(0, -1.3, 0.86))
shelf.scale = (1.4, 0.7, 0.04)

# join details into body (keep wheels separate)
for o in parts + decals:
    o.select_set(True); bpy.context.view_layer.objects.active = o
    for m in list(o.modifiers): bpy.ops.object.modifier_apply(modifier=m.name)
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    o.select_set(False)
bpy.ops.object.select_all(action='DESELECT')
for o in parts + decals: o.select_set(True)
body.select_set(True); bpy.context.view_layer.objects.active = body
bpy.ops.object.join()
body = bpy.context.active_object; body.name = 'Body'

# ------------------------------------------------------------ wheel
def build_wheel():
    bm = bmesh.new()
    segs = 72
    # tyre profile (r, x) — outer side x>0
    prof = [(0.232, 0.098), (0.255, 0.112), (0.285, 0.119), (0.31, 0.118), (0.324, 0.11), (0.331, 0.095),
            (0.332, 0.06), (0.326, 0.052), (0.326, 0.038), (0.332, 0.03), (0.332, -0.03), (0.326, -0.038), (0.326, -0.052), (0.332, -0.06),
            (0.331, -0.095), (0.324, -0.11), (0.31, -0.118), (0.285, -0.119), (0.255, -0.112), (0.232, -0.098)]
    rings = []
    for i in range(segs):
        a = 2 * math.pi * i / segs
        ca, sa = math.cos(a), math.sin(a)
        rings.append([bm.verts.new((x, r * ca, r * sa)) for (r, x) in prof])
    n = len(prof)
    for i in range(segs):
        A, B = rings[i], rings[(i + 1) % segs]
        for j in range(n - 1):
            bm.faces.new((A[j], A[j + 1], B[j + 1], B[j]))
    tyre = obj_from_bm('Tyre', bm, ['Rubber'])

    objs = [tyre]
    def P(fn, mats, **kw):
        fn(**kw); o = bpy.context.active_object
        o.data.materials.clear()
        for nn in mats: o.data.materials.append(M[nn])
        bpy.ops.object.shade_smooth(); objs.append(o); return o
    # barrel + lip
    P(bpy.ops.mesh.primitive_cylinder_add, ['RimDark'], vertices=64, radius=0.229, depth=0.2, location=(-0.005, 0, 0), rotation=(0, math.pi / 2, 0), end_fill_type='NOTHING')
    P(bpy.ops.mesh.primitive_torus_add, ['Rim'], major_radius=0.226, minor_radius=0.009, major_segments=72, minor_segments=8, location=(0.098, 0, 0), rotation=(0, math.pi / 2, 0))
    # brake disc (spins)
    P(bpy.ops.mesh.primitive_cylinder_add, ['Disc'], vertices=56, radius=0.175, depth=0.028, location=(-0.035, 0, 0), rotation=(0, math.pi / 2, 0))
    P(bpy.ops.mesh.primitive_cylinder_add, ['RimDark'], vertices=32, radius=0.085, depth=0.05, location=(-0.02, 0, 0), rotation=(0, math.pi / 2, 0))
    # 10 spokes (5 twin), slightly concave
    for k in range(10):
        a = 2 * math.pi * (k // 2) / 5 + (0.17 if k % 2 else -0.17)
        o = P(bpy.ops.mesh.primitive_cube_add, ['Rim'], size=1)
        o.scale = (0.022, 0.03, 0.17)
        r = 0.125
        o.location = (0.07, r * math.sin(a), r * math.cos(a))
        o.rotation_euler = (-a, 0, 0)
        o.rotation_euler.rotate_axis('Y', 0.18)
        bv = o.modifiers.new('b', 'BEVEL'); bv.width = 0.006; bv.segments = 2
        tp = o.modifiers.new('t', 'SIMPLE_DEFORM'); tp.deform_method = 'TAPER'; tp.factor = 0.5; tp.deform_axis = 'Z'
    # hub face & cap
    P(bpy.ops.mesh.primitive_cylinder_add, ['Rim'], vertices=40, radius=0.065, depth=0.04, location=(0.075, 0, 0), rotation=(0, math.pi / 2, 0))
    P(bpy.ops.mesh.primitive_cylinder_add, ['RimDark'], vertices=24, radius=0.028, depth=0.012, location=(0.098, 0, 0), rotation=(0, math.pi / 2, 0))
    for k in range(5):
        a = 2 * math.pi * k / 5 + math.pi / 5
        P(bpy.ops.mesh.primitive_cylinder_add, ['Chrome'], vertices=6, radius=0.009, depth=0.02, location=(0.096, 0.046 * math.sin(a), 0.046 * math.cos(a)), rotation=(0, math.pi / 2, 0))
    for o in objs:
        bpy.ops.object.select_all(action='DESELECT'); o.select_set(True); bpy.context.view_layer.objects.active = o
        for m in list(o.modifiers): bpy.ops.object.modifier_apply(modifier=m.name)
        bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    bpy.ops.object.select_all(action='DESELECT')
    for o in objs: o.select_set(True)
    bpy.context.view_layer.objects.active = tyre
    bpy.ops.object.join()
    w = bpy.context.active_object; w.name = 'Wheel'
    return w

def build_caliper():
    bm = bmesh.new()
    segs = 10; r0, r1 = 0.13, 0.19
    a0, a1 = math.radians(20), math.radians(100)
    rings = []
    for i in range(segs + 1):
        a = lerp(a0, a1, i / segs)
        ca, sa = math.cos(a), math.sin(a)
        rings.append([bm.verts.new((x, r * ca, r * sa)) for (x, r) in ((-0.075, r0), (-0.075, r1), (0.005, r1), (0.005, r0))])
    for A, B in zip(rings, rings[1:]):
        for j in range(4): bm.faces.new((A[j], A[(j + 1) % 4], B[(j + 1) % 4], B[j]))
    bm.faces.new(rings[0]); bm.faces.new(list(reversed(rings[-1])))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    o = obj_from_bm('Caliper', bm, ['Caliper'])
    bv = o.modifiers.new('b', 'BEVEL'); bv.width = 0.01; bv.segments = 2
    bpy.context.view_layer.objects.active = o
    bpy.ops.object.modifier_apply(modifier='b')
    return o

wheel = build_wheel()
caliper = build_caliper()

# ------------------------------------------------------------ preview render
def preview(path):
    sc = bpy.context.scene
    # placed wheels for preview only
    temps = []
    for (ay, tx) in AXLES:
        for s in (1, -1):
            w = wheel.copy(); w.data = wheel.data; bpy.context.collection.objects.link(w)
            w.location = (s * tx, ay, 0.33); w.scale = (s, 1, 1); temps.append(w)
            c = caliper.copy(); c.data = caliper.data; bpy.context.collection.objects.link(c)
            c.location = (s * tx, ay, 0.33); c.scale = (s, 1, 1); temps.append(c)
    wheel.hide_render = True; caliper.hide_render = True
    bpy.ops.mesh.primitive_plane_add(size=40, location=(0, 0, 0)); gp = bpy.context.active_object
    gm = mat('Ground', (0.22, 0.2, 0.19), rough=0.6); gp.data.materials.append(gm); temps.append(gp)
    sc.render.engine = 'CYCLES'; sc.cycles.samples = 48; sc.cycles.use_denoising = True
    sc.render.resolution_x = 960; sc.render.resolution_y = 540
    world = bpy.data.worlds.new('W'); sc.world = world; world.use_nodes = True
    bg = world.node_tree.nodes['Background']
    sky = world.node_tree.nodes.new('ShaderNodeTexSky'); sky.sky_type = 'NISHITA'; sky.sun_elevation = math.radians(12); sky.sun_rotation = math.radians(200)
    world.node_tree.links.new(sky.outputs[0], bg.inputs[0]); bg.inputs[1].default_value = 0.6
    sc.view_settings.view_transform = 'AgX'
    bpy.ops.object.camera_add(); cam = bpy.context.active_object; sc.camera = cam
    cam.data.lens = 50
    views = [((5.2, 4.6, 1.6), 'front'), ((-4.8, -5.0, 1.9), 'rear'), ((6.5, 0.2, 0.9), 'side')]
    for loc, nm in views:
        cam.location = loc
        d = Vector((0, 0, 0.6)) - Vector(loc)
        cam.rotation_euler = d.to_track_quat('-Z', 'Y').to_euler()
        sc.render.filepath = f'{path}_{nm}.png'
        bpy.ops.render.render(write_still=True)
    for t in temps: bpy.data.objects.remove(t, do_unlink=True)
    bpy.data.objects.remove(cam, do_unlink=True)
    wheel.hide_render = False; caliper.hide_render = False

if '--preview' in sys.argv:
    preview(OUT)

# ------------------------------------------------------------ ambient occlusion -> vertex colours
def bake_ao(objs):
    sc = bpy.context.scene
    sc.render.engine = 'CYCLES'; sc.cycles.samples = 64; sc.cycles.device = 'CPU'
    sc.render.bake.target = 'VERTEX_COLORS'
    bpy.ops.mesh.primitive_plane_add(size=30, location=(0, 0, 0)); gp = bpy.context.active_object
    gp.data.materials.append(mat('AOGround', (0.5, 0.5, 0.5)))
    for o in objs:
        me = o.data
        ca = me.color_attributes.new('AO', 'FLOAT_COLOR', 'POINT')
        me.color_attributes.active_color = ca
        bpy.ops.object.select_all(action='DESELECT'); o.select_set(True); bpy.context.view_layer.objects.active = o
        bpy.ops.object.bake(type='AO')
        for d in ca.data:
            v = d.color[0]; v = 0.3 + 0.7 * (v ** 0.8); d.color = (v, v, v, 1)
        print('AO baked', o.name)
    bpy.data.objects.remove(gp, do_unlink=True)
if '--noao' not in sys.argv:
    for o in (body, wheel, caliper): o.location = (0, 0, 0)
    body_z = body.location.z
    bake_ao([body])
    wheel.location = (0, 0, 2.0); caliper.location = (0, 0, 2.0)   # lift wheel off ground for its own AO
    bake_ao([wheel, caliper])

# ------------------------------------------------------------ export
for o in (body, wheel, caliper): o.location = (0, 0, 0)
bpy.ops.object.select_all(action='DESELECT')
for o in (body, wheel, caliper): o.select_set(True)
tris = sum(len(p.vertices) - 2 for o in (body, wheel, caliper) for p in o.data.polygons)
print('TRIS', tris)
bpy.ops.export_scene.gltf(filepath=OUT + '.glb', export_format='GLB', use_selection=True, export_apply=True,
                          export_yup=True, export_normals=True, export_texcoords=False, export_draco_mesh_compression_enable=False, export_vertex_color='ACTIVE')
print('EXPORTED')
