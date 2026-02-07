from pathlib import Path
from build123d import *
from ocp_vscode import *

th = 2
id = 12.75
od = 24
hole_radius = 18/2
hole_d = 3.2
pcb_th = 1.75
pcb_clearance = 2
rr = 0.5
pcb_w = 13
pcb_len = 20.5
length = 42

with BuildPart() as p:
    # disc
    with BuildSketch():
        Circle(radius=od/2)
        with Locations(((length - od/2)/2, 0)):
            Rectangle(length - od/2, od)
    extrude(amount=th)
    # hole
    with BuildSketch():
        Circle(radius=id/2)
    extrude(amount=th, mode=Mode.SUBTRACT)
    # mounting holes
    with BuildSketch():
        with PolarLocations(radius=hole_radius, count=3):
            Circle(radius=hole_d/2)
    extrude(amount=th, mode=Mode.SUBTRACT)
    # pcb mount
    with BuildSketch():
        with Locations(((length - od/2)/2 + pcb_w, 0)):
            Rectangle(pcb_w, od)
    extrude(amount=th + 2*pcb_clearance + pcb_th)
    with BuildSketch(Plane.XY.offset(th)):
        with Locations(((length - od/2)/2 + pcb_w, 0)):
            Rectangle(pcb_w, pcb_len - 2*pcb_clearance)
    extrude(amount=2*pcb_clearance + pcb_th, mode=Mode.SUBTRACT)
    with BuildSketch(Plane.XY.offset(th + pcb_clearance)):
        with Locations(((length - od/2)/2 + pcb_w, 0)):
            Rectangle(pcb_w, pcb_len)
    extrude(amount=pcb_th, mode=Mode.SUBTRACT)
    e = p.edges().filter_by(Axis.Z).group_by(SortBy.LENGTH)
    print(len(e))
    fillet(e[0], 0.5)
    fillet(e[3], 1)
    # slot for wires
    with BuildSketch():
        with Locations((-od/2, 0)):
            Rectangle(od/2, 1)
    extrude(amount=th, mode=Mode.SUBTRACT)

show(p)

export_step(p.part, f'{Path(__file__).stem}.step')
