# %%
from build123d import *
from ocp_vscode import *
from epilogue import *

ow = 48
oh = 25
th = 3

mhole_cc = 25
insert_d = 4.1

rr = 2

with BuildPart() as p:
    # plate
    with BuildSketch(Plane.XY):
        Rectangle(ow, oh, align=(Align.CENTER, Align.MIN))
    extrude(amount=th)
    # bracket
    with BuildSketch(Plane.XZ):
        Rectangle(ow, 10, align=(Align.CENTER, Align.MIN))
    extrude(amount=th)

    filletz(p, 1)

    # mounting holes
    with BuildSketch(Plane.XZ):
        with Locations((0, 5)):
            with GridLocations(mhole_cc, 1, 2, 1):
                Circle(radius=insert_d/2-0.1) # orientation compensation
    extrude(amount=th, mode=Mode.SUBTRACT)
    with BuildSketch(Plane.XY):
        with Locations((0, oh/2)):
            Circle(radius=8.5/2)
    extrude(amount=th, mode=Mode.SUBTRACT)

epilogue(p)
