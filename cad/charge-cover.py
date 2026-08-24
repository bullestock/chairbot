# %%
from build123d import *
from ocp_vscode import *
from epilogue import *

iw = 124.5
h = 18
arc_ir = 20
th = 3

ALIGN_MIN = (Align.MIN, Align.MIN)
ALIGN_X = (Align.MIN, Align.CENTER)

center = (0, 0)
alpha = 26
x1 = 6
y1 = 6

recess_dia = 5

with BuildPart() as p:
    with BuildSketch(Plane.XY):
        with BuildLine():
            a2 = CenterArc(center=center, radius=arc_ir+th, start_angle=-alpha, arc_size=2*alpha)
            Line((-x1, -y1), a2.start_point())
            Line((-x1, -y1), (-x1, y1))
            Line((-x1, y1), a2.end_point())
        make_face()
    extrude(amount=th)
    with BuildSketch(Plane.XY.offset(th)):
        with BuildLine():
            a1 = CenterArc(center=center, radius=arc_ir, start_angle=-alpha, arc_size=2*alpha)
            a2 = CenterArc(center=center, radius=arc_ir+th, start_angle=-alpha, arc_size=2*alpha)
            Line(a1.start_point(), a2.start_point())
            Line(a1.end_point(), a2.end_point())
        make_face()
    extrude(amount=iw)
    with BuildSketch(Plane.XY.offset(th+iw)):
        with BuildLine():
            a2 = CenterArc(center=center, radius=arc_ir+th, start_angle=-alpha, arc_size=2*alpha)
            Line((-x1, -y1), a2.start_point())
            Line((-x1, -y1), (-x1, y1))
            Line((-x1, y1), a2.end_point())
        make_face()
    extrude(amount=th)
    with Locations((0, 0, recess_dia/2)):
            Sphere(radius=recess_dia/2)
    with Locations((0, 0, iw+recess_dia/2)):
            Sphere(radius=recess_dia/2, mode=Mode.SUBTRACT)
    
    print(p.part.bounding_box())

epilogue(p)
