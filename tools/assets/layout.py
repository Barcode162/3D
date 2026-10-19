# The new bar's floor plan, shared by every asset script (see
# docs/BAR_REDESIGN.md). app/bar_scene.cpp repeats these numbers; change
# both together.
#
# World space = model space for the room-sized models: x to the right,
# y up, -z toward the back wall (the bar), floor at y = 0.
import math

X0, X1 = -9.0, 9.0      # left and right walls
Z0, Z1 = -7.4, 7.0      # back wall (bar) and front wall (street)
H = 4.0                 # hall ceiling
T = 0.3                 # wall thickness

# Lower ceiling (wood-slat soffit) over the bar.
SOFFIT_X1, SOFFIT_Z1, SOFFIT_Y = 2.6, -3.5, 3.1

# The back-right corner is cut off by a diagonal wall (the rhythm corner).
CHAMFER_A = (6.0, Z0)
CHAMFER_B = (X1, Z0 + 3.0)

# Bay window in the front wall, with a window seat.
BAY_X0, BAY_X1, BAY_DEPTH, BAY_H = -2.2, 1.8, 1.0, 2.9

# Street entrance (double door, kept closed) in the front wall.
ENTRY_X0, ENTRY_X1, ENTRY_H = 2.5, 3.9, 2.4

# Doorway A (back wall) and doorway B (left wall) lead to the two rooms
# that are still being built: brick arches, taped off.
DOOR_A_X0, DOOR_A_X1 = 2.9, 4.5
DOOR_B_Z0, DOOR_B_Z1 = -3.3, -1.7
ARCH_SPRING = 1.95      # where the round top starts
CORRIDOR_DEPTH = 3.0    # how far you can see into each unfinished room
CORRIDOR_H = 3.0
# The arcade behind doorway A (upgrade plan item 3, replaces "room 2"): an
# 8 x 8 m room, its front wall the back of the bar's back wall.
ARCADE_X0, ARCADE_X1 = 0.4, 8.4
ARCADE_Z1 = -7.7          # Z0 - T
ARCADE_Z0 = ARCADE_Z1 - 8.0
ARCADE_H = 3.4

# Raised stage (piano) in the front-left corner, with a step along its
# open sides.
STAGE_X1, STAGE_Z0, STAGE_H = -5.4, 3.8, 0.3
STEP_W, STEP_H = 0.35, 0.15

# Chimney breast for the lounge fireplace on the left wall.
CHIMNEY_Z0, CHIMNEY_Z1, CHIMNEY_D = -0.2, 1.8, 0.45

# The bar counter: its customer edge bows into the room along a big circle
# from x = BAR_XA to BAR_XB, then turns and runs back to the back wall.
BAR_XA, BAR_XB = -7.9, 1.1
BAR_Z_ENDS, BAR_Z_MID = -4.75, -4.0
BAR_INSET = 0.36        # counter centerline, behind the customer edge
BAR_TURN_R = 0.55       # radius of the turn (centerline)
BACK_BAR_X0, BACK_BAR_X1 = -8.8, 0.9


def bar_arc():
    """(center x, center z, radius) of the circle the customer edge follows."""
    a = (BAR_XB - BAR_XA) / 2
    s = BAR_Z_ENDS - BAR_Z_MID          # how far the middle bows out (negative z down)
    s = -s
    r = (a * a + s * s) / (2 * s)
    xc = (BAR_XA + BAR_XB) / 2
    return xc, BAR_Z_MID - r, r


def bar_centerline(step=0.05):
    """The counter's centerline as floor points [(x, z), ...], from the left
    end along the curve, around the turn, and back to the back wall. The
    customer side is on the right-hand normal (common.path_frames)."""
    xc, zc, r = bar_arc()
    rc = r - BAR_INSET
    a0 = math.atan2(0, BAR_XA - xc)  # placeholder, replaced below
    # Angles of the two ends, measured around the circle's center.
    def ang(x):
        return math.atan2(math.sqrt(max(r * r - (x - xc) ** 2, 0.0)), x - xc)
    a0, a1 = ang(BAR_XA), ang(BAR_XB)
    pts = []
    n = max(2, int(abs(a0 - a1) * rc / step))
    for i in range(n + 1):
        a = a0 + (a1 - a0) * i / n
        pts.append((xc + rc * math.cos(a), zc + rc * math.sin(a)))
    # Heading at the end of the curve, then turn until heading -z.
    (x1, z1), (x2, z2) = pts[-2], pts[-1]
    phi1 = math.atan2(z2 - z1, x2 - x1)
    nrm = (-math.sin(phi1), math.cos(phi1))
    cx, cz = x2 - BAR_TURN_R * nrm[0], z2 - BAR_TURN_R * nrm[1]
    phi_end = -math.pi / 2
    m = max(2, int(abs(phi1 - phi_end) * BAR_TURN_R / step))
    for i in range(1, m + 1):
        phi = phi1 + (phi_end - phi1) * i / m
        pts.append((cx - BAR_TURN_R * math.sin(phi), cz + BAR_TURN_R * math.cos(phi)))
    xr, zr = pts[-1]
    k = max(1, int((zr - Z0) / step))
    for i in range(1, k + 1):
        pts.append((xr, zr + (Z0 - zr) * i / k))
    return pts
