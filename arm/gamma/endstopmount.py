# %%
from build123d import *
from ocp_vscode import *

bdx, bdy = 25, 21
hd = 21.2
th = 1.5
l = 21
hole_cc = 15
offset = (bdx - bdy)/2

hole_pos = [(hole_cc/2 + offset, 7), (-hole_cc/2 + offset, 7)]

with BuildPart() as o:
    with BuildSketch() as sk:
        RectangleRounded(bdx + 2*th, bdy + 2*th, 2)
    extrude(amount=l)
    with BuildSketch(o.faces().sort_by(Axis.Z).last) as h:
        with Locations((offset, 0)):
            Circle(radius=hd/2)
    extrude(amount=-l, mode=Mode.SUBTRACT)
    face = o.faces().sort_by(Axis.Y).last.offset(3)
    with BuildSketch(face):
        with Locations(hole_pos):
            RectangleRounded(5, 7, 2)
    extrude(amount=-3)
    with BuildSketch(face):
        with Locations(hole_pos):
            Circle(radius=2.6/2)
    extrude(amount=-10, mode=Mode.SUBTRACT)
    with BuildSketch(o.faces().sort_by(Axis.X).first):
        Circle(radius=3.3/2)
    extrude(amount=-10, mode=Mode.SUBTRACT)

show(o)    
export_step(o.part, "endstopmount.step")
