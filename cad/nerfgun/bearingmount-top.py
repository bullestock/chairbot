from build123d import *
from ocp_vscode import *

th = 7
b_id = 8
lip_od = 12
lip_th = 2
hole_dia = 3
cs_dia = 5
cs_depth = 4
b_crush = 0.35
rr = 0.5

with BuildPart() as p:
    # inner
    with BuildSketch():
        Circle(radius=b_id/2 - b_crush/2)
    extrude(amount=th)
    # bearing crush ribs
    with BuildSketch(p.faces().sort_by(Axis.Z)[0].offset(0)):
        with PolarLocations(radius=b_id/2 - b_crush/2, count=10):
            Circle(b_crush)
    extrude(amount=-th)
    # lip
    with BuildSketch(p.faces().sort_by(Axis.Z).last):
        Circle(radius=lip_od/2)
    extrude(amount=lip_th)
    with Locations(p.faces().sort_by(Axis.Z)[-1]):
        CounterBoreHole(radius=hole_dia/2, counter_bore_radius=cs_dia/2, counter_bore_depth=cs_depth, depth=50)
    fillet(p.edges().sort_by(Axis.Z)[-2], radius=.75)

show(p)

export_step(p.part, 'bearingmount-top.step')
