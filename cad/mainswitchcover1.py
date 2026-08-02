# %%
from build123d import *
from ocp_vscode import *
from epilogue import *

# Overall w/h
ow = 84.5
oh = 65
th = 34
# Cutout w/h
cw = 47
ch = 62
cx = 30
# Insert 1 (M6)
ins1_x = 25
ins1_cc = 20
ins1_d = 7.1
# Insert 2 (M3)
ins2_d = 4.1
ins2_is = 8

rr = 2

ALIGN_MIN = (Align.MIN, Align.MIN)
ALIGN_X = (Align.MIN, Align.CENTER)

with BuildPart() as p:
    with BuildSketch(Plane.XY):
        RectangleRounded(ow, oh, rr, align=ALIGN_MIN)
    extrude(amount=th)
    with BuildSketch(Plane.XY):
        with Locations((cx, (ch-oh-rr)/2)):
            RectangleRounded(cw, ch+rr, rr, align=ALIGN_MIN)
    extrude(amount=th, mode=Mode.SUBTRACT)
    with BuildSketch(Plane.XY):
        with Locations((ins1_x, 30)):
            with GridLocations(1, ins1_cc, 1, 2):
                Circle(radius=ins1_d/2)
    extrude(amount=th, mode=Mode.SUBTRACT)
    with BuildSketch(Plane.XY):
        with Locations((ow/2, oh/2)):
            with GridLocations(ow-ins2_is, oh-ins2_is, 2, 2):
                Circle(radius=ins2_d/2)
    extrude(amount=th, mode=Mode.SUBTRACT)

epilogue(p)
