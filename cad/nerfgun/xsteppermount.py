from build123d import *
from ocp_vscode import *

stepper_w = 41
stepper_h = 43
y_offset = 2
slit_w = 22.5
th = 4+2
hole_d = 3.2
hole_cc = 31
flange_h = 10
insert_r = 4.0/2

with BuildPart() as p:
    with BuildSketch():
        RectangleRounded(stepper_w, stepper_h + y_offset, 1)
    extrude(amount=th)
    with BuildSketch(Plane.XY):
        with Locations((0, stepper_w*0.25)):
            RectangleRounded(slit_w, stepper_h, slit_w/2 - 0.5)
        with Locations((0, 4)):
            with GridLocations(hole_cc, hole_cc, 2, 2):
                RectangleRounded(hole_d, 4*hole_d, hole_d/2 - 0.1)
    extrude(amount=th, mode=Mode.SUBTRACT)
    with BuildSketch():
        with Locations((0, -stepper_h/2 - 0.5)):
            Rectangle(stepper_w, th)
    extrude(amount=flange_h + th)
    with BuildSketch(p.faces().sort_by(Axis.Y)[0]):
        with Locations((0, th/2 - 2)):
            with GridLocations(hole_cc, 0, 2, 1):
                Circle(radius=insert_r)
    extrude(amount=-th, mode=Mode.SUBTRACT)

show(p, reset_camera=Camera.KEEP)

export_step(p.part, 'xsteppermount.step')
