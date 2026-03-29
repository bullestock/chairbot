from build123d import *
#from epilogue import *
#from ocp_vscode import *

stepper_w = 36
stepper_h = 40
y_offset = 0
slit_w = 22
th = 4+2
hole_d = 3.2
s_hole_cc = 20
m_hole_cc = 31
flange_h = 13
insert_r = 4.0/2
insert_offset = 3

with BuildPart() as p:
    with BuildSketch():
        RectangleRounded(stepper_w, stepper_h + y_offset, 1)
    extrude(amount=th)
    with BuildSketch(Plane.XY):
        with Locations((0, y_offset)):
            Circle(slit_w/2)
        with Locations((0, y_offset)):
            with GridLocations(s_hole_cc, s_hole_cc, 2, 2):
                Circle(hole_d/2)
    extrude(amount=th, mode=Mode.SUBTRACT)
    with BuildSketch():
        with Locations((0, -stepper_h/2 - 0.5)):
            Rectangle(stepper_w, th)
    extrude(amount=flange_h + th)
    fillet(p.edges().filter_by(Axis.X), radius=2)
    # insert holes
    with BuildSketch(p.faces().sort_by(Axis.Y)[0]):
        with Locations((0, insert_offset)):
            with GridLocations(m_hole_cc, 0, 2, 1):
                Circle(radius=insert_r)
    extrude(amount=-th, mode=Mode.SUBTRACT)

#show(p, reset_camera=Camera.KEEP)

export_step(p.part, 'xsteppermount.step')
