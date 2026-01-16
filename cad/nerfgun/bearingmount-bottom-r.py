from build123d import *
from ocp_vscode import *

th = 1
id = 2.7
od = 12
stem_od = 5.25
stem_l = 10
rr = 0.5
g_w = 1.5
g_d = 0

with BuildPart() as p:
    # disc
    with BuildSketch():
        Circle(radius=od/2)
    extrude(amount=th + g_d)
    # stem
    with BuildSketch(p.faces().sort_by(Axis.Z).last):
        Circle(radius=stem_od/2)
    extrude(amount=stem_l)
    # hole
    with BuildSketch():
        Circle(radius=id/2)
    extrude(amount=100, mode=Mode.SUBTRACT)

show(p)

export_step(p.part, 'bearingmount-bottom-r.step')
