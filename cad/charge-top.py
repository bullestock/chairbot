# %%
from build123d import *
from ocp_vscode import *
from epilogue import *

# spacing around charging pins
pin_cc = 35
pin_cl_x = 20
pin_cl_y = 15
pin_inset = 10

# wall thickness
wth = 7
bth = 3

# inner height
h = 17.9
outer_h = h + bth

mhole_cc = 150
mhole_d = 8.5

# compute outer dimensions

iw = 2 * pin_cl_x + 2 * pin_cc
ow = 2*wth + iw
ih = pin_inset + pin_cl_y
oh = ih + wth

# Insert (M3)
ins_d = 4.1
ins_is = wth/2
ins_l = 5

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
    extrude(amount=outer_h)
    # mount flange
    with BuildSketch(Plane.XY.offset(h)):
        RectangleRounded(mhole_cc + 2*mhole_d, oh, rr)
    extrude(amount=bth)
    # cutout
    with BuildSketch(Plane.XY):
        with Locations((0, (oh-ih+rr)/2)):
            RectangleRounded(iw, ih+rr, rr)
    extrude(amount=h, mode=Mode.SUBTRACT)
    # insert holes
    with BuildSketch(Plane.XY):
        with GridLocations(ow-2*ins_is, oh-2*ins_is, 2, 2):
            Circle(radius=ins_d/2)
    extrude(amount=ins_l, mode=Mode.SUBTRACT)
    filletz(p, rr)
    # recesses
    rx = ow/2+recess_dia/2-recess_depth
    with Locations((rx, oh/2 - recess_inset, h/2)):
            Sphere(radius=recess_dia/2, mode=Mode.SUBTRACT)
    with Locations((-rx, oh/2 - recess_inset, h/2)):
            Sphere(radius=recess_dia/2, mode=Mode.SUBTRACT)
    # screw holes
    with BuildSketch(Plane.XY):
        with Locations((0, wth/2)):
            with GridLocations(pin_cc, 1, 3, 1):
                Circle(radius=3.2/2)
    extrude(amount=outer_h, mode=Mode.SUBTRACT)
    # mount holes
    with BuildSketch(Plane.XY):
        with GridLocations(mhole_cc, 1, 2, 1):
            Circle(radius=mhole_d/2)
    extrude(amount=outer_h, mode=Mode.SUBTRACT)

print("Overall dimensions", mhole_cc + 2*mhole_d, "x", oh)
print("Lid", ow, "x", oh)
print("Insert holes c/c", ow-2*ins_is, oh-2*ins_is)

epilogue(p)
