# %%
from build123d import *
from ocp_vscode import *
from epilogue import *

# spacing around charging pins
pin_cc = 50
pin_cl_x = 20
pin_cl_y = 15
pin_inset = 10

# wall thickness
wth = 7

# height
h = 20

# compute outer dimensions

iw = 2 * pin_cl_x + 2 * pin_cc
ow = 2*wth + iw
ih = pin_inset + pin_cl_y
oh = ih + wth

# Insert (M3)
ins_d = 4.1
ins_is = wth/2

recess_dia = 5
recess_depth = 3
recess_inset = 15

rr = 2

ALIGN_MIN = (Align.MIN, Align.MIN)
ALIGN_X = (Align.MIN, Align.CENTER)

with BuildPart() as p:
    # overall box
    with BuildSketch(Plane.XY):
        RectangleRounded(ow, oh, rr)
    extrude(amount=h)
    # cutout
    with BuildSketch(Plane.XY):
        with Locations((0, (oh-ih+rr)/2)):
            RectangleRounded(iw, ih+rr, rr)
    extrude(amount=h, mode=Mode.SUBTRACT)
    # insert holes
    with BuildSketch(Plane.XY):
        with GridLocations(ow-2*ins_is, oh-2*ins_is, 2, 2):
            Circle(radius=ins_d/2)
    extrude(amount=h, mode=Mode.SUBTRACT)
    filletz(p, rr)
    # recesses
    rx = ow/2+recess_dia/2-recess_depth
    with Locations((rx, oh/2 - recess_inset, h/2)):
            Sphere(radius=recess_dia/2, mode=Mode.SUBTRACT)
    with Locations((-rx, oh/2 - recess_inset, h/2)):
            Sphere(radius=recess_dia/2, mode=Mode.SUBTRACT)

epilogue(p)
