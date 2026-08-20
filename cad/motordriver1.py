# %%
from build123d import *
from ocp_vscode import *
from epilogue import *

hole_cc = 40
ow = 48
standoff_d = 8
standoff_h = 25
standoff_cut = 1.5
th = 3

# Insert (M3)
insert_d = 4.1
insert_l = 5

mhole_cc = 25

rr = 2

with BuildPart() as p:
    # plate
    with BuildSketch(Plane.XY):
        RectangleRounded(ow, ow, rr)
    extrude(amount=th)
    # standoffs
    with BuildSketch(last_z(p)):
        with GridLocations(hole_cc, hole_cc, 2, 2):
            Circle(radius=standoff_d/2)
    extrude(amount=standoff_h)
    with BuildSketch(Plane.XY.offset(th+standoff_h)):
        with GridLocations(hole_cc, hole_cc, 2, 2):
            Circle(radius=insert_d/2)
    extrude(amount=-insert_l, mode=Mode.SUBTRACT)
    # cutoff upper standoffs
    cw = 10
    with BuildSketch(Plane.XY.offset(th)):
        with Locations((0, ow/2-cw/2-standoff_d+standoff_cut)):
            Rectangle(ow, cw)
    extrude(amount=standoff_h, mode=Mode.SUBTRACT)
    # cutout
    with BuildSketch(Plane.XY):
         with Locations((0, -10)):
             RectangleRounded(ow/2, ow, rr)
    extrude(amount=th, mode=Mode.SUBTRACT)
    filletz(p, rr)
    # mounting holes
    with BuildSketch(Plane.XY):
        with Locations((0, ow/2-5)):
            with GridLocations(mhole_cc, 1, 2, 1):
                Circle(radius=3.2/2)
    extrude(amount=th, mode=Mode.SUBTRACT)

epilogue(p)
