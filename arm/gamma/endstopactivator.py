from build123d import *
from ocp_vscode import *

length, width, height = 12, 16, 10
hole_dia = 4.2

bottom = (Align.CENTER, Align.CENTER, Align.MIN)

with BuildPart() as o:
    Box(length, width, height)
    fillet(o.edges().filter_by(Axis.Z), radius=3)
    fillet(o.edges().sort_by(Axis.Z)[-1], radius=2)
    with Locations(o.faces().sort_by(Axis.Z).last):
        CounterSinkHole(radius=hole_dia/2, counter_sink_radius=4)
show(o)

export_step(o.part, 'endstopactivator.step')
