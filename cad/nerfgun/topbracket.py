from build123d import *
from ocp_vscode import *

th = 6
width_i = 77
width_o = width_i + 2*th
height_i = 45
height_o = height_i + th
hole_offset = 8
hole_dia = 3
cs_dia = 5
cs_depth = 2
rr = 3
depth = 15

with BuildPart() as p:
    with BuildSketch(Plane.XY):
        Rectangle(width_o, height_o)
    extrude(amount=depth)
    with BuildSketch(Plane.XY):
        with Locations((0, -th/2)):
            Rectangle(width_i, height_i)
    extrude(amount=depth, mode=Mode.SUBTRACT)
    with Locations(p.faces().sort_by(Axis.X)[-1]):
        with Locations((-height_o/2 + hole_offset, 0)):
            CounterBoreHole(radius=hole_dia/2, counter_bore_radius=cs_dia/2, counter_bore_depth=cs_depth, depth=50)
    with Locations(p.faces().sort_by(Axis.X)[0]):
        with Locations((-height_o/2 + hole_offset, 0)):
            CounterBoreHole(radius=hole_dia/2, counter_bore_radius=cs_dia/2, counter_bore_depth=cs_depth, depth=50)
    fillet(p.edges().filter_by(Axis.Z), radius=2)
    fillet(p.edges().filter_by(Axis.Y), radius=2)
                            

show(p)

export_step(p.part, 'topbracket.step')
