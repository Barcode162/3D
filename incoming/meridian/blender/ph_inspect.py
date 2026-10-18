import bpy, sys, math, os
from mathutils import Vector
names = sys.argv[sys.argv.index('--') + 1:]
for n in names:
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=f'/tmp/ph/mdl/{n}/{n}.gltf')
    obs = [o for o in bpy.context.scene.objects if o.type == 'MESH']
    tris = sum(sum(len(p.vertices) - 2 for p in o.data.polygons) for o in obs)
    mn = Vector((1e9,)*3); mx = Vector((-1e9,)*3)
    for o in obs:
        for c in o.bound_box:
            w = o.matrix_world @ Vector(c); mn = Vector(map(min, mn, w)); mx = Vector(map(max, mx, w))
    print('INFO', n, 'objs', len(obs), [o.name for o in obs][:8], 'tris', tris, 'size', [round(v,2) for v in (mx-mn)], 'mats', [m.name for m in bpy.data.materials])
