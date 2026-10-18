# Meridian nature kit v2 — textured, game-ready vegetation & rocks.
# Pipeline: (1) model a fir branch + a leaf cluster in Blender and render them to alpha cards (albedo + normal),
# (2) build card-based pines / cypress with bark-textured trunks and bent-normal foliage,
# (3) import Poly Haven CC0 scans (rocks, quiver tree, shrubs), decimate to game budgets,
# (4) build grass clump cards from the scanned grass atlas, (5) export one GLB with WebP textures.
# Run: blender -b -P nature.py -- <phase> ; phase = cards | build
import bpy, bmesh, math, random, sys, os, json
from mathutils import Vector, Matrix, Euler, noise

PH = '/tmp/ph'
CARDS = '/tmp/cards'
OUT = '/tmp/nature'
os.makedirs(CARDS, exist_ok=True)
phase = sys.argv[sys.argv.index('--') + 1] if '--' in sys.argv else 'build'

def reset():
    bpy.ops.wm.read_factory_settings(use_empty=True)

def lerp(a, b, t): return a + (b - a) * t

def new_obj(name, bm):
    me = bpy.data.meshes.new(name); bm.to_mesh(me); bm.free()
    ob = bpy.data.objects.new(name, me); bpy.context.scene.collection.objects.link(ob)
    return ob

def color_attr(me, name='Col'):
    a = me.color_attributes.get(name) or me.color_attributes.new(name, 'FLOAT_COLOR', 'CORNER')
    me.color_attributes.active_color = a
    return a

# =====================================================================================
# 1. card rendering
# =====================================================================================
def emission_mat(name, kind, image=None, alpha_img=None, crop=None):
    m = bpy.data.materials.new(name); m.use_nodes = True
    nt = m.node_tree; N = nt.nodes; L = nt.links
    for n in list(N): N.remove(n)
    out = N.new('ShaderNodeOutputMaterial')
    em = N.new('ShaderNodeEmission')
    if kind == 'albedo':
        if image:
            tex = N.new('ShaderNodeTexImage'); tex.image = image
            uv = N.new('ShaderNodeUVMap')
            L.new(uv.outputs[0], tex.inputs[0])
            vc = N.new('ShaderNodeVertexColor'); vc.layer_name = 'Col'
            mul = N.new('ShaderNodeMixRGB'); mul.blend_type = 'MULTIPLY'; mul.inputs[0].default_value = 1
            L.new(tex.outputs[0], mul.inputs[1]); L.new(vc.outputs[0], mul.inputs[2])
            L.new(mul.outputs[0], em.inputs[0])
        else:
            vc = N.new('ShaderNodeVertexColor'); vc.layer_name = 'Col'
            L.new(vc.outputs[0], em.inputs[0])
    else:  # world normal → colour
        geo = N.new('ShaderNodeNewGeometry')
        vm = N.new('ShaderNodeVectorMath'); vm.operation = 'MULTIPLY_ADD'
        vm.inputs[1].default_value = (0.5, 0.5, 0.5); vm.inputs[2].default_value = (0.5, 0.5, 0.5)
        L.new(geo.outputs['Normal'], vm.inputs[0]); L.new(vm.outputs[0], em.inputs[0])
    if alpha_img is not None:
        at = N.new('ShaderNodeTexImage'); at.image = alpha_img; at.image.colorspace_settings.name = 'Non-Color'
        uv2 = N.new('ShaderNodeUVMap'); L.new(uv2.outputs[0], at.inputs[0])
        tr = N.new('ShaderNodeBsdfTransparent'); mix = N.new('ShaderNodeMixShader')
        L.new(at.outputs[0], mix.inputs[0]); L.new(tr.outputs[0], mix.inputs[1]); L.new(em.outputs[0], mix.inputs[2])
        L.new(mix.outputs[0], out.inputs[0])
        m.blend_method = 'CLIP'
    else:
        L.new(em.outputs[0], out.inputs[0])
    return m

def render_card(path_base, w_m, h_m, res=(1024, 512)):
    sc = bpy.context.scene
    sc.render.engine = 'CYCLES'; sc.cycles.samples = 24; sc.cycles.use_denoising = False
    sc.render.film_transparent = True
    sc.render.resolution_x, sc.render.resolution_y = res
    sc.render.image_settings.file_format = 'PNG'; sc.render.image_settings.color_mode = 'RGBA'
    sc.cycles.max_bounces = 0
    cam_d = bpy.data.cameras.new('cam'); cam_d.type = 'ORTHO'; cam_d.ortho_scale = max(w_m, h_m)
    cam = bpy.data.objects.new('cam', cam_d); sc.collection.objects.link(cam); sc.camera = cam
    cam.location = (w_m / 2, 0, 5); cam.rotation_euler = (0, 0, 0)
    sc.render.pixel_aspect_x = 1
    obs = [o for o in sc.objects if o.type == 'MESH']
    for kind in ('albedo', 'normal'):
        sc.view_settings.view_transform = 'Standard' if kind == 'albedo' else 'Raw'
        for o in obs:
            src = o.get('alb_mat') if kind == 'albedo' else o.get('nor_mat')
            o.data.materials.clear(); o.data.materials.append(bpy.data.materials[src])
        sc.render.filepath = f'{path_base}_{kind}.png'
        bpy.ops.render.render(write_still=True)

def needle_bm(bm, base, direction, length, width, col, cols):
    d = direction.normalized(); side = d.cross(Vector((0, 0, 1)))
    if side.length < 1e-4: side = Vector((1, 0, 0))
    side.normalize(); up = side.cross(d).normalized()
    tip = base + d * length
    a = base + side * width; b = base - side * width; c = base + up * width * 0.8
    vs = [bm.verts.new(v) for v in (a, b, c, tip)]
    for f in ((vs[0], vs[1], vs[3]), (vs[1], vs[2], vs[3]), (vs[2], vs[0], vs[3])):
        ff = bm.faces.new(f); cols[ff] = col

def build_fir_branch(seed=2):
    rng = random.Random(seed)
    bm = bmesh.new(); cols = {}
    dark = (0.035, 0.075, 0.035); mid = (0.07, 0.14, 0.06); light = (0.14, 0.22, 0.09); stemc = (0.16, 0.1, 0.06)
    L = 1.0
    def twig(start, d, length, thick, depth):
        d = d.normalized()
        steps = 10
        for s in range(steps):
            t0 = s / steps; p = start + d * length * t0
            # stem segment as thin box
            needle_bm(bm, p, d, length / steps * 1.05, thick * (1 - t0 * 0.7), stemc, cols)
            # needles: two flat rows + some upward
            n = 9 if depth else 7
            for k in range(n):
                tt = t0 + rng.random() / steps
                q = start + d * length * tt
                side = 1 if k % 2 else -1
                perp = Vector((-d.y, d.x, 0)) * side
                nd = (perp * rng.uniform(0.6, 1.0) + d * rng.uniform(0.35, 0.7) + Vector((0, 0, rng.uniform(-0.05, 0.25)))).normalized()
                ln = rng.uniform(0.045, 0.07) * (1 - 0.35 * tt)
                c = tuple(lerp(lerp(dark[i], mid[i], rng.random()), light[i], (tt ** 2) * 0.8) for i in range(3))
                needle_bm(bm, q, nd, ln, 0.0022, c, cols)
        if depth > 0:
            for k in range(7):
                t = 0.12 + k * 0.12
                if t > 0.92: break
                side = 1 if k % 2 else -1
                base = start + d * length * t
                ang = math.radians(rng.uniform(42, 58)) * side
                nd = Vector((d.x * math.cos(ang) - d.y * math.sin(ang), d.x * math.sin(ang) + d.y * math.cos(ang), 0))
                twig(base, nd, length * (0.42 - 0.3 * t) * rng.uniform(0.85, 1.1), thick * 0.5, depth - 1)
    twig(Vector((0.0, 0, 0)), Vector((1, 0, 0)), L * 0.98, 0.012, 1)
    ob = new_obj('FirBranch', bm)
    me = ob.data; ca = color_attr(me)
    # faces order matches dict insertion
    bm2 = bmesh.new(); bm2.from_mesh(me)
    return ob, cols

def make_fir_card():
    reset()
    rng = random.Random(4)
    bm = bmesh.new(); colmap = []
    dark = (0.028, 0.062, 0.032); mid = (0.06, 0.12, 0.05); light = (0.12, 0.19, 0.08); stemc = (0.14, 0.09, 0.05)
    def tri(a, b, c, col):
        vs = [bm.verts.new(v) for v in (a, b, c)]; bm.faces.new(vs); colmap.append(col)
    def needle(base, d, ln, w, col):
        d = d.normalized(); side = Vector((-d.y, d.x, 0)).normalized() if abs(d.z) < 0.99 else Vector((1, 0, 0))
        up = Vector((0, 0, 1)); tip = base + d * ln
        a = base + side * w; b = base - side * w; c = base + up * w * 1.2
        tri(a, b, tip, col); tri(b, c, tip, col); tri(c, a, tip, col)
    def twig(start, d, length, thick, depth):
        d = d.normalized(); steps = 14
        for s in range(steps):
            t0 = s / steps; p = start + d * length * t0
            needle(p, d, length / steps * 1.1, thick * (1 - 0.6 * t0), stemc)
            n = int((12 if depth == 0 else 9) * (1 - 0.3 * t0))
            for k in range(n):
                tt = t0 + rng.random() / steps; q = start + d * length * tt
                side = 1 if k % 2 else -1
                perp = Vector((-d.y, d.x, 0)) * side
                nd = perp * rng.uniform(0.5, 1.0) + d * rng.uniform(0.35, 0.9) + Vector((0, 0, rng.uniform(-0.25, 0.5)))
                ln = rng.uniform(0.055, 0.085) * (1 - 0.35 * tt) * (0.8 if depth == 2 else 1)
                f = rng.random()
                c = tuple(lerp(lerp(dark[i], mid[i], f), light[i], (tt ** 1.6) * 0.75 + 0.2 * rng.random()) for i in range(3))
                needle(q, nd, ln, 0.0034, c)
        if depth > 0:
            nk = 12 if depth == 2 else 6
            for k in range(nk):
                t = 0.06 + k * (0.86 / nk)
                side = 1 if k % 2 else -1
                base = start + d * length * t
                ang = math.radians(rng.uniform(38, 58)) * side
                nd = Vector((d.x * math.cos(ang) - d.y * math.sin(ang), d.x * math.sin(ang) + d.y * math.cos(ang), 0))
                ln = length * ((0.29 - 0.22 * t) if depth == 2 else (0.42 - 0.25 * t)) * rng.uniform(0.85, 1.1)
                twig(base, nd, ln, thick * 0.55, depth - 1)
    twig(Vector((0.01, 0, 0)), Vector((1, 0, 0)), 0.97, 0.012, 2)
    ob = new_obj('Fir', bm)
    ca = color_attr(ob.data)
    for p, c in zip(ob.data.polygons, colmap):
        for li in p.loop_indices: ca.data[li].color = (*c, 1)
    print('fir card tris', len(colmap))
    alb = emission_mat('alb', 'albedo'); nor = emission_mat('nor', 'normal')
    ob['alb_mat'] = 'alb'; ob['nor_mat'] = 'nor'
    render_card(f'{CARDS}/fir', 1.0, 0.5)

def make_leaf_card():
    reset()
    crops = json.load(open(f'{PH}/crops.json'))['leaves']
    img = bpy.data.images.load(f'{PH}/tex/searsia_lucida_Diffuse.png')
    aimg = bpy.data.images.load(f'{PH}/tex/searsia_lucida_Alpha.png')
    rng = random.Random(7)
    bm = bmesh.new(); uvs = []; colmap = []
    stemc = (0.2, 0.14, 0.08)
    def quad(center, d, ln, w, crop, col, lift=0.0):
        d = d.normalized(); side = Vector((-d.y, d.x, 0)).normalized()
        x0, y0, x1, y1 = crop
        u0, u1 = x0 / 1024, x1 / 1024; v0, v1 = 1 - y1 / 1024, 1 - y0 / 1024
        # leaf image points up (stem at bottom) → map leaf base at 'center'
        p0 = center - side * w / 2; p1 = center + side * w / 2
        p2 = p1 + d * ln + Vector((0, 0, lift)); p3 = p0 + d * ln + Vector((0, 0, lift))
        vs = [bm.verts.new(v) for v in (p0, p1, p2, p3)]
        bm.faces.new(vs)
        uvs.append([(u0, v0), (u1, v0), (u1, v1), (u0, v1)]); colmap.append(col)
    # twigs (thin strips, use crop of a branch zone as brown)
    def twig(start, d, length, depth):
        d = d.normalized(); seg = 6
        for s in range(seg):
            t = s / seg; p = start + d * length * t
            quad(p, d, length / seg * 1.05, 0.012 * (1 - t * 0.6), (60, 470, 70, 480), stemc)
            for k in range(2):
                q = p + d * length / seg * rng.random()
                side = rng.choice((-1, 1))
                ang = math.radians(rng.uniform(25, 70)) * side
                nd = Vector((d.x * math.cos(ang) - d.y * math.sin(ang), d.x * math.sin(ang) + d.y * math.cos(ang), 0))
                sz = rng.uniform(0.08, 0.13) * (1.1 - 0.4 * t)
                cr = rng.choice(crops)
                tint = rng.uniform(0.75, 1.1)
                quad(q, nd, sz, sz * (cr[2] - cr[0]) / (cr[3] - cr[1]), cr, (tint, tint * rng.uniform(0.95, 1.05), tint * 0.9), lift=rng.uniform(0, 0.03))
        if depth:
            for k in range(4):
                t = 0.2 + 0.18 * k; side = 1 if k % 2 else -1
                ang = math.radians(rng.uniform(30, 55)) * side
                nd = Vector((d.x * math.cos(ang) - d.y * math.sin(ang), d.x * math.sin(ang) + d.y * math.cos(ang), 0))
                twig(start + d * length * t, nd, length * (0.5 - 0.3 * t), depth - 1)
    twig(Vector((0.02, 0, 0)), Vector((1, 0, 0)), 0.93, 1)
    ob = new_obj('Leaf', bm)
    me = ob.data
    uvl = me.uv_layers.new(name='UVMap')
    for p, q in zip(me.polygons, uvs):
        for li, uv in zip(p.loop_indices, q): uvl.data[li].uv = uv
    ca = color_attr(me)
    for p, c in zip(me.polygons, colmap):
        for li in p.loop_indices: ca.data[li].color = (*c, 1)
    alb = emission_mat('alb', 'albedo', image=img, alpha_img=aimg)
    nor = emission_mat('nor', 'normal', alpha_img=aimg)
    ob['alb_mat'] = 'alb'; ob['nor_mat'] = 'nor'
    render_card(f'{CARDS}/leaf', 1.0, 0.5)

# =====================================================================================
# 2. building game assets
# =====================================================================================
def pbr_mat(name, diff, nor=None, rough_img=None, alpha=None, rough=0.85, clip=False, double=False):
    m = bpy.data.materials.new(name); m.use_nodes = True
    nt = m.node_tree; N = nt.nodes; L = nt.links
    b = N['Principled BSDF']
    t = N.new('ShaderNodeTexImage'); t.image = diff
    vc = N.new('ShaderNodeVertexColor'); vc.layer_name = 'Col'
    mul = N.new('ShaderNodeMix'); mul.data_type = 'RGBA'; mul.blend_type = 'MULTIPLY'; mul.inputs['Factor'].default_value = 1
    L.new(t.outputs[0], mul.inputs[6]); L.new(vc.outputs[0], mul.inputs[7]); L.new(mul.outputs[2], b.inputs['Base Color'])
    if alpha is not None:
        if alpha == 'diffuse':
            L.new(t.outputs['Alpha'], b.inputs['Alpha'])
        m.blend_method = 'CLIP'
    if nor is not None:
        nt_ = N.new('ShaderNodeTexImage'); nt_.image = nor; nor.colorspace_settings.name = 'Non-Color'
        nm = N.new('ShaderNodeNormalMap'); L.new(nt_.outputs[0], nm.inputs['Color']); L.new(nm.outputs[0], b.inputs['Normal'])
    b.inputs['Roughness'].default_value = rough
    m.use_backface_culling = not double
    return m

def load_img(path, size=512, name=None):
    im = bpy.data.images.load(path); im.name = name or os.path.basename(path)
    if im.size[0] > size: im.scale(size, size * im.size[1] // im.size[0])
    return im

def set_cols(ob, fn):
    me = ob.data; ca = color_attr(me)
    for p in me.polygons:
        for li, vi in zip(p.loop_indices, p.vertices):
            co = me.vertices[vi].co
            ca.data[li].color = (*fn(co, p), 1)

def lathe(bm, path, radii, segs, uv_layer, v_scale, uv_u=1.0):
    """tube along path points with radii; cylindrical UVs"""
    rings = []
    acc = 0.0
    for i, (p, r) in enumerate(zip(path, radii)):
        if i > 0: acc += (path[i] - path[i - 1]).length
        d = (path[min(i + 1, len(path) - 1)] - path[max(i - 1, 0)]).normalized()
        side = d.cross(Vector((0, 1, 0)) if abs(d.y) < 0.9 else Vector((1, 0, 0))).normalized(); up = d.cross(side).normalized()
        ring = []
        for k in range(segs + 1):
            a = 2 * math.pi * k / segs
            ring.append((bm.verts.new(p + (side * math.cos(a) + up * math.sin(a)) * r), k / segs * uv_u, acc * v_scale))
        rings.append(ring)
    faces = []
    for A, B in zip(rings, rings[1:]):
        for k in range(segs):
            f = bm.faces.new((A[k][0], A[k + 1][0], B[k + 1][0], B[k][0]))
            faces.append((f, [(A[k][1], A[k][2]), (A[k + 1][1], A[k + 1][2]), (B[k + 1][1], B[k + 1][2]), (B[k][1], B[k][2])]))
    return faces

def card(bm, root, d, L, W, droop, roll, uvs, flip=False, segs=3):
    """bent branch card from root along d; texture u along branch, v across"""
    d = d.normalized()
    side = d.cross(Vector((0, 0, 1)))
    if side.length < 1e-3: side = Vector((1, 0, 0))
    side.normalize()
    up = side.cross(d).normalized()
    # roll around branch axis
    R = Matrix.Rotation(roll, 3, d)
    side = R @ side; up = R @ up
    cols = []
    for s in range(segs + 1):
        t = s / segs
        c = root + d * L * t + Vector((0, 0, -droop * t * t))
        cols.append((bm.verts.new(c + side * W / 2), bm.verts.new(c - side * W / 2), t))
    out = []
    for (a1, a2, t0), (b1, b2, t1) in zip(cols, cols[1:]):
        f = bm.faces.new((a2, b2, b1, a1))
        out.append((f, [(t0, 0), (t1, 0), (t1, 1), (t0, 1)]))
    return out

def build_pine(name, seed, height=10.5, bark=None, fol=None):
    rng = random.Random(seed)
    bm = bmesh.new(); uvdata = []; matidx = []
    # trunk
    path = [Vector((rng.uniform(-0.04, 0.04) * (i / 8), rng.uniform(-0.04, 0.04) * (i / 8), height * 0.96 * i / 8)) for i in range(9)]
    radii = [lerp(0.3, 0.035, (i / 8) ** 0.9) for i in range(9)]
    radii[0] = 0.38
    for f, uv in lathe(bm, path, radii, 9, None, 0.35, 2.0):
        uvdata.append(uv); matidx.append(0)
    # whorls
    nW = 15
    for w in range(nW):
        t = w / (nW - 1)
        z = lerp(1.9, height * 0.97, t ** 0.92)
        L = lerp(3.3, 0.55, t ** 0.85) * rng.uniform(0.85, 1.12)
        nb = 7 if t < 0.7 else 5
        az0 = rng.random() * 6.28
        for b in range(nb):
            az = az0 + b * 2 * math.pi / nb + rng.uniform(-0.25, 0.25)
            pitch = math.radians(lerp(-8, 18, t) + rng.uniform(-6, 6))
            d = Vector((math.cos(az) * math.cos(pitch), math.sin(az) * math.cos(pitch), math.sin(pitch)))
            root = Vector((0, 0, z)) + d * 0.06
            for k, roll in enumerate((rng.uniform(-0.25, 0.25), rng.uniform(0.9, 1.2) * rng.choice((-1, 1)))):
                LL = L * (1.0 if k == 0 else 0.82)
                for f, uv in card(bm, root, d, LL, LL * 0.64, LL * lerp(0.32, 0.12, t), roll, None):
                    uvdata.append(uv); matidx.append(1)
    # crown tip
    for k in range(3):
        az = k * 2.1
        d = Vector((math.cos(az) * 0.25, math.sin(az) * 0.25, 1))
        for f, uv in card(bm, Vector((0, 0, height * 0.9)), d, 1.3, 0.6, 0.0, k * 1.05, None, segs=1):
            uvdata.append(uv); matidx.append(1)
    ob = new_obj(name, bm)
    me = ob.data
    uvl = me.uv_layers.new(name='UVMap')
    for p, uv, mi in zip(me.polygons, uvdata, matidx):
        p.material_index = mi
        for li, c in zip(p.loop_indices, uv): uvl.data[li].uv = c
    me.materials.append(bark); me.materials.append(fol)
    # vertex colour = baked ambient occlusion-ish: darker near trunk & low, lighter at tips/top
    def colfn(co, p):
        if p.material_index == 0:
            g = lerp(0.55, 1.0, min(1, co.z / 3)); return (g, g, g)
        r = Vector((co.x, co.y)).length
        ao = 0.42 + 0.58 * min(1, r / 2.4) ** 0.8
        ao *= lerp(0.8, 1.1, co.z / height)
        hue = noise.noise(co * 0.7)
        return (ao * (1 + 0.08 * hue), ao * (1 + 0.04 * hue), ao * 0.95)
    set_cols(ob, colfn)
    # bent normals: foliage normals point away from the crown axis (soft volumetric lighting)
    me.calc_loop_triangles() if hasattr(me, 'calc_loop_triangles') else None
    normals = []
    for l in me.loops:
        v = me.vertices[l.vertex_index].co
        p = me.polygons[0]
        normals.append(None)
    lnor = [Vector((0, 0, 1))] * len(me.loops)
    for p in me.polygons:
        for li in p.loop_indices:
            v = me.vertices[me.loops[li].vertex_index].co
            if p.material_index == 1:
                axis = Vector((0, 0, v.z - 0.6))
                n = (v - axis).normalized()
                n = (n * 0.8 + Vector((0, 0, 0.45))).normalized()
            else:
                n = Vector((v.x, v.y, 0)).normalized() if Vector((v.x, v.y)).length > 1e-4 else p.normal
            lnor[li] = n
    me.normals_split_custom_set(lnor)
    return ob

def build_cypress(name, seed, bark, fol):
    rng = random.Random(seed)
    bm = bmesh.new(); uvdata = []; matidx = []
    # windswept trunk + three main limbs
    def limb(start, d, length, r0, r1, depth):
        pts = []; rad = []
        p = start.copy(); dd = d.normalized()
        for i in range(7):
            pts.append(p.copy()); rad.append(lerp(r0, r1, i / 6))
            dd = (dd + Vector((rng.uniform(-0.18, 0.18), rng.uniform(-0.18, 0.18), rng.uniform(-0.05, 0.12)))).normalized()
            p = p + dd * length / 6
        for f, uv in lathe(bm, pts, rad, 7, None, 0.5, 1.0): uvdata.append(uv); matidx.append(0)
        end = pts[-1]
        if depth:
            for k in range(rng.choice((2, 3))):
                nd = (dd + Vector((rng.uniform(-1, 1), rng.uniform(-1, 1), rng.uniform(0.0, 0.6)))).normalized()
                nd.z = max(nd.z, 0.15)
                limb(end, nd, length * rng.uniform(0.55, 0.75), r1, r1 * 0.5, depth - 1)
        else:
            clumps.append(end)
    clumps = []
    limb(Vector((0, 0, -0.2)), Vector((0.38, 0.05, 1)), 2.5, 0.42, 0.26, 2)
    # foliage clumps: flattened domes of leaf cards (Monterey-cypress shelf look)
    for c in clumps:
        R = rng.uniform(1.9, 2.6)
        for k in range(12):
            az = rng.random() * 6.28; el = rng.uniform(-0.15, 0.55)
            d = Vector((math.cos(az) * math.cos(el), math.sin(az) * math.cos(el), math.sin(el)))
            root = c + Vector((rng.uniform(-0.3, 0.3), rng.uniform(-0.3, 0.3), rng.uniform(-0.1, 0.4))) - d * R * 0.35
            for f, uv in card(bm, root, d, R * 1.25, R * 0.75, R * 0.18, rng.uniform(-0.6, 0.6), None, segs=2):
                uvdata.append(uv); matidx.append(1)
    ob = new_obj(name, bm); me = ob.data
    uvl = me.uv_layers.new(name='UVMap')
    for p, uv, mi in zip(me.polygons, uvdata, matidx):
        p.material_index = mi
        for li, c in zip(p.loop_indices, uv): uvl.data[li].uv = c
    me.materials.append(bark); me.materials.append(fol)
    cen = Vector((0, 0, 0)); n = 0
    for c in clumps: cen += c; n += 1
    cen /= max(n, 1)
    def colfn(co, p):
        if p.material_index == 0: return (0.8, 0.8, 0.8)
        dd = (co - cen); ao = 0.5 + 0.5 * min(1, dd.length / 3.0); ao *= lerp(0.75, 1.1, min(1, max(0, (co.z - cen.z + 1.5) / 3)))
        return (ao, ao, ao * 0.95)
    set_cols(ob, colfn)
    lnor = [None] * len(me.loops)
    for p in me.polygons:
        for li in p.loop_indices:
            v = me.vertices[me.loops[li].vertex_index].co
            if p.material_index == 1: n_ = ((v - cen).normalized() + Vector((0, 0, 0.6))).normalized()
            else: n_ = p.normal.copy()
            lnor[li] = n_
    me.normals_split_custom_set(lnor)
    return ob

def import_scan(name, objs=None, target_tris=800, scale_to=None, rename=None, keep_mat=True):
    before = set(bpy.data.objects)
    bpy.ops.import_scene.gltf(filepath=f'{PH}/mdl/{name}/{name}.gltf')
    new = [o for o in bpy.data.objects if o not in before and o.type == 'MESH']
    if objs: new = [o for o in new if any(s in o.name for s in objs)]
    for o in [o for o in bpy.data.objects if o not in before and o not in new]:
        bpy.data.objects.remove(o, do_unlink=True)
    bpy.ops.object.select_all(action='DESELECT')
    for o in new: o.select_set(True)
    bpy.context.view_layer.objects.active = new[0]
    bpy.ops.object.parent_clear(type='CLEAR_KEEP_TRANSFORM')
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    if len(new) > 1: bpy.ops.object.join()
    ob = bpy.context.active_object
    tris = sum(len(p.vertices) - 2 for p in ob.data.polygons)
    if tris > target_tris:
        dm = ob.modifiers.new('d', 'DECIMATE'); dm.ratio = target_tris / tris; dm.use_collapse_triangulate = True
        bpy.ops.object.modifier_apply(modifier='d')
    # ground the asset: origin at bottom centre
    mn = Vector((1e9,) * 3); mx = Vector((-1e9,) * 3)
    for v in ob.data.vertices: mn = Vector(map(min, mn, v.co)); mx = Vector(map(max, mx, v.co))
    off = Vector(((mn.x + mx.x) / 2, (mn.y + mx.y) / 2, mn.z))
    for v in ob.data.vertices: v.co -= off
    if scale_to:
        s = scale_to / max(mx.x - mn.x, mx.y - mn.y, mx.z - mn.z)
        for v in ob.data.vertices: v.co *= s
    ob.location = (0, 0, 0)
    ob.name = rename or name
    for p in ob.data.polygons: p.use_smooth = True
    for m in ob.data.materials:
        for n in m.node_tree.nodes:
            if n.type == 'TEX_IMAGE' and n.image and n.image.size[0] > 512: n.image.scale(512, 512)
    return ob

def build_grass(name, im, aim, crops, seed):
    rng = random.Random(seed)
    bm = bmesh.new(); uvdata = []
    for k in range(3):
        cr = crops[k % len(crops)]
        x0, y0, x1, y1 = cr; u0, u1 = x0 / 1024, x1 / 1024; v0, v1 = 1 - y1 / 1024, 1 - y0 / 1024
        w = 0.95 * (x1 - x0) / (y1 - y0) * 0.6; h = 0.62 * rng.uniform(0.85, 1.15)
        a = k * math.pi / 3 + rng.uniform(-0.2, 0.2)
        dx, dy = math.cos(a) * w / 2, math.sin(a) * w / 2
        ox, oy = rng.uniform(-0.08, 0.08), rng.uniform(-0.08, 0.08)
        vs = [bm.verts.new(v) for v in ((ox - dx, oy - dy, -0.02), (ox + dx, oy + dy, -0.02), (ox + dx, oy + dy, h), (ox - dx, oy - dy, h))]
        bm.faces.new(vs); uvdata.append([(u0, v0), (u1, v0), (u1, v1), (u0, v1)])
    ob = new_obj(name, bm); me = ob.data
    uvl = me.uv_layers.new(name='UVMap')
    for p, uv in zip(me.polygons, uvdata):
        for li, c in zip(p.loop_indices, uv): uvl.data[li].uv = c
    set_cols(ob, lambda co, p: tuple([lerp(0.45, 1.0, min(1, co.z / 0.5))] * 3))
    lnor = []
    for l in me.loops: lnor.append(Vector((0, 0, 1)))  # grass lit like the ground → no dark card sides
    me.normals_split_custom_set(lnor)
    return ob

def build_delineator(name):
    bm = bmesh.new()
    bmesh.ops.create_cube(bm, size=1)
    for v in bm.verts: v.co.x *= 0.1; v.co.y *= 0.04; v.co.z = (v.co.z + 0.5) * 1.12
    bmesh.ops.bevel(bm, geom=[e for e in bm.edges], offset=0.008, segments=2, affect='EDGES')
    ob = new_obj(name, bm)
    m = bpy.data.materials.new('PostMat'); m.use_nodes = True
    m.node_tree.nodes['Principled BSDF'].inputs['Base Color'].default_value = (0.78, 0.76, 0.72, 1)
    m.node_tree.nodes['Principled BSDF'].inputs['Roughness'].default_value = 0.45
    ob.data.materials.append(m)
    set_cols(ob, lambda co, p: (0.55, 0.55, 0.55) if co.z < 0.12 else (1, 1, 1))
    for p in ob.data.polygons: p.use_smooth = False
    return ob

def dilate_note(): pass

if phase == 'cards':
    make_fir_card(); make_leaf_card()
    print('CARDS DONE')
elif phase == 'build':
    reset()
    crops = json.load(open(f'{PH}/crops.json'))
    bark = load_img(f'{PH}/tex/pine_bark_Diffuse.jpg', 512, 'pine_bark_diff')
    barkN = load_img(f'{PH}/tex/pine_bark_nor_gl.jpg', 512, 'pine_bark_nor')
    fir = load_img(f'{CARDS}/fir_albedo_d.png', 512, 'fir_card'); firN = load_img(f'{CARDS}/fir_normal_d.png', 512, 'fir_card_nor')
    leaf = load_img(f'{CARDS}/leaf_albedo_d.png', 512, 'leaf_card'); leafN = load_img(f'{CARDS}/leaf_normal_d.png', 512, 'leaf_card_nor')
    grassI = load_img(f'{CARDS}/grass_rgba.png', 512, 'grass_card')
    M_bark = pbr_mat('Bark', bark, barkN, rough=0.9)
    M_fir = pbr_mat('FirFoliage', fir, firN, alpha='diffuse', rough=0.8, double=True)
    M_leaf = pbr_mat('LeafFoliage', leaf, leafN, alpha='diffuse', rough=0.7, double=True)
    M_grass = pbr_mat('Grass', grassI, None, alpha='diffuse', rough=0.9, double=True)
    objs = []
    objs.append(build_pine('Pine', 3, 10.5, M_bark, M_fir))
    objs.append(build_pine('Pine2', 11, 8.2, M_bark, M_fir))
    objs.append(build_cypress('Cypress', 5, M_bark, M_leaf))
    objs.append(import_scan('quiver_tree_02', target_tris=3200, scale_to=5.2, rename='Joshua'))
    objs.append(import_scan('namaqualand_boulder_02', target_tris=900, scale_to=2.4, rename='Rock'))
    objs.append(import_scan('rock_moss_set_01', objs=['rock01'], target_tris=700, scale_to=2.2, rename='Rock2'))
    objs.append(import_scan('rock_07', target_tris=600, scale_to=1.8, rename='Rock3'))
    objs.append(import_scan('shrub_03', objs=['shrub_03_a', 'shrub_03_b'], target_tris=2400, scale_to=1.3, rename='Shrub'))
    g = build_grass('Grass', None, None, crops['grass_card'], 9); g.data.materials.append(M_grass); objs.append(g)
    objs.append(build_delineator('Delineator'))
    # vertex colours for scans (white = untouched) so the game's shader path is uniform
    for o in objs:
        if not o.data.color_attributes:
            set_cols(o, lambda co, p: (1, 1, 1))
    for o in objs: print('ASSET', o.name, sum(len(p.vertices) - 2 for p in o.data.polygons), [m.name for m in o.data.materials])
    if '--preview' in sys.argv:
        sc = bpy.context.scene
        for i, o in enumerate(objs): o.location = ((i % 5) * 7 - 14, (i // 5) * 10, 0)
        bpy.ops.mesh.primitive_plane_add(size=200); gp = bpy.context.active_object
        gm = bpy.data.materials.new('G'); gm.use_nodes = True; gm.node_tree.nodes['Principled BSDF'].inputs['Base Color'].default_value = (0.25, 0.2, 0.15, 1); gp.data.materials.append(gm)
        sc.render.engine = 'CYCLES'; sc.cycles.samples = 32; sc.cycles.use_denoising = True
        sc.render.resolution_x = 1400; sc.render.resolution_y = 700
        w = bpy.data.worlds.new('W'); sc.world = w; w.use_nodes = True
        sky = w.node_tree.nodes.new('ShaderNodeTexSky'); sky.sky_type = 'NISHITA'; sky.sun_elevation = math.radians(30); sky.sun_rotation = math.radians(140)
        w.node_tree.links.new(sky.outputs[0], w.node_tree.nodes['Background'].inputs[0]); w.node_tree.nodes['Background'].inputs[1].default_value = 0.4
        sc.view_settings.view_transform = 'AgX'
        cd = bpy.data.cameras.new('c'); cam = bpy.data.objects.new('c', cd); sc.collection.objects.link(cam); sc.camera = cam
        cam.location = (0, -30, 7); cam.rotation_euler = (Vector((0, 6, 3.5)) - cam.location).to_track_quat('-Z', 'Y').to_euler(); cd.lens = 30
        sc.render.filepath = OUT + '_preview.png'; bpy.ops.render.render(write_still=True)
        bpy.data.objects.remove(gp, do_unlink=True); bpy.data.objects.remove(cam, do_unlink=True)
        for o in objs: o.location = (0, 0, 0)
    bpy.ops.object.select_all(action='DESELECT')
    for o in objs: o.select_set(True)
    bpy.ops.export_scene.gltf(filepath=OUT + '.glb', export_format='GLB', use_selection=True, export_yup=True,
                              export_normals=True, export_texcoords=True, export_materials='EXPORT',
                              export_vertex_color='ACTIVE', export_image_format='WEBP', export_image_quality=82,
                              export_apply=True)
    print('EXPORTED', os.path.getsize(OUT + '.glb'))
