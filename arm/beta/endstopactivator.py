# %%
from build123d import *
from ocp_vscode import *

id, od, l = 6.5, 20, 8
bd = 10

with BuildPart() as o:
    with BuildSketch() as sk:
        Circle(radius=od/2)
    extrude(amount=l)
    with BuildSketch() as sk:
        with Locations((5, 5)):
            Circle(radius=bd/2)
    extrude(amount=l)
    edges = o.edges().sort_by(Axis.Z)
    edgeidx = [ 1, 4, 5 ]
    fillet([edges[i] for i in edgeidx], radius=1)
    #fillet(o.edges().sort_by(Axis.Z).last, radius=1)
    with BuildSketch() as sk:
        Circle(radius=id/2)
    extrude(amount=10, mode=Mode.SUBTRACT)
    with BuildSketch(Plane.XZ):
        with Locations((0, l/2)):
            Circle(radius=3.3/2)
    extrude(amount=10, mode=Mode.SUBTRACT)

show(o)    
export_step(o.part, "beta-endstopactivator.step")
