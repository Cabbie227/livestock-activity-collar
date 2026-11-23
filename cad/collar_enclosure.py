"""Parametric 3D model of the collar enclosure (base + lid).

The case holds an Arduino Nano and a GY-521 (MPU6050) on a 50 x 30 mm piece
of perfboard, plus a 9 V battery in its own compartment. It is mounted on a
30 mm wide collar strap that runs through two slotted tabs at the ends.

Run:
    pip install manifold3d trimesh matplotlib
    python cad/collar_enclosure.py

Output:
    cad/collar_case_base.stl, cad/collar_case_lid.stl  - ready for a slicer (PLA/PETG, 0.2 mm layers)
    docs/images/enclosure.png                          - preview render

All dimensions are in millimetres. Change the parameters below to fit other parts.
"""

from pathlib import Path

import numpy as np
import trimesh
from manifold3d import Manifold

# ---------- Parameters ----------
WALL = 2.0            # wall thickness
FLOOR = 2.0           # floor thickness
CORNER_R = 4.0        # outer corner radius

BATTERY = (49.0, 27.0, 18.0)   # 9 V battery pocket (L, W, H) incl. clearance
BOARD = (52.0, 32.0)           # perfboard with Nano + GY-521, incl. clearance
DIVIDER = 1.6                  # wall between battery and electronics
INNER_H = 22.0                 # inner height (Nano on headers + MPU6050)

STRAP_W = 32.0        # slot length for a 30 mm strap (max. ~33 mm at this case width)
STRAP_T = 4.0         # slot width (strap thickness + clearance)
TAB_L = 12.0          # length of each strap tab

LID_T = 2.0           # lid plate thickness
LID_LIP = 2.0         # lip that fits inside the base
FIT = 0.3             # clearance between lid lip and base walls

SCREW_R = 1.4         # M3 self-tapping pilot hole radius in the posts
SCREW_HEAD_R = 1.7    # M3 clearance hole in the lid
POST_R = 3.5          # radius of the corner screw posts

LED_R = 2.6           # 5 mm LED holes
BUZZER_HOLE_R = 0.8   # sound holes above the buzzer
USB = (9.0, 6.0)      # mini-USB opening for the Nano (W, H)
SWITCH_R = 3.2        # 6 mm toggle switch for the battery

# ---------- Derived sizes ----------
IN_L = BATTERY[0] + DIVIDER + BOARD[0]
IN_W = max(BATTERY[1], BOARD[1])
OUT_L = IN_L + 2 * WALL
OUT_W = IN_W + 2 * WALL
OUT_H = INNER_H + FLOOR

SEG = 48  # circle resolution


def rounded_box(l, w, h, r):
    """Box with rounded vertical edges, from (0,0,0) to (l,w,h)."""
    r = min(r, l / 2 - 0.01, w / 2 - 0.01)
    if r <= 0.05:
        return Manifold.cube([l, w, h])
    corners = [
        Manifold.cylinder(h, r, circular_segments=SEG).translate([x, y, 0])
        for x in (r, l - r)
        for y in (r, w - r)
    ]
    return Manifold.batch_hull(corners)


def cyl(h, r, x, y, z=0.0):
    return Manifold.cylinder(h, r, circular_segments=SEG).translate([x, y, z])


def box(l, w, h, x, y, z):
    return Manifold.cube([l, w, h]).translate([x, y, z])


def post_positions():
    d = WALL + POST_R - 0.5
    return [(d, d), (OUT_L - d, d), (d, OUT_W - d), (OUT_L - d, OUT_W - d)]


def make_base():
    # Shell
    shell = rounded_box(OUT_L, OUT_W, OUT_H, CORNER_R)
    cavity = rounded_box(IN_L, IN_W, OUT_H, CORNER_R - WALL).translate([WALL, WALL, FLOOR])
    base = shell - cavity

    # Divider between battery and electronics (lower, so wires can pass over it)
    div_x = WALL + BATTERY[0]
    base += box(DIVIDER, IN_W, BATTERY[2] - 4, div_x, WALL, FLOOR)

    # Screw posts in the corners
    for x, y in post_positions():
        base += cyl(INNER_H - LID_LIP - FIT, POST_R, x, y, FLOOR)
        base -= cyl(INNER_H, SCREW_R, x, y, FLOOR + 2)

    # Strap tabs at both ends with a slot for the collar strap
    tab_h = 4.0
    for x0 in (-TAB_L, OUT_L):
        tab = box(TAB_L + 1, OUT_W, tab_h, x0 if x0 < 0 else x0 - 1, 0, 0)  # 1 mm overlap with the shell
        slot_len = min(STRAP_W, OUT_W - 2 * 1.5)
        slot_x = x0 + (TAB_L - STRAP_T) / 2
        tab -= box(STRAP_T, slot_len, tab_h + 2, slot_x, (OUT_W - slot_len) / 2, -1)
        base += tab

    # Mini-USB opening in the electronics end wall (Nano sits at that end)
    base -= box(WALL + 2, USB[0], USB[1], OUT_L - WALL - 1, (OUT_W - USB[0]) / 2, FLOOR + 6)

    # Power switch hole in the battery end wall
    sw = Manifold.cylinder(WALL + 2, SWITCH_R, circular_segments=SEG).rotate([0, 90, 0])
    base -= sw.translate([-1, OUT_W / 2, FLOOR + 12])

    # Arrow engraved in the floor: mount the MPU6050 with its X axis this way,
    # i.e. across the strap, pointing towards the animal's head.
    ax_x = WALL + BATTERY[0] + DIVIDER + BOARD[0] / 2
    y0 = OUT_W / 2 - 8
    depth = 0.6
    base -= box(1.2, 12, depth + 1, ax_x - 0.6, y0, FLOOR - depth)
    head = Manifold.batch_hull([
        cyl(depth + 1, 0.3, ax_x - 3, y0 + 12, FLOOR - depth),
        cyl(depth + 1, 0.3, ax_x + 3, y0 + 12, FLOOR - depth),
        cyl(depth + 1, 0.3, ax_x, y0 + 17, FLOOR - depth),
    ])
    base -= head
    return base


def make_lid():
    lid = rounded_box(OUT_L, OUT_W, LID_T, CORNER_R)
    # Lip that fits inside the walls
    lip_l, lip_w = IN_L - 2 * FIT, IN_W - 2 * FIT
    lip = rounded_box(lip_l, lip_w, LID_LIP, CORNER_R - WALL).translate([WALL + FIT, WALL + FIT, LID_T])
    lip_hole = rounded_box(lip_l - 2 * WALL, lip_w - 2 * WALL, LID_LIP + 2, 1.0).translate(
        [WALL + FIT + WALL, WALL + FIT + WALL, LID_T - 1])
    lid += lip - lip_hole

    for x, y in post_positions():
        lid -= cyl(LID_T + LID_LIP + 2, POST_R + FIT, x, y, LID_T)  # room for the posts
        lid -= cyl(LID_T + 2, SCREW_HEAD_R, x, y, -1)

    # LEDs and buzzer above the electronics compartment
    ex = WALL + BATTERY[0] + DIVIDER
    lid -= cyl(LID_T + 2, LED_R, ex + 12, OUT_W / 2 - 6, -1)   # green: active
    lid -= cyl(LID_T + 2, LED_R, ex + 12, OUT_W / 2 + 6, -1)   # red: alarm
    bx, by = ex + 36, OUT_W / 2
    for dx in (-3, 0, 3):
        for dy in (-3, 0, 3):
            lid -= cyl(LID_T + 2, BUZZER_HOLE_R, bx + dx, by + dy, -1)
    return lid


def to_trimesh(man):
    mesh = man.to_mesh()
    return trimesh.Trimesh(vertices=np.asarray(mesh.vert_properties)[:, :3],
                           faces=np.asarray(mesh.tri_verts), process=True)


def render_preview(base, lid, path):
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    from mpl_toolkits.mplot3d.art3d import Poly3DCollection

    fig = plt.figure(figsize=(12, 4.2), dpi=160)
    views = [("Base", base, (32, -58)), ("Lid (top side)", lid, (40, -58))]
    for i, (title, mesh, (elev, azim)) in enumerate(views, 1):
        ax = fig.add_subplot(1, 2, i, projection="3d", computed_zorder=True)
        tris = mesh.vertices[mesh.faces]
        # simple Lambert shading from the face normals
        light = np.array([0.3, -0.5, 0.8])
        light /= np.linalg.norm(light)
        shade = 0.30 + 0.70 * np.clip(mesh.face_normals @ light, 0, 1)
        colors = np.c_[0.20 * shade + 0.1, 0.55 * shade + 0.1, 0.45 * shade + 0.1, np.ones_like(shade)]
        ax.add_collection3d(Poly3DCollection(tris, facecolors=colors, edgecolor="none", linewidth=0, antialiased=False))
        lo, hi = mesh.bounds
        ax.set_xlim(lo[0], hi[0]); ax.set_ylim(lo[1], hi[1]); ax.set_zlim(lo[2], hi[2])
        ax.set_box_aspect(hi - lo, zoom=1.0)
        ax.view_init(elev=elev, azim=azim)
        ax.set_axis_off()
        ax.set_title(f"{title}\n{hi[0]-lo[0]:.0f} x {hi[1]-lo[1]:.0f} x {hi[2]-lo[2]:.0f} mm", fontsize=10)
    plt.tight_layout()
    fig.savefig(path, bbox_inches="tight")


def main():
    root = Path(__file__).resolve().parent.parent
    base, lid = make_base(), make_lid()
    base_m, lid_m = to_trimesh(base), to_trimesh(lid)
    # Finer triangles only for the preview, so matplotlib's depth sorting looks right
    base_view = to_trimesh(base.refine_to_length(1.5))
    lid_view = to_trimesh(lid.refine_to_length(1.5))

    out = root / "cad"
    base_m.export(out / "collar_case_base.stl")
    lid_m.export(out / "collar_case_lid.stl")

    img_dir = root / "docs" / "images"
    img_dir.mkdir(parents=True, exist_ok=True)
    lid_view.apply_transform(trimesh.transformations.rotation_matrix(np.pi, [1, 0, 0]))
    render_preview(base_view, lid_view, img_dir / "enclosure.png")

    for name, m in (("base", base_m), ("lid", lid_m)):
        ext = m.bounds[1] - m.bounds[0]
        print(f"{name}: {ext[0]:.1f} x {ext[1]:.1f} x {ext[2]:.1f} mm, "
              f"volume {m.volume / 1000:.1f} cm3, watertight={m.is_watertight}")


if __name__ == "__main__":
    main()
