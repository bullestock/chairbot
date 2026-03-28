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

pts = [
    ( 0, 0 ),
    ( 0, H ),
    ( C, H ),
    ( C, H+E ),
    ( C+B-A, H+E ),
    ( C+B, H+F ),
    ( C+B, H ),
    ( D, H ),
]

with BuildPart() as p:
    with BuildSketch(Plane.YZ):
        with BuildLine():
            Polyline(pts, close=True)
        make_face()
    extrude(amount=groove_len)
    #fillet(p.edges().filter_by(Axis.Z), radius=1)
    #fillet(p.edges().sort_by(Axis.Z)[-1], radius=1)

epilogue(p)
