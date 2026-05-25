# %%
from build123d import *
from ocp_vscode import *
from epilogue import *

th = 4
sr = 25

with BuildPart() as p:
    with BuildSketch():
        RectangleRounded(6, th, 1)
    extrude(amount=65)
    with BuildSketch(Plane.XZ.offset(-th/2)):
        with Locations((0, 0)):
            RectangleRounded(20, 25, 5)
    extrude(amount=th)
    with BuildSketch(p.faces().sort_by(Axis.Z).last):
        RectangleRounded(3, 1.5, 0.5)
    extrude(amount=-0.1, mode=Mode.SUBTRACT)
    with Locations((0, sr+0.5, 0)):
            Sphere(radius=sr, mode=Mode.SUBTRACT)
    with Locations((0, -sr-0.5, 0)):
            Sphere(radius=sr, mode=Mode.SUBTRACT)
epilogue(p)
