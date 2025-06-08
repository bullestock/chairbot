# %%
from build123d import *
from ocp_vscode import *

od, h, th = 10, 5, 3.5

with BuildPart() as o:
    with BuildSketch() as sk:
        Circle(radius=od/2)
    extrude(amount=th)
    split(bisect_by=Plane.XZ.offset(od/2 - h))

show(o)    
export_step(o.part, "beta-endstopbump.step")
