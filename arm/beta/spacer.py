# %%
from build123d import *
from ocp_vscode import *

id, od, l = 6.5, 10, 5

with BuildPart() as o:
    with BuildSketch() as sk:
        Circle(radius=od/2)
    extrude(amount=l)
    with BuildSketch() as sk:
        Circle(radius=id/2)
    extrude(amount=10, mode=Mode.SUBTRACT)

show(o)    
export_step(o.part, "spacer.step")
