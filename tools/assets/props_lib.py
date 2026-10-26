# Small props shared by the zone scripts (the big bar upgrade): bottles,
# glassware with drinks, bar tools, citrus, books, picture frames, plants,
# candles. Every builder makes objects at a spot, turned by `yaw` around +y
# (the engine's turnY sense). Materials are made once per Blender run and
# shared through atlases, so hundreds of props stay a handful of draw calls.
#
# Usage inside an asset script:
#   import props_lib
#   P = props_lib.Props(tex_dir)
#   P.bottle(x, y, z, "whiskey", yaw=0.3)
import math
import os
import random

import bmesh
import numpy as np
from mathutils import Matrix, Vector

import common


def _place(obj, x, y, z, yaw=0.0, tilt=None):
    m = Matrix.Translation((x, y, z)) @ Matrix.Rotation(yaw, 4, "Y")
    if tilt is not None:
        m = m @ tilt
    obj.data.transform(m)
    return obj


GLASS_COLORS = [(0.42, 0.17, 0.03), (0.05, 0.20, 0.06), (0.55, 0.60, 0.58), (0.30, 0.03, 0.05),
                (0.62, 0.36, 0.06), (0.04, 0.09, 0.30), (0.07, 0.05, 0.04), (0.02, 0.28, 0.18),
                (0.75, 0.75, 0.72), (0.25, 0.12, 0.02), (0.50, 0.08, 0.30), (0.10, 0.35, 0.40),
                (0.80, 0.55, 0.10), (0.20, 0.20, 0.22), (0.60, 0.15, 0.05), (0.35, 0.45, 0.10)]
LIQUIDS = {"whiskey": (0.55, 0.22, 0.03), "beer": (0.85, 0.55, 0.08), "red_wine": (0.25, 0.01, 0.03),
           "white_wine": (0.85, 0.78, 0.40), "pink": (0.95, 0.30, 0.45), "blue": (0.10, 0.45, 0.85),
           "green": (0.35, 0.75, 0.20), "cola": (0.12, 0.04, 0.02), "orange": (0.95, 0.45, 0.05),
           "water": (0.75, 0.85, 0.90), "foam": (0.95, 0.92, 0.85), "coffee": (0.20, 0.10, 0.05),
           "cream": (0.92, 0.85, 0.70), "red": (0.75, 0.05, 0.05), "ice": (0.88, 0.93, 0.95),
           "lime": (0.45, 0.75, 0.15)}
_LIQ_KEYS = list(LIQUIDS)


class Props:
    def __init__(self, tex_dir, seed=11):
        self.tex_dir = tex_dir
        self.rng = random.Random(seed)
        self._m = {}

    # ---- Materials (made on first use) ----
    def mat(self, key):
        if key in self._m:
            return self._m[key]
        c = common
        m = None
        if key == "bottle_glass":
            # See-through tinted glass (J35.4): the drink inside is its own
            # solid shape, the colour shows through.
            sw = np.zeros((16, 16, 3), np.float32)
            for i, col in enumerate(GLASS_COLORS):
                sw[:, i] = col
            m = c.image_material("Bottle glass", np.repeat(np.repeat(sw, 4, 0), 4, 1), roughness=0.05)
            m.node_tree.nodes["Principled BSDF"].inputs["Alpha"].default_value = 0.42
            m.surface_render_method = "BLENDED"
        elif key == "labels":
            # Printed labels of made-up brands (tools/assets/labels.py), or
            # the old drawn ones when that picture hasn't been made.
            path = os.path.join(self.tex_dir or "", "bottle_labels.png")
            if os.path.exists(path):
                _, _, px = c._load_pixels(path)
                m = c.image_material("Bottle labels", px[..., :3], roughness=0.55)
            else:
                m = c.image_material("Bottle labels", _label_atlas(), roughness=0.55)
        elif key == "liquid":
            sw = np.zeros((16, 16, 3), np.float32)
            for i, k in enumerate(_LIQ_KEYS):
                sw[:, i] = LIQUIDS[k]
            m = c.image_material("Drinks", np.repeat(np.repeat(sw, 4, 0), 4, 1), roughness=0.08)
        elif key == "glass":
            m = c.glass_material("Glassware", alpha=0.14)
        elif key == "cap_gold":
            m = c.plain_material("Cap foil", (0.85, 0.65, 0.3), 1.0, 0.35)
        elif key == "cap_dark":
            m = c.plain_material("Cap", (0.05, 0.04, 0.04), 0.3, 0.4)
        elif key == "cork":
            m = c.plain_material("Cork", (0.45, 0.30, 0.17), 0.0, 0.8)
        elif key == "steel":
            m = c.plain_material("Stainless", (0.80, 0.80, 0.82), 1.0, 0.22)
        elif key == "chrome":
            m = c.plain_material("Chrome", (0.9, 0.9, 0.92), 1.0, 0.1)
        elif key == "brass":
            m = c.plain_material("Brass", (0.91, 0.70, 0.38), 1.0, 0.3)
        elif key == "copper":
            m = c.plain_material("Copper", (0.95, 0.55, 0.38), 1.0, 0.28)
        elif key == "black":
            m = c.plain_material("Black plastic", (0.02, 0.02, 0.02), 0.0, 0.45)
        elif key == "white":
            m = c.plain_material("White ceramic", (0.85, 0.83, 0.78), 0.0, 0.25)
        elif key == "wood":
            m = c.pbr_material("Prop wood", self.tex_dir, "wood_table_001", tint=(0.7, 0.55, 0.42), size=256)
        elif key == "dark_wood":
            m = c.pbr_material("Prop dark wood", self.tex_dir, "dark_wood", tint=(0.5, 0.4, 0.36), size=256)
        elif key == "fruit":
            sw = np.zeros((8, 32, 3), np.float32)
            cols = [(0.35, 0.65, 0.08), (0.95, 0.80, 0.10), (0.95, 0.45, 0.05), (0.75, 0.02, 0.05),
                    (0.30, 0.38, 0.10), (0.95, 0.95, 0.80), (0.85, 0.90, 0.60), (0.20, 0.35, 0.08)]
            for i, col in enumerate(cols):
                sw[:, i * 4:(i + 1) * 4] = col
            m = c.image_material("Garnish", np.repeat(np.repeat(sw, 4, 0), 4, 1), roughness=0.35)
        elif key == "books":
            m = c.image_material("Book spines", _book_atlas(), roughness=0.6)
        elif key == "pages":
            m = c.plain_material("Pages", (0.85, 0.80, 0.68), 0.0, 0.8)
        elif key == "photos":
            m = c.image_material("Photos", _photo_atlas(), roughness=0.25)
        elif key == "leaf":
            m = c.plain_material("Leaves", (0.06, 0.20, 0.05), 0.0, 0.5)
        elif key == "leaf2":
            m = c.plain_material("Leaves light", (0.12, 0.30, 0.06), 0.0, 0.55)
        elif key == "soil":
            m = c.plain_material("Soil", (0.06, 0.04, 0.03), 0.0, 0.95)
        elif key == "terracotta":
            m = c.plain_material("Terracotta", (0.55, 0.22, 0.12), 0.0, 0.8)
        elif key == "wax":
            m = c.plain_material("Candle wax", (0.90, 0.85, 0.72), 0.0, 0.5)
        elif key == "flame":
            m = c.plain_material("Flame", (1.0, 0.7, 0.3), 0.0, 0.5, emission=(1.0, 0.55, 0.18), strength=12.0)
        elif key == "rubber":
            m = c.plain_material("Bar mat rubber", (0.03, 0.03, 0.03), 0.0, 0.8)
        elif key == "paper":
            m = c.plain_material("Paper", (0.88, 0.86, 0.80), 0.0, 0.85)
        elif key == "ice":
            m = c.glass_material("Ice", color=(0.9, 0.95, 1.0), alpha=0.45, roughness=0.15)
        else:
            raise KeyError(key)
        self._m[key] = m
        return m

    # ---- Bottles ----
    SHAPES = {
        "whiskey": [(0.0, 0), (0.036, 0), (0.038, 0.006), (0.038, 0.19), (0.032, 0.215), (0.016, 0.235),
                    (0.014, 0.27), (0.0, 0.27)],
        "slim": [(0.0, 0), (0.031, 0), (0.033, 0.006), (0.033, 0.21), (0.024, 0.24), (0.012, 0.26),
                 (0.011, 0.31), (0.0, 0.31)],
        "stubby": [(0.0, 0), (0.042, 0), (0.044, 0.006), (0.044, 0.15), (0.034, 0.18), (0.016, 0.195),
                   (0.015, 0.225), (0.0, 0.225)],
        "liqueur": [(0.0, 0), (0.03, 0), (0.042, 0.03), (0.044, 0.08), (0.036, 0.13), (0.014, 0.16),
                    (0.010, 0.25), (0.0, 0.25)],
        "wine": [(0.0, 0), (0.035, 0), (0.037, 0.006), (0.037, 0.2), (0.03, 0.225), (0.0135, 0.25),
                 (0.0125, 0.30), (0.0, 0.30)],
        "champagne": [(0.0, 0), (0.044, 0), (0.046, 0.008), (0.046, 0.2), (0.036, 0.24), (0.016, 0.275),
                      (0.015, 0.315), (0.0, 0.315)],
        "beer": [(0.0, 0), (0.03, 0), (0.031, 0.005), (0.031, 0.12), (0.025, 0.15), (0.013, 0.18),
                 (0.0125, 0.23), (0.0, 0.23)],
        "bitters": [(0.0, 0), (0.026, 0), (0.027, 0.004), (0.027, 0.1), (0.02, 0.115), (0.008, 0.125),
                    (0.008, 0.15), (0.0, 0.15)],
        "cognac": [(0.0, 0), (0.05, 0), (0.06, 0.03), (0.062, 0.07), (0.05, 0.12), (0.016, 0.15),
                   (0.014, 0.2), (0.0, 0.2)],
        "tequila": [(0.0, 0), (0.04, 0), (0.042, 0.005), (0.04, 0.14), (0.036, 0.17), (0.02, 0.19),
                    (0.018, 0.24), (0.0, 0.24)],
        "magnum": [(0.0, 0), (0.05, 0), (0.052, 0.008), (0.052, 0.26), (0.042, 0.3), (0.018, 0.33),
                   (0.016, 0.39), (0.0, 0.39)],
    }
    LABEL_Y = {"whiskey": (0.05, 0.15), "slim": (0.06, 0.17), "stubby": (0.04, 0.12), "liqueur": (0.045, 0.11),
               "wine": (0.05, 0.14), "champagne": (0.06, 0.15), "beer": (0.04, 0.1), "bitters": (0.02, 0.08),
               "cognac": (0.03, 0.08), "tequila": (0.04, 0.12), "magnum": (0.07, 0.19)}
    KINDS = ["whiskey", "slim", "stubby", "liqueur", "wine", "square", "tequila", "cognac", "whiskey", "slim"]

    def bottle(self, x, y, z, kind=None, color=None, label=None, yaw=0.0, segments=10, fill=None, tilt=None):
        """A bottle standing at (x, y, z); its label faces +z turned by yaw
        (`tilt`, a rotation Matrix, lays it down first). Returns its height."""
        rng = self.rng
        kind = kind or rng.choice(self.KINDS)
        color = rng.randrange(len(GLASS_COLORS)) if color is None else color
        label = rng.randrange(16) if label is None else label
        if kind == "square":
            return self._square_bottle(x, y, z, color, label, yaw)
        prof = self.SHAPES[kind]
        top = prof[-1][1]
        body = common.lathe("Bottle", prof, self.mat("bottle_glass"), segments)
        common.swatch_uvs(body, (color + 0.5) / 16, 0.5)
        _place(body, x, y, z, yaw, tilt)
        drink = fill if fill is not None else self._drink_for(kind)
        if drink:
            # The drink: the body's shape a little inside the glass, filled
            # 55-90% of the way to the shoulder.
            shoulder = next(yy for (r, yy) in prof[2:] if r < max(rr for (rr, _) in prof) - 0.003)
            level = 0.006 + (shoulder - 0.006) * rng.uniform(0.55, 0.9)
            inner = [(0.0, 0.006), (self._radius_at(prof, 0.006) * 0.9, 0.006)] + [(r * 0.9, yy) for (r, yy) in prof[1:] if 0.006 < yy < level] + \
                    [(self._radius_at(prof, level) * 0.9, level), (0.0, level)]
            liq = common.lathe("Drink", inner, self.mat("liquid"), max(6, segments - 2))
            common.swatch_uvs(liq, (_LIQ_KEYS.index(drink) + 0.5) / 16, 0.5)
            _place(liq, x, y, z, yaw, tilt)
        r_body = max(r for (r, _) in prof)
        y0, y1 = self.LABEL_Y[kind]
        self._label_band(r_body + 0.0015, y0, y1, label, x, y, z, yaw, tilt=tilt)
        neck_r = prof[-2][0]
        cap = [self.mat("cap_gold"), self.mat("cap_dark"), self.mat("cork")][label % 3]
        cap_h = 0.035 if label % 3 != 2 else 0.02
        cyl = common.cylinder("Cap", Vector((0, top - cap_h / 2 + 0.008, 0)), neck_r + 0.002, cap_h, "y", cap,
                              segments=8, bevel=0.0)
        _place(cyl, x, y, z, yaw, tilt)
        if kind == "bitters":  # paper-wrapped dasher bottle
            band = self._label_band(r_body + 0.002, 0.005, 0.105, label, x, y, z, yaw, wide=2.9)
        return top + 0.008

    DRINKS = {"whiskey": ["whiskey"], "stubby": ["whiskey", "cola"], "cognac": ["whiskey"], "magnum": ["red_wine"],
              "tequila": ["white_wine", None], "slim": [None, None, "green"], "beer": ["beer"],
              "liqueur": ["red", "green", "orange", "cream", "coffee", "blue"], "wine": ["red_wine", "white_wine"],
              "champagne": ["white_wine"], "bitters": ["coffee"]}

    def _drink_for(self, kind):
        """What's in a bottle of this kind (None: a clear spirit, the glass
        alone shows)."""
        return self.rng.choice(self.DRINKS.get(kind, [None]))

    @staticmethod
    def _radius_at(prof, h):
        for (r0, y0), (r1, y1) in zip(prof, prof[1:]):
            if y0 <= h <= y1 and y1 > y0:
                return r0 + (r1 - r0) * (h - y0) / (y1 - y0)
        return prof[1][0]

    def _square_bottle(self, x, y, z, color, label, yaw):
        s = 0.034
        body = common.box("Square bottle", (-s, 0, -s), (s, 0.17, s), self.mat("bottle_glass"), 0.008, 2)
        common.swatch_uvs(body, (color + 0.5) / 16, 0.5)
        _place(body, x, y, z, yaw)
        liq = common.box("Drink", (-s + 0.004, 0.005, -s + 0.004), (s - 0.004, 0.17 * self.rng.uniform(0.55, 0.9),
                                                                     s - 0.004), self.mat("liquid"), 0.006, 2)
        common.swatch_uvs(liq, (_LIQ_KEYS.index("whiskey") + 0.5) / 16, 0.5)
        _place(liq, x, y, z, yaw)
        sh = common.lathe("Shoulder", [(0.0, 0.17), (0.034, 0.17), (0.026, 0.19), (0.014, 0.205),
                                       (0.0125, 0.25), (0.0, 0.25)], self.mat("bottle_glass"), 12)
        common.swatch_uvs(sh, (color + 0.5) / 16, 0.5)
        _place(sh, x, y, z, yaw)
        self._flat_label(-s + 0.006, s - 0.006, 0.04, 0.13, s + 0.0015, label, x, y, z, yaw)
        cap = common.cylinder("Cap", Vector((0, 0.245, 0)), 0.0145, 0.03, "y", self.mat("cap_dark"), segments=10,
                              bevel=0.001)
        _place(cap, x, y, z, yaw)
        return 0.26

    def _label_band(self, radius, y0, y1, k, x, y, z, yaw, wide=1.0, segments=6, tilt=None):
        half = min(math.radians(55) * wide, math.pi * 0.98)
        bm = bmesh.new()
        cols = []
        for s in range(segments + 1):
            a = -half + 2 * half * s / segments
            p = (radius * math.sin(a), radius * math.cos(a))
            cols.append((bm.verts.new((p[0], y0, p[1])), bm.verts.new((p[0], y1, p[1]))))
        for s in range(segments):
            bm.faces.new([cols[s][0], cols[s + 1][0], cols[s + 1][1], cols[s][1]])
        obj = common.mesh_object("Label", bm, self.mat("labels"), smooth=True)
        r, c = divmod(k % 16, 4)
        u0, v0 = c / 4 + 0.01, r / 4 + 0.01
        common.set_uvs(obj, lambda co, n: (u0 + 0.23 * (0.5 + math.atan2(co.x, co.z) / (2 * half)),
                                           v0 + 0.23 * (co.y - y0) / (y1 - y0)))
        return _place(obj, x, y, z, yaw, tilt)

    def _flat_label(self, x0, x1, y0, y1, zf, k, x, y, z, yaw):
        obj = common.quad("Label", [(x0, y0, zf), (x1, y0, zf), (x1, y1, zf), (x0, y1, zf)], self.mat("labels"))
        r, c = divmod(k % 16, 4)
        u0, v0 = c / 4 + 0.01, r / 4 + 0.01
        common.set_uvs(obj, lambda co, n: (u0 + 0.23 * (co.x - x0) / (x1 - x0), v0 + 0.23 * (co.y - y0) / (y1 - y0)))
        return _place(obj, x, y, z, yaw)

    def bottle_row(self, x0, x1, y, z, yaw=0.0, gap=(0.13, 0.19), kinds=None, along=(1, 0)):
        """Bottles side by side from x0 to x1 along `along` (floor direction)."""
        rng = self.rng
        n = 0
        t = 0.0
        length = x1 - x0
        while t < length - 0.04:
            kind = rng.choice(kinds or self.KINDS)
            self.bottle(x0 + along[0] * t + 0.04 * along[0], y, z + along[1] * t, kind, yaw=yaw + rng.uniform(-0.25, 0.25))
            n += 1
            t += rng.uniform(*gap)
        return n

    # ---- Glassware ----
    GLASSES = {
        # (outer profile, liquid top height, liquid radius at top, liquid bottom)
        "pint": ([(0, 0), (0.03, 0), (0.031, 0.008), (0.042, 0.15), (0.044, 0.155), (0.0, 0.155)], 0.14),
        "rocks": ([(0, 0), (0.04, 0), (0.041, 0.012), (0.042, 0.09), (0.0, 0.09)], 0.05),
        "highball": ([(0, 0), (0.031, 0), (0.032, 0.01), (0.034, 0.15), (0.0, 0.15)], 0.12),
        "shot": ([(0, 0), (0.02, 0), (0.021, 0.012), (0.025, 0.06), (0.0, 0.06)], 0.045),
    }

    def glass(self, x, y, z, kind="rocks", drink=None, yaw=0.0, ice=False):
        """A drinking glass, optionally with a drink (LIQUIDS key) inside."""
        g = self.mat("glass")
        if kind in self.GLASSES:
            prof, fill_h = self.GLASSES[kind]
            outer = common.lathe("Glass", prof, g, 16)
            _place(outer, x, y, z, yaw)
            if drink:
                rr = lambda h: max(r for (r, hh) in prof if hh <= h + 1e-6) - 0.003  # noqa: E731
                lp = [(0, prof[1][1] + 0.006), (prof[1][0] - 0.003, prof[1][1] + 0.006),
                      (rr(fill_h) * 0.98, fill_h), (0, fill_h)]
                self._liquid(lp, drink, x, y, z, yaw)
                if kind == "pint" and drink == "beer":
                    self._liquid([(0, fill_h), (rr(fill_h), fill_h), (rr(fill_h), fill_h + 0.013), (0, fill_h + 0.016)],
                                 "foam", x, y, z, yaw)
            if ice:
                for k in range(3):
                    a = k * 2.1
                    cube = common.obox("Ice cube", (0.014 * math.cos(a), fill_h - 0.012, 0.014 * math.sin(a)),
                                       (0.022, 0.022, 0.022), a, self.mat("ice"), 0.004)
                    _place(cube, x, y, z, yaw)
            return
        if kind in ("martini", "coupe", "wine", "snifter", "flute"):
            bowl = {"martini": [(0.0, 0.12), (0.055, 0.19), (0.0, 0.19)],
                    "coupe": [(0.0, 0.12), (0.03, 0.125), (0.05, 0.145), (0.052, 0.17), (0.0, 0.17)],
                    "wine": [(0.0, 0.11), (0.03, 0.12), (0.043, 0.16), (0.04, 0.21), (0.034, 0.23), (0.0, 0.23)],
                    "snifter": [(0.0, 0.06), (0.035, 0.07), (0.05, 0.1), (0.045, 0.14), (0.032, 0.16), (0.0, 0.16)],
                    "flute": [(0.0, 0.11), (0.018, 0.12), (0.026, 0.2), (0.025, 0.24), (0.0, 0.24)]}[kind]
            stem_top = bowl[0][1]
            prof = [(0, 0), (0.035 if kind != "snifter" else 0.03, 0), (0.035, 0.003), (0.004, 0.008),
                    (0.0035, stem_top - 0.004), (0.006, stem_top)] + bowl[1:]
            outer = common.lathe("Stemware", prof, g, 18)
            _place(outer, x, y, z, yaw)
            if drink:
                top = bowl[-2][1]
                lvl = stem_top + (top - stem_top) * 0.7
                rad = bowl[-2][0] * (0.85 if kind != "martini" else 0.68)
                self._liquid([(0, stem_top + 0.004), (rad * 0.5, (stem_top + lvl) / 2), (rad, lvl), (0, lvl)],
                             drink, x, y, z, yaw)
            return
        raise KeyError(kind)

    def _liquid(self, prof, drink, x, y, z, yaw):
        obj = common.lathe("Drink", prof, self.mat("liquid"), 14)
        i = _LIQ_KEYS.index(drink)
        common.swatch_uvs(obj, (i + 0.5) / 16, 0.5)
        _place(obj, x, y, z, yaw)
        return obj

    # ---- Bar tools ----
    def shaker(self, x, y, z, yaw=0.0):
        s = self.mat("steel")
        tin = common.lathe("Shaker tin", [(0, 0), (0.032, 0), (0.034, 0.004), (0.044, 0.17), (0.045, 0.175),
                                          (0.0, 0.175)], s, 20)
        _place(tin, x, y, z, yaw)
        cap = common.lathe("Shaker top", [(0, 0.17), (0.043, 0.17), (0.044, 0.2), (0.03, 0.235), (0.016, 0.25),
                                          (0.017, 0.27), (0, 0.27)], s, 20)
        _place(cap, x, y, z, yaw)

    def tin(self, x, y, z, yaw=0.0, tall=True):
        """A Boston shaker tin (open)."""
        h = 0.17 if tall else 0.12
        t = common.lathe("Tin", [(0, 0), (0.03, 0), (0.031, 0.004), (0.044, h), (0.04, h - 0.002), (0.028, 0.006),
                                 (0, 0.006)], self.mat("steel"), 20)
        _place(t, x, y, z, yaw)

    def jigger(self, x, y, z, yaw=0.0):
        j = common.lathe("Jigger", [(0, 0), (0.024, 0), (0.022, 0.003), (0.009, 0.045), (0.009, 0.05),
                                    (0.02, 0.09), (0.021, 0.093), (0.0, 0.093)], self.mat("copper"), 16)
        _place(j, x, y, z, yaw)

    def strainer(self, x, y, z, yaw=0.0):
        s = self.mat("steel")
        disc = common.cylinder("Strainer", Vector((0, 0.004, 0)), 0.042, 0.004, "y", s, segments=20)
        _place(disc, x, y, z, yaw)
        spring = common.torus("Strainer spring", Vector((0, 0.008, 0)), 0.034, 0.004, s, segments=24, sides=5)
        _place(spring, x, y, z, yaw)
        handle = common.box("Strainer handle", (0.04, 0.002, -0.009), (0.13, 0.006, 0.009), s, 0.002)
        _place(handle, x, y, z, yaw)

    def bar_spoon(self, x, y, z, yaw=0.0, lean=0.0):
        s = self.mat("steel")
        pts = [(0.0, 0.0, 0.0), (0.0, 0.32, 0.0)]
        rod = common.tube("Bar spoon", pts, 0.0028, s, sides=6)
        bowl = common.sphere("Spoon bowl", Vector((0, 0.0, 0)), 0.011, s, 10, 6)
        bowl.data.transform(Matrix.Diagonal((1.0, 0.35, 1.5, 1.0)))
        tilt = Matrix.Rotation(lean, 4, "Z")
        for o in (rod, bowl):
            _place(o, x, y, z, yaw, tilt)

    def muddler(self, x, y, z, yaw=0.0, lean=0.0):
        m = common.cylinder("Muddler", Vector((0, 0.11, 0)), 0.012, 0.22, "y", self.mat("wood"), segments=10)
        _place(m, x, y, z, yaw, Matrix.Rotation(lean, 4, "Z"))

    def utensil_cup(self, x, y, z, yaw=0.0):
        """A steel cup holding bar spoons, a muddler and straws."""
        cup = common.lathe("Utensil cup", [(0, 0), (0.035, 0), (0.037, 0.12), (0.034, 0.12), (0.032, 0.006),
                                           (0, 0.006)], self.mat("steel"), 16)
        _place(cup, x, y, z, yaw)
        for k, lean in enumerate((-0.12, 0.08, 0.15)):
            a = yaw + k * 2.1
            self.bar_spoon(x + 0.01 * math.cos(a), y + 0.01, z + 0.01 * math.sin(a), a, lean)
        self.muddler(x - 0.012, y + 0.005, z + 0.01, yaw + 1.0, -0.15)

    def wedge(self, x, y, z, kind=0, yaw=0.0):
        """A citrus wedge (kind 0 lime, 1 lemon, 2 orange) lying on its side."""
        cols = {0: 0, 1: 1, 2: 2}
        sp = common.sphere("Wedge", Vector((0, 0, 0)), 0.022, self.mat("fruit"), 10, 6)
        bm = bmesh.new()
        bm.from_mesh(sp.data)
        bmesh.ops.bisect_plane(bm, geom=bm.verts[:] + bm.edges[:] + bm.faces[:], plane_co=(0, 0, 0),
                               plane_no=(0, 1, 0), clear_inner=True)
        bmesh.ops.contextual_create(bm, geom=[e for e in bm.edges if e.is_boundary])
        bm.to_mesh(sp.data)
        bm.free()
        sp.data.transform(Matrix.Diagonal((1.0, 1.0, 0.45, 1.0)))
        common.swatch_uvs(sp, (cols[kind] * 4 + 2) / 32, 0.5)
        _place(sp, x, y, z, yaw)

    def cherry(self, x, y, z):
        c = common.sphere("Cherry", Vector((0, 0.009, 0)), 0.009, self.mat("fruit"), 8, 6)
        common.swatch_uvs(c, (3 * 4 + 2) / 32, 0.5)
        _place(c, x, y, z)

    def olive(self, x, y, z):
        c = common.sphere("Olive", Vector((0, 0.008, 0)), 0.008, self.mat("fruit"), 8, 6)
        c.data.transform(Matrix.Diagonal((1, 1.35, 1, 1)))
        common.swatch_uvs(c, (4 * 4 + 2) / 32, 0.5)
        _place(c, x, y, z)

    def garnish_tray(self, x, y, z, yaw=0.0):
        """A steel tray with six compartments: limes, lemons, oranges,
        cherries, olives, mint."""
        s = self.mat("steel")
        w, d = 0.36, 0.13
        tray = common.box("Garnish tray", (-w / 2, 0, -d / 2), (w / 2, 0.05, d / 2), s, 0.004)
        _place(tray, x, y, z, yaw)
        for k in range(6):
            cx = -w / 2 + w / 6 * (k + 0.5)
            for j in range(4):
                px, pz = cx + self.rng.uniform(-0.02, 0.02), self.rng.uniform(-0.035, 0.035)
                wx = x + math.cos(yaw) * px + math.sin(yaw) * pz
                wz = z - math.sin(yaw) * px + math.cos(yaw) * pz
                if k < 3:
                    self.wedge(wx, y + 0.04, wz, k, yaw + self.rng.uniform(0, 6.3))
                elif k == 3:
                    self.cherry(wx, y + 0.04, wz)
                elif k == 4:
                    self.olive(wx, y + 0.04, wz)
                else:
                    leaf = common.sphere("Mint", Vector((0, 0, 0)), 0.014, self.mat("leaf2"), 8, 4)
                    leaf.data.transform(Matrix.Diagonal((1.0, 0.25, 0.6, 1.0)))
                    _place(leaf, wx, y + 0.045, wz, self.rng.uniform(0, 6.3))

    def bar_mat(self, x, y, z, length, yaw=0.0, width=0.09):
        """A ribbed rubber service mat."""
        m = common.box("Service mat", (-length / 2, 0, -width / 2), (length / 2, 0.006, width / 2),
                       self.mat("rubber"), 0.003)
        _place(m, x, y, z, yaw)
        n = int(length / 0.025)
        for k in range(n):
            u = -length / 2 + (k + 0.5) * length / n
            rib = common.box("Mat rib", (u - 0.004, 0.006, -width / 2 + 0.008), (u + 0.004, 0.011, width / 2 - 0.008),
                             self.mat("rubber"), 0.0)
            _place(rib, x, y, z, yaw)

    # ---- Room clutter ----
    def book(self, x, y, z, w, h, d, yaw=0.0, color=None, lying=False):
        """A book standing on its bottom edge (spine facing +z) or lying flat."""
        color = self.rng.randrange(16) if color is None else color
        if lying:
            obj = common.box("Book", (-h / 2, 0, -d / 2), (h / 2, w, d / 2), self.mat("books"), 0.003)
        else:
            obj = common.box("Book", (-w / 2, 0, -d / 2), (w / 2, h, d / 2), self.mat("books"), 0.003)
        r, c = divmod(color, 4)
        u0, v0 = c / 4, r / 4
        common.set_uvs(obj, lambda co, n: (u0 + 0.02 + 0.21 * ((co.x / max(w, h) + 0.5) % 1.0),
                                           v0 + 0.02 + 0.21 * min(max(co.y / max(h, w), 0), 1)))
        _place(obj, x, y, z, yaw)
        pages = common.box("Pages", (-w / 2 + 0.003, 0.004, -d / 2 - 0.0005), (w / 2 - 0.003, h - 0.004, d / 2 - 0.006),
                           self.mat("pages"), 0.0) if not lying else None
        if pages is not None:
            _place(pages, x, y, z, yaw)

    def book_row(self, x, y, z, length, depth=0.17, yaw=0.0, max_h=0.26):
        """Books standing side by side, from (x, y, z) along the shelf's
        local +x (turned by yaw), spines facing local +z."""
        rng = self.rng
        dx, dz = math.cos(yaw), -math.sin(yaw)
        t = 0.0
        while t < length - 0.03:
            w = rng.uniform(0.022, 0.05)
            h = rng.uniform(0.17, max_h)
            if rng.random() < 0.07 and t > 0.1:  # a gap now and then
                t += 0.06
                continue
            u = t + w / 2
            self.book(x + dx * u, y, z + dz * u, w, h, depth, yaw)
            t += w + 0.002

    def frame(self, x, y, z, w, h, photo, yaw=0.0, depth=0.03, border=0.035, mat=None):
        """A picture frame hanging with its back at (x, y, z) facing +z."""
        fm = mat or self.mat("dark_wood")
        parts = [((-w / 2, -h / 2), (w / 2, -h / 2 + border)), ((-w / 2, h / 2 - border), (w / 2, h / 2)),
                 ((-w / 2, -h / 2), (-w / 2 + border, h / 2)), ((w / 2 - border, -h / 2), (w / 2, h / 2))]
        for (a, b) in parts:
            o = common.box("Frame", (a[0], a[1], 0), (b[0], b[1], depth), fm, 0.004)
            _place(o, x, y, z, yaw)
        r, c = divmod(photo % 16, 4)
        u0, v0 = c / 4 + 0.005, r / 4 + 0.005
        img = common.quad("Photo", [(-w / 2 + border, -h / 2 + border, depth * 0.4),
                                    (w / 2 - border, -h / 2 + border, depth * 0.4),
                                    (w / 2 - border, h / 2 - border, depth * 0.4),
                                    (-w / 2 + border, h / 2 - border, depth * 0.4)], self.mat("photos"),
                          uvs=((u0, v0), (u0 + 0.24, v0), (u0 + 0.24, v0 + 0.24), (u0, v0 + 0.24)))
        _place(img, x, y, z, yaw)

    def candle(self, x, y, z, h=0.08, r=0.025, lit=True):
        common.cylinder("Candle", Vector((x, y + h / 2, z)), r, h, "y", self.mat("wax"), segments=14)
        if lit:
            f = common.sphere("Flame", Vector((x, y + h + 0.018, z)), 0.008, self.mat("flame"), 8, 6)
            f.data.transform(Matrix.Translation((x, y + h + 0.018, z)) @ Matrix.Diagonal((1, 2.2, 1, 1)) @
                             Matrix.Translation((-x, -(y + h + 0.018), -z)))

    def plant(self, x, y, z, size=1.0, pot_r=0.16, pot_h=0.3, leaves=26, seed=0, hanging=False):
        """A potted plant: terracotta pot, soil, and leaves on stems."""
        rng = random.Random(seed)
        if not hanging:
            pot = common.lathe("Pot", [(0, 0), (pot_r * 0.75, 0), (pot_r, pot_h), (pot_r * 1.08, pot_h),
                                       (pot_r * 1.08, pot_h + 0.03), (pot_r * 0.93, pot_h + 0.03),
                                       (pot_r * 0.9, pot_h * 0.6), (0, pot_h * 0.6)], self.mat("terracotta"), 20)
            _place(pot, x, y, z)
            common.cylinder("Soil", Vector((x, y + pot_h - 0.01, z)), pot_r * 0.92, 0.01, "y", self.mat("soil"),
                            segments=16)
        for k in range(leaves):
            a = rng.uniform(0, 2 * math.pi)
            if hanging:
                L = rng.uniform(0.3, 0.9) * size
                droop = [(0, 0, 0)]
                for t in (0.25, 0.5, 0.75, 1.0):
                    droop.append((math.cos(a) * 0.15 * t * size, -L * t * t, math.sin(a) * 0.15 * t * size))
                common.tube("Vine", [(x + px, y + py, z + pz) for (px, py, pz) in droop], 0.003, self.mat("leaf"), 4)
                for t in (0.3, 0.55, 0.8, 1.0):
                    p = droop[min(4, int(t * 4))]
                    lf = common.sphere("Leaf", Vector((0, 0, 0)), 0.03 * size, self.mat("leaf2"), 6, 4)
                    lf.data.transform(Matrix.Diagonal((1.0, 0.2, 0.6, 1.0)))
                    _place(lf, x + p[0], y + p[1], z + p[2], rng.uniform(0, 6.3))
                continue
            L = rng.uniform(0.25, 0.6) * size
            up = rng.uniform(0.5, 1.2)
            tip = (math.cos(a) * L * 0.55, L * up, math.sin(a) * L * 0.55)
            base = (x, y + pot_h, z)
            common.tube("Stem", [base, (base[0] + tip[0] * 0.5, base[1] + tip[1] * 0.6, base[2] + tip[2] * 0.5),
                                 (base[0] + tip[0], base[1] + tip[1], base[2] + tip[2])], 0.004, self.mat("leaf"), 4)
            lf = common.sphere("Leaf", Vector((0, 0, 0)), 0.07 * size, self.mat("leaf" if k % 2 else "leaf2"), 8, 5)
            lf.data.transform(Matrix.Rotation(-0.5, 4, "X") @ Matrix.Diagonal((0.5, 0.12, 1.0, 1.0)))
            _place(lf, base[0] + tip[0], base[1] + tip[1], base[2] + tip[2], -a + math.pi / 2)


# ---- Atlases ----

def _label_atlas():
    """16 bottle label designs (4x4)."""
    LS = 128
    labels = np.zeros((LS * 4, LS * 4, 3), dtype=np.float32)
    PAPER = [(0.85, 0.80, 0.65), (0.08, 0.08, 0.08), (0.75, 0.10, 0.08), (0.92, 0.92, 0.88),
             (0.12, 0.20, 0.35), (0.70, 0.55, 0.20), (0.15, 0.30, 0.15), (0.55, 0.15, 0.25)]
    INK = [(0.10, 0.08, 0.05), (0.85, 0.70, 0.35), (0.95, 0.90, 0.80), (0.60, 0.10, 0.08),
           (0.85, 0.80, 0.60), (0.10, 0.08, 0.05), (0.90, 0.85, 0.60), (0.95, 0.85, 0.75)]
    lr = np.random.default_rng(3)
    yy, xx = np.mgrid[0:LS, 0:LS]
    for k in range(16):
        paper, ink = PAPER[k % 8], INK[(k * 3) % 8]
        cell = np.empty((LS, LS, 3), dtype=np.float32)
        cell[:] = paper
        cell *= (0.94 + 0.06 * lr.random((LS, LS, 1))).astype(np.float32)
        bw = 4 + k % 3 * 2
        edge = (xx < bw) | (xx >= LS - bw) | (yy < bw) | (yy >= LS - bw)
        inner = (xx >= bw + 3) & (xx < LS - bw - 3) & (yy >= bw + 3) & (yy < LS - bw - 3)
        cell[edge] = ink
        band = (yy > LS * 0.42) & (yy < LS * 0.62) & inner
        name_c = ink
        if k % 2:
            cell[band] = ink
            name_c = paper
        x = LS * 0.18
        for _ in range(lr.integers(2, 4)):
            wl = lr.integers(12, 28)
            cell[(xx > x) & (xx < x + wl) & (yy > LS * 0.47) & (yy < LS * 0.57)] = name_c
            x += wl + 6
        for line in range(3):
            y0 = LS * (0.18 + 0.06 * line)
            lw = lr.integers(40, 80)
            cell[(xx > (LS - lw) / 2) & (xx < (LS + lw) / 2) & (yy > y0) & (yy < y0 + 3)] = ink
        cy, cx, cr = LS * 0.78, LS * 0.5, LS * (0.08 + 0.02 * (k % 3))
        d = np.sqrt((xx - cx) ** 2 + (yy - cy) ** 2)
        cell[(d < cr) & (d > cr - 3)] = ink
        if k % 3 == 0:
            cell[d < cr * 0.5] = ink
        r, c = divmod(k, 4)
        labels[r * LS:(r + 1) * LS, c * LS:(c + 1) * LS] = cell
    return labels


def _book_atlas():
    """16 book cover/spine colors with bands and title bars (4x4)."""
    S = 64
    img = np.zeros((S * 4, S * 4, 3), np.float32)
    cols = [(0.35, 0.05, 0.04), (0.05, 0.12, 0.25), (0.08, 0.20, 0.10), (0.45, 0.35, 0.20),
            (0.10, 0.10, 0.10), (0.55, 0.40, 0.10), (0.30, 0.10, 0.25), (0.65, 0.60, 0.50),
            (0.15, 0.25, 0.35), (0.50, 0.15, 0.05), (0.20, 0.30, 0.15), (0.70, 0.20, 0.15),
            (0.25, 0.18, 0.12), (0.10, 0.30, 0.30), (0.60, 0.50, 0.30), (0.40, 0.40, 0.45)]
    rng = np.random.default_rng(5)
    for k in range(16):
        r, c = divmod(k, 4)
        cell = np.empty((S, S, 3), np.float32)
        cell[:] = cols[k]
        cell *= (0.9 + 0.1 * rng.random((S, S, 1))).astype(np.float32)
        gold = (0.8, 0.6, 0.25) if k % 3 else (0.9, 0.88, 0.8)
        for y in (int(S * 0.12), int(S * 0.85)):
            cell[y:y + 2] = gold
        cell[int(S * 0.55):int(S * 0.7), int(S * 0.2):int(S * 0.8)] = gold if k % 2 else np.array(cols[k]) * 0.5
        img[r * S:(r + 1) * S, c * S:(c + 1) * S] = cell
    return img


def _photo_atlas():
    """16 "photos" and posters: sunsets, portraits-ish blobs, band posters,
    old-time black and white, abstract art (4x4)."""
    S = 128
    img = np.zeros((S * 4, S * 4, 3), np.float32)
    rng = np.random.default_rng(9)
    yy, xx = np.mgrid[0:S, 0:S] / S
    for k in range(16):
        r, c = divmod(k, 4)
        kind = k % 5
        if kind == 0:  # sunset over water
            sky = np.stack([0.9 - yy * 0.3, 0.35 + yy * 0.1, 0.25 + yy * 0.5], -1)
            sun = np.exp(-(((xx - 0.6) ** 2 + (yy - 0.45) ** 2) / 0.01))[..., None]
            cell = sky * (yy > 0.35)[..., None] + np.stack([0.05 + 0 * yy, 0.08 + 0 * yy, 0.18 + 0 * yy], -1) * (yy <= 0.35)[..., None]
            cell = cell + sun * np.array([1.0, 0.7, 0.3])
        elif kind == 1:  # sepia group photo
            base = 0.25 + 0.35 * yy
            cell = np.stack([base, base * 0.85, base * 0.65], -1)
            for p in range(5):
                px = 0.15 + p * 0.17
                head = ((xx - px) ** 2 + (yy - 0.62) ** 2) < 0.004
                body = (np.abs(xx - px) < 0.06) & (yy < 0.55) & (yy > 0.1)
                cell[head | body] *= 0.35
        elif kind == 2:  # band poster: color block + title bars
            col = np.array([rng.uniform(0.3, 1), rng.uniform(0.1, 0.6), rng.uniform(0.1, 0.8)])
            cell = np.ones((S, S, 3)) * col * 0.4
            circle = ((xx - 0.5) ** 2 + (yy - 0.55) ** 2) < 0.06
            cell[circle] = col
            cell[(yy > 0.12) & (yy < 0.2) & (xx > 0.15) & (xx < 0.85)] = 0.95
            cell[(yy > 0.85) & (yy < 0.92) & (xx > 0.25) & (xx < 0.75)] = 0.9
        elif kind == 3:  # black and white street
            base = 0.15 + 0.6 * yy
            cell = np.stack([base] * 3, -1)
            for b in range(4):
                x0 = rng.uniform(0, 0.8)
                cell[(xx > x0) & (xx < x0 + 0.15) & (yy < rng.uniform(0.4, 0.9))] *= 0.4
        else:  # abstract
            cell = np.stack([np.sin(xx * 9 + k) * 0.3 + 0.5, np.sin(yy * 7 + k * 2) * 0.3 + 0.4,
                             np.cos((xx + yy) * 5 + k) * 0.3 + 0.5], -1)
        img[r * S:(r + 1) * S, c * S:(c + 1) * S] = np.clip(cell, 0, 1) ** 2.2
    return img
