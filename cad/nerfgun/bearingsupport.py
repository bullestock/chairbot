from build123d import *
from ocp_vscode import *

b_od = 22.35
b_th = 7
mount_od = 14
mount_th = 3
b_crush = 0.35
th = b_th + mount_th + 5
rr = 4
insert_r = 9.2/2
insert_l = 12.5
stepper_w = 43
stepper_h = 43 + 10

CUTOUT = True

pts = [
    (0, 0),
    (5, 40),
    (25, 85),
    (37.5, 110),
    (55, 120),
    (75, 105),
    (87.5, 45),
    (90, 10),
    (90, 0)
]

hole_loc = (55, 100)

with BuildPart() as p:
    with BuildSketch():
        with BuildLine():
            l1 = Spline(pts)
            Line([pts[-1], pts[0]])
        make_face()
    extrude(amount=th)
    # bearing hole
    with BuildSketch(Plane.XY.offset(th)):
        with Locations(hole_loc):
            Circle(radius=b_od/2 + b_crush/2)
    extrude(amount=-b_th, mode=Mode.SUBTRACT)
    # space for bearing mount
    with BuildSketch(Plane.XY.offset(th - b_th)):
        with Locations(hole_loc):
            Circle(radius=mount_od/2)
    extrude(amount=-mount_th, mode=Mode.SUBTRACT)
    # crush ribs
    with BuildSketch(Plane.XY.offset(th)):
        with Locations(hole_loc):
            with PolarLocations(radius=b_od/2 + b_crush/2, count=10):
                Circle(b_crush)
    extrude(amount=-b_th)
    # fillet
    fillet(p.edges().sort_by(Axis.Z)[-2], radius=rr)
    fillet(p.edges().sort_by(Axis.Z)[1], radius=rr)
    # insert holes
    with BuildSketch(p.faces().sort_by(Axis.Y)[0]):
        if CUTOUT:
            with Locations((25, 0), (-37.5, 0)):
                Circle(radius=insert_r)
        else:
            with PolarLocations(radius=25, count = 2):
                Circle(radius=insert_r)
    extrude(amount=-insert_l, mode=Mode.SUBTRACT)
    # assorted holes
    with BuildSketch(p.faces().sort_by(Axis.Z)[0]):
        with Locations((0, 10)):
            if not CUTOUT:
                Circle(radius=12.5)
            with PolarLocations(radius=27.5, count = 4, start_angle=-110):
                Circle(radius=6)
    extrude(amount=-th, mode=Mode.SUBTRACT)
    e = p.edges().sort_by(Axis.Z)
    s = []
    for i in range(1, 6):
        s.append(e[i])
    fillet(s, radius=2)
    s = []
    if CUTOUT:
        for i in range(70, 72):
            s.append(e[i])
        s.append(e[92])
    else:
        for i in range(70, 75):
            s.append(e[i])
        s.append(e[95])
    fillet(s, radius=2)
    if CUTOUT:
        # motor cutout
        with BuildSketch(Plane.XY):
            offset = 10
            with Locations((hole_loc[0] - 1.5, stepper_h/2 - offset/2)):
                RectangleRounded(stepper_w, stepper_h + offset, 3)
        extrude(amount=th, mode=Mode.SUBTRACT)

show(p, reset_camera=Camera.KEEP)

export_step(p.part, 'bearingsupport-l.step')
export_step(p.part.mirror(), 'bearingsupport-r.step')
