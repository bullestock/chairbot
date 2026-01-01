from build123d import *
from ocp_vscode import *

th = 2
id = 5.5
od = 12
rr = 0.5
g_w = 1.5
g_d = 2

with BuildPart() as p:
    # disc
    with BuildSketch():
        Circle(radius=od/2)
    extrude(amount=th + g_d)
    # hole
    with BuildSketch():
        Circle(radius=id/2)
    extrude(amount=th + g_d, mode=Mode.SUBTRACT)
    # bearing crush ribs
    with BuildSketch(p.faces().sort_by(Axis.Z).last):
        Rectangle(g_w, od)
        Rectangle(od, g_w)
    extrude(amount=-g_d, mode=Mode.SUBTRACT)

show(p)

export_step(p.part, 'bearingmount-bottom.step')
