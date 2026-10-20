# The giant 2-in-1 mirror on the right wall (docs/BAR_REDESIGN.md): a slim
# black steel frame with a brass bead, from just above the floor to near
# the ceiling, a vanity light bar on top, and its control panel (a brass
# plate printed FULL / AVATARS / OFF). The three buttons are a separate
# model (mirror_button.glb) so each can light up on its own.
#
#   blender -b --factory-startup --python tools/assets/big_mirror.py -- <texture dir> <out dir>
#
# Model space = world space. Matches buildMirror() in bar/bar_scene.cpp.
import os
import sys

import numpy as np
from mathutils import Vector

sys.path.insert(0, os.path.dirname(__file__))
import common  # noqa: E402
import layout as L  # noqa: E402

tex_dir, out_dir = sys.argv[sys.argv.index("--") + 1:][:2]

ZC, HALF_W, Y0, Y1 = 3.6, 2.2, 0.14, 3.62
PANEL_Z, PANEL_Y = ZC - HALF_W - 0.42, 1.3
BUTTON_YS = (PANEL_Y + 0.09, PANEL_Y, PANEL_Y - 0.09)


def build_frame():
    common.reset_scene()
    steel = common.plain_material("Frame steel", (0.025, 0.025, 0.028), 0.7, 0.35)
    brass = common.plain_material("Frame brass", (0.91, 0.70, 0.38), 1.0, 0.25)
    backing = common.plain_material("Mirror backing", (0.02, 0.02, 0.02), 0.0, 0.8)
    bulb = common.plain_material("Vanity bulb", (1.0, 0.9, 0.75), 0.0, 0.3, emission=(1.0, 0.75, 0.45), strength=6.0)
    X = L.X1
    fw, fd = 0.08, 0.07  # frame width and depth
    z0, z1 = ZC - HALF_W, ZC + HALF_W
    common.box("Backing", (X - 0.025, Y0 - 0.02, z0 - 0.02), (X, Y1 + 0.02, z1 + 0.02), backing, 0.0)
    for (lo, hi) in (((X - fd, Y0 - fw, z0 - fw), (X, Y0, z1 + fw)), ((X - fd, Y1, z0 - fw), (X, Y1 + fw, z1 + fw)),
                     ((X - fd, Y0, z0 - fw), (X, Y1, z0)), ((X - fd, Y0, z1), (X, Y1, z1 + fw))):
        o = common.box("Frame", lo, hi, steel, 0.01, 3)
        o["grain_axis"] = "z"
    for (lo, hi) in (((X - fd - 0.006, Y0 - 0.012, z0 - 0.012), (X - 0.03, Y0, z1 + 0.012)),
                     ((X - fd - 0.006, Y1, z0 - 0.012), (X - 0.03, Y1 + 0.012, z1 + 0.012)),
                     ((X - fd - 0.006, Y0, z0 - 0.012), (X - 0.03, Y1, z0)),
                     ((X - fd - 0.006, Y0, z1), (X - 0.03, Y1, z1 + 0.012))):
        common.box("Bead", lo, hi, brass, 0.004)
    # Vanity light bar above the mirror, with round bulbs.
    common.box("Light bar", (X - 0.12, Y1 + fw + 0.02, z0 + 0.2), (X - 0.02, Y1 + fw + 0.1, z1 - 0.2), brass, 0.02, 3)
    for k in range(12):
        z = z0 + 0.35 + k * (z1 - z0 - 0.7) / 11
        common.sphere("Bulb", Vector((X - 0.15, Y1 + fw + 0.06, z)), 0.035, bulb, 12, 8)
        common.cylinder("Bulb socket", Vector((X - 0.115, Y1 + fw + 0.06, z)), 0.02, 0.03, "x", brass, segments=10)
    # Control panel plate with the labels.
    px = np.zeros((96, 192, 3), np.float32)
    px[:] = common.srgb((0.62, 0.47, 0.22))
    for i, word in enumerate(("FULL", "AVATARS", "OFF")):
        common.draw_text(px, word, 120, 86 - i * 30, 2, common.srgb((0.08, 0.06, 0.03)), center=True)
    common.draw_text(px, "MIRROR", 40, 92, 1, common.srgb((0.12, 0.09, 0.05)), center=True)
    label = common.image_material("Mirror panel", px, metallic=0.0, roughness=0.35)
    common.box("Panel", (X - 0.012, PANEL_Y - 0.15, PANEL_Z - 0.17), (X, PANEL_Y + 0.15, PANEL_Z + 0.17), brass, 0.006)
    common.panel("Panel label", (X - 0.0125, PANEL_Y, PANEL_Z), (0, 0, 1), (0, 1, 0), 0.32, 0.27, label)
    common.finish_and_export(os.path.join(out_dir, "big_mirror.glb"), uv_scale={})


def build_button():
    """One push button, centered at the origin, facing -x."""
    common.reset_scene()
    cap = common.plain_material("Button cap", (0.95, 0.85, 0.6), 0.0, 0.3)
    ring = common.plain_material("Button ring", (0.91, 0.70, 0.38), 1.0, 0.25)
    common.cylinder("Button ring", Vector((-0.008, 0, 0)), 0.03, 0.016, "x", ring, segments=20)
    common.cylinder("Button cap", Vector((-0.018, 0, 0)), 0.022, 0.012, "x", cap, segments=20)
    common.finish_and_export(os.path.join(out_dir, "mirror_button.glb"), uv_scale={})


build_frame()
build_button()
