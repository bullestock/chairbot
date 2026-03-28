# %%
from build123d import *
from ocp_vscode import *
from epilogue import *

A = 1.75
B = 6
C = 4.5
D = 20
E = 1.9
F = 5.5
H = 3 # max 8
groove_len = 40
groove_h = F+H

lid_th = 2

pocket_w = 30
pocket_h = 11
pocket_l = 48 + lid_th

shell_th = 1.5

pts = [
    ( 0, 0 ),
    ( 0, H ),
    ( C, H ),
    ( C, H+E ),
    ( C+B-A, H+E ),
    ( C+B-A, H+F ),
    ( C+B, H+F ),
    ( C+B, H ),
    ( D, H ),
    ( D, 0 ),
]

for i in range(0, len(pts)):
    pts[i] = ( pts[i][0] - D/2, pocket_h - pts[i][1] + 1 )

with BuildPart() as cutout:
    with BuildSketch(Plane.XZ.offset(11.5)):
        with BuildLine():
            Polyline(pts, close=True)
        make_face()
    extrude(amount=groove_len)
    fillet(cutout.edges().filter_by(Axis.Y), radius=.5)

with BuildPart() as p:
    with BuildSketch(Plane.XZ):
        RectangleRounded(
            pocket_w + 2*shell_th,
            pocket_h + 2*shell_th + groove_h,
            1)
    extrude(amount=pocket_l + shell_th)
    fillet(p.edges().filter_by(Axis.X), radius=1)
    with BuildSketch(p.faces().sort_by(Axis.Y).last):
        with Locations((0, groove_h/2)):
            RectangleRounded(pocket_w, pocket_h, 0.5)
    extrude(amount=-pocket_l, mode=Mode.SUBTRACT)

p.part -= cutout.part

epilogue(p)
