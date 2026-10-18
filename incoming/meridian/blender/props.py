# Meridian props — stylised-realistic vegetation & roadside kit, vertex coloured, origin at ground.
import bpy, bmesh, math, random, sys
from mathutils import Vector, Matrix, noise

OUT = sys.argv[sys.argv.index('--') + 1] if '--' in sys.argv else '/tmp/props'
bpy.ops.object.select_all(action='SELECT'); bpy.ops.object.delete()

def srgb(h):
    c = [((h >> 16) & 255) / 255, ((h >> 8) & 255) / 255, (h & 255) / 255]
    return [x / 12.92 if x <= 0.04045 else ((x + 0.055) / 1.055) ** 2.4 for x in c]
def mix(a, b, t): return [a[i] + (b[i] - a[i]) * t for i in range(3)]
def lerp(a, b, t): return a + (b - a) * t

class Builder:
    def __init__(self): self.bm = bmesh.new(); self.col = {}  # face -> color
    def face(self, verts, color):
        vs = [self.bm.verts.new(v) for v in verts]
        f = self.bm.faces.new(vs); self.col[f] = color; return f
    def add_bm(self, other_bm, colfn, matrix=Matrix()):
        other_bm.normal_update()
        vmap = {}
        for v in other_bm.verts: vmap[v] = self.bm.verts.new(matrix @ v.co)
        for f in other_bm.faces:
            nf = self.bm.faces.new([vmap[v] for v in f.verts])
            c = (matrix.to_3x3() @ f.calc_center_median())
            self.col[nf] = colfn(f, matrix @ f.calc_center_median(), (matrix.to_3x3() @ f.normal).normalized())
    def finish(self, name):
        bmesh.ops.recalc_face_normals(self.bm, faces=self.bm.faces)
        me = bpy.data.meshes.new(name)
        cols = [self.col.get(f, [1, 1, 1]) for f in self.bm.faces]
        self.bm.to_mesh(me); self.bm.free()
        attr = me.color_attributes.new('Col', 'FLOAT_COLOR', 'CORNER')
        for p, c in zip(me.polygons, cols):
            for li in p.loop_indices: attr.data[li].color = (*c, 1.0)
        me.color_attributes.active_color = attr
        ob = bpy.data.objects.new(name, me); bpy.context.collection.objects.link(ob)
        m = bpy.data.materials.new(name + 'M'); m.use_nodes = True
        nt = m.node_tree; ca = nt.nodes.new('ShaderNodeVertexColor'); ca.layer_name = 'Col'
        nt.links.new(ca.outputs[0], nt.nodes['Principled BSDF'].inputs['Base Color'])
        nt.nodes['Principled BSDF'].inputs['Roughness'].default_value = 0.9
        me.materials.append(m)
        for p in me.polygons: p.use_smooth = False
        print(name, 'tris', sum(len(p.vertices) - 2 for p in me.polygons))
        return ob

def cone_bm(r, h, segs, droop=0.0, jag=0.0, rng=None, top_off=(0, 0)):
    bm = bmesh.new()
    top = bm.verts.new((top_off[0], top_off[1], h))
    ring = []
    for i in range(segs * 2 if jag else segs):
        a = 2 * math.pi * i / (segs * 2 if jag else segs) + (rng.random() * 0.2 if rng else 0)
        rr = r * ((1 - jag) if (jag and i % 2) else 1) * (0.88 + (rng.random() * 0.24 if rng else 0.12))
        dz = -droop * (rr / r) + (rng.uniform(-0.08, 0.08) if rng else 0)
        ring.append(bm.verts.new((math.cos(a) * rr, math.sin(a) * rr, dz)))
    for i in range(len(ring)):
        bm.faces.new((ring[i], ring[(i + 1) % len(ring)], top))
    # underside (slightly concave)
    mid = bm.verts.new((0, 0, h * 0.18))
    for i in range(len(ring)):
        bm.faces.new((ring[(i + 1) % len(ring)], ring[i], mid))
    return bm

def cyl_bm(r0, r1, h, segs, bend=(0, 0), rings=4, rng=None):
    bm = bmesh.new(); loops = []
    for k in range(rings + 1):
        t = k / rings
        r = lerp(r0, r1, t)
        ox = bend[0] * t * t; oy = bend[1] * t * t
        loops.append([bm.verts.new((ox + math.cos(2 * math.pi * i / segs) * r * (1 + (rng.uniform(-0.08, 0.08) if rng else 0)),
                                    oy + math.sin(2 * math.pi * i / segs) * r, h * t)) for i in range(segs)])
    for a, b in zip(loops, loops[1:]):
        for i in range(segs): bm.faces.new((a[i], a[(i + 1) % segs], b[(i + 1) % segs], b[i]))
    bm.faces.new(list(reversed(loops[0]))); bm.faces.new(loops[-1])
    return bm

def blob_bm(r, sub, rng, squash=1.0, amp=0.25, seed=0.0):
    bm = bmesh.new(); bmesh.ops.create_icosphere(bm, subdivisions=sub, radius=r)
    for v in bm.verts:
        n = noise.noise(v.co * (1.6 / r) + Vector((seed, seed * 1.7, seed * 0.3)))
        v.co *= 1 + amp * n
        v.co.z *= squash
    return bm

# ---------------------------------------------------------------- pine
def pine(name, snow=False, seed=1):
    rng = random.Random(seed); B = Builder()
    bark = srgb(0x4a3022); bark2 = srgb(0x6b4a34)
    trunk = cyl_bm(0.32, 0.08, 8.8, 7, rings=5, rng=rng)
    B.add_bm(trunk, lambda f, c, n: mix(bark, bark2, max(0, n.x) * 0.5 + rng.random() * 0.2))
    tiers = 10
    deep = srgb(0x1c3a2a); mid = srgb(0x2c5a3a); tip = srgb(0x4f7d4a)
    snowc = srgb(0xeef3fb); snowsh = srgb(0xb9c7dc)
    for i in range(tiers):
        t = i / (tiers - 1)
        z = lerp(1.7, 8.2, t)
        r = lerp(2.9, 0.55, t ** 0.85) * rng.uniform(0.9, 1.1)
        h = lerp(1.5, 1.0, t)
        cb = cone_bm(r, h, 7, droop=lerp(0.65, 0.2, t), jag=0.32, rng=rng, top_off=(rng.uniform(-0.1, 0.1), rng.uniform(-0.1, 0.1)))
        rot = Matrix.Rotation(rng.random() * 6.28, 4, 'Z')
        M_ = Matrix.Translation((0, 0, z)) @ rot
        def colfn(f, c, n, t=t):
            radial = min(1, Vector((c.x, c.y)).length / max(r, 0.1))
            base = mix(deep, mid, radial * 0.8 + t * 0.3)
            base = mix(base, tip, max(0, radial - 0.6) * 1.2 * rng.uniform(0.6, 1))
            if n.z < 0: base = mix(base, deep, 0.6)
            if snow and n.z > 0.35:
                base = mix(base, snowc if n.z > 0.6 else snowsh, min(1, (n.z - 0.3) * 2.2) * rng.uniform(0.75, 1))
            return base
        B.add_bm(cb, colfn, M_)
    top = cone_bm(0.35, 1.3, 6, rng=rng)
    B.add_bm(top, lambda f, c, n: snowc if (snow and n.z > 0.4) else tip, Matrix.Translation((0, 0, 8.6)))
    return B.finish(name)

# ---------------------------------------------------------------- joshua tree
def joshua(name, seed=3):
    rng = random.Random(seed); B = Builder()
    bark = srgb(0x5b4a3a); shag = srgb(0x7a6550); leaf = srgb(0x6f7a3c); leaf2 = srgb(0x9aa255); dead = srgb(0x8a7a5a)
    def spikeball(center, r):
        bm = bmesh.new(); bmesh.ops.create_icosphere(bm, subdivisions=1, radius=r * 0.35)
        B.add_bm(bm, lambda f, c, n: dead, Matrix.Translation(center))
        for k in range(34):
            d = Vector((rng.gauss(0, 1), rng.gauss(0, 1), abs(rng.gauss(0.4, 0.8)))).normalized()
            L = r * rng.uniform(0.8, 1.25)
            base = Vector(center)
            perp = d.orthogonal().normalized() * r * 0.08
            perp2 = d.cross(perp).normalized() * r * 0.08
            tipv = base + d * L
            for a, b in ((perp, perp2), (perp2, -perp), (-perp, -perp2)):
                B.face([base + a, base + b, tipv], mix(leaf, leaf2, rng.random()))
    def branch(start, direction, length, radius, depth):
        d = direction.normalized()
        bm = cyl_bm(radius, radius * 0.72, length, 6, rings=2, rng=rng)
        q = Vector((0, 0, 1)).rotation_difference(d)
        Mx = Matrix.Translation(start) @ q.to_matrix().to_4x4()
        B.add_bm(bm, lambda f, c, n: mix(bark, shag, rng.random() * 0.7), Mx)
        end = start + d * length
        if depth == 0:
            spikeball(end, rng.uniform(0.62, 0.8)); return
        for k in range(rng.choice([2, 2, 3])):
            nd = (d + Vector((rng.uniform(-1, 1), rng.uniform(-1, 1), rng.uniform(0.0, 0.9))) * 0.9)
            nd.z = max(nd.z, 0.25)
            branch(end, nd, length * rng.uniform(0.6, 0.85), radius * 0.68, depth - 1)
    branch(Vector((0, 0, -0.1)), Vector((rng.uniform(-0.15, 0.15), 0.1, 1)), 2.4, 0.4, 3)
    return B.finish(name)

# ---------------------------------------------------------------- cypress (windswept coastal)
def cypress(name, seed=5):
    rng = random.Random(seed); B = Builder()
    bark = srgb(0x4b3a2c); dark = srgb(0x2c4428); mid = srgb(0x45663a); light = srgb(0x6e8c4c)
    lean = (1.4, 0.3)
    B.add_bm(cyl_bm(0.35, 0.14, 5.0, 7, bend=lean, rings=5, rng=rng), lambda f, c, n: bark)
    for k in range(9):
        t = k / 8
        cx = lean[0] * (0.5 + 0.6 * t) + rng.uniform(-1.6, 1.6) + 1.0 * t
        cy = lean[1] + rng.uniform(-1.4, 1.4)
        cz = 4.6 + rng.uniform(-0.4, 1.2)
        bb = blob_bm(rng.uniform(1.3, 2.2), 2, rng, squash=0.5, amp=0.35, seed=k * 3.1)
        B.add_bm(bb, lambda f, c, n: mix(dark, mix(mid, light, rng.random() * 0.6), max(0, n.z) * 0.9 + 0.1), Matrix.Translation((cx, cy, cz)))
    return B.finish(name)

# ---------------------------------------------------------------- rocks
def rock(name, seed, stretch=(1, 1, 0.6)):
    rng = random.Random(seed)
    bm = bmesh.new(); bmesh.ops.create_icosphere(bm, subdivisions=3, radius=1)
    off = Vector((seed * 1.3, seed * 2.1, seed * 0.7))
    for v in bm.verts:
        p = v.co.copy()
        n1 = noise.noise(p * 1.2 + off); n2 = noise.noise(p * 3.1 + off * 2)
        cell = noise.voronoi(p * 1.7 + off)[0][0] if hasattr(noise, 'voronoi') else 0
        v.co = p * (1 + 0.32 * n1 + 0.12 * n2 - 0.15 * cell)
        v.co.x *= stretch[0]; v.co.y *= stretch[1]; v.co.z *= stretch[2]
        if v.co.z < -0.25: v.co.z = -0.25 + (v.co.z + 0.25) * 0.15
    bmesh.ops.translate(bm, verts=bm.verts, vec=(0, 0, 0.2))
    me = bpy.data.meshes.new('tmp'); bm.to_mesh(me); bm.free()
    tmp = bpy.data.objects.new('tmp', me); bpy.context.collection.objects.link(tmp)
    dm = tmp.modifiers.new('d', 'DECIMATE'); dm.ratio = 0.5
    bpy.context.view_layer.objects.active = tmp; bpy.ops.object.modifier_apply(modifier='d')
    bm2 = bmesh.new(); bm2.from_mesh(tmp.data)
    bpy.data.objects.remove(tmp, do_unlink=True)
    B = Builder()
    a = srgb(0x8e857c); b = srgb(0x6a625c); c_ = srgb(0xa49a8c); moss = srgb(0x6b6e4a)
    def colfn(f, c, n):
        strata = 0.5 + 0.5 * math.sin(c.z * 9 + noise.noise(c * 2) * 2)
        col = mix(b, a, strata)
        col = mix(col, c_, max(0, n.z) * 0.35)
        if n.z > 0.75 and rng.random() < 0.35: col = mix(col, moss, 0.35)
        return mix(col, b, max(0, -n.z) * 0.6)
    B.add_bm(bm2, colfn)
    return B.finish(name)

# ---------------------------------------------------------------- shrub (sagebrush)
def shrub(name, seed=9):
    rng = random.Random(seed); B = Builder()
    g1 = srgb(0x6f7553); g2 = srgb(0x9a9a72); g3 = srgb(0x55593e)
    for k in range(6):
        a = rng.random() * 6.28; d = rng.uniform(0, 0.45)
        bb = blob_bm(rng.uniform(0.3, 0.55), 1, rng, squash=0.75, amp=0.45, seed=k * 1.7)
        B.add_bm(bb, lambda f, c, n: mix(g3, mix(g1, g2, rng.random()), max(0, n.z) * 0.8 + 0.2), Matrix.Translation((math.cos(a) * d, math.sin(a) * d, 0.28 + rng.uniform(0, 0.15))))
    return B.finish(name)

# ---------------------------------------------------------------- grass tuft
def grass(name, seed=11):
    rng = random.Random(seed); B = Builder()
    base = srgb(0x5a5a34); tip = srgb(0xc9b98a)
    for k in range(16):
        a = rng.random() * 6.28; r = rng.uniform(0, 0.18)
        h = rng.uniform(0.35, 0.75); w = rng.uniform(0.025, 0.05)
        lean = Vector((math.cos(a), math.sin(a), 0)) * rng.uniform(0.1, 0.35)
        p = Vector((math.cos(a) * r, math.sin(a) * r, 0))
        side = Vector((-math.sin(a), math.cos(a), 0)) * w
        tipp = p + lean + Vector((0, 0, h))
        B.face([p - side, p + side, tipp], mix(base, tip, rng.uniform(0.3, 0.8)))
    return B.finish(name)

# ---------------------------------------------------------------- delineator post
def delineator(name):
    B = Builder()
    white = srgb(0xe4e0d6); grey = srgb(0x9a968e)
    bm = bmesh.new()
    bmesh.ops.create_cube(bm, size=1)
    for v in bm.verts:
        v.co.x *= 0.1; v.co.y *= 0.035; v.co.z = (v.co.z + 0.5) * 1.1
    bmesh.ops.bevel(bm, geom=[e for e in bm.edges if abs(e.verts[0].co.z - e.verts[1].co.z) > 0.5], offset=0.012, segments=2, affect='EDGES')
    B.add_bm(bm, lambda f, c, n: white if c.z > 0.08 else grey)
    return B.finish(name)

objs = [pine('Pine', seed=1), pine('SnowPine', snow=True, seed=2), joshua('Joshua', seed=3), cypress('Cypress', seed=5),
        rock('Rock', 4, (1.1, 0.9, 0.62)), rock('Rock2', 7, (1.4, 0.8, 0.45)), shrub('Shrub'), grass('Grass'), delineator('Delineator')]

if '--preview' in sys.argv:
    sc = bpy.context.scene
    for i, o in enumerate(objs): o.location = ((i % 5) * 7 - 14, (i // 5) * 9, 0)
    bpy.ops.mesh.primitive_plane_add(size=120); g = bpy.context.active_object
    gm = bpy.data.materials.new('G'); gm.use_nodes = True; gm.node_tree.nodes['Principled BSDF'].inputs['Base Color'].default_value = (0.3, 0.2, 0.15, 1); g.data.materials.append(gm)
    sc.render.engine = 'CYCLES'; sc.cycles.samples = 32; sc.cycles.use_denoising = True
    sc.render.resolution_x = 1200; sc.render.resolution_y = 600
    w = bpy.data.worlds.new('W'); sc.world = w; w.use_nodes = True
    sky = w.node_tree.nodes.new('ShaderNodeTexSky'); sky.sky_type = 'NISHITA'; sky.sun_elevation = math.radians(25)
    w.node_tree.links.new(sky.outputs[0], w.node_tree.nodes['Background'].inputs[0]); w.node_tree.nodes['Background'].inputs[1].default_value = 0.5
    sc.view_settings.view_transform = 'AgX'
    bpy.ops.object.camera_add(location=(0, -32, 9)); cam = bpy.context.active_object; sc.camera = cam
    cam.rotation_euler = (Vector((0, 4, 3)) - cam.location).to_track_quat('-Z', 'Y').to_euler(); cam.data.lens = 32
    sc.render.filepath = OUT + '_preview.png'; bpy.ops.render.render(write_still=True)
    bpy.data.objects.remove(g, do_unlink=True); bpy.data.objects.remove(cam, do_unlink=True)
    for o in objs: o.location = (0, 0, 0)

bpy.ops.object.select_all(action='DESELECT')
for o in objs: o.select_set(True)
kw = dict(filepath=OUT + '.glb', export_format='GLB', use_selection=True, export_yup=True, export_normals=True, export_texcoords=False, export_materials='NONE')
try:
    bpy.ops.export_scene.gltf(**kw, export_vertex_color='ACTIVE')
except TypeError:
    bpy.ops.export_scene.gltf(**kw, export_colors=True)
print('EXPORTED')
