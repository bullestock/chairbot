# %%
from build123d import *
from ocp_vscode import *
from epilogue import *

hex_d = 4.25 # was 4.2
hex_l = 3.5
outer_d = 15
th = 2
skirt_th = 8
outer_h = 12

with BuildPart() as p:
    # plate/skirt
    with BuildSketch(Plane.XY):
        Circle(radius=(outer_d+skirt_th)/2)
    extrude(amount=th+outer_h)
    with BuildSketch(last_z(p)):
        Circle(radius=outer_d/2)
    extrude(amount=-outer_h, mode=Mode.SUBTRACT)
    # skirt slits
    with BuildSketch(Plane.XY.offset(1.5*th)):
        with PolarLocations(radius=outer_d/2, count=12):
            Rectangle(10, 0.25)
    extrude(amount=outer_h, mode=Mode.SUBTRACT)
    filletz(p, .5)
    fillet(z_edges(p)[0], 2)
    # hex part
    with BuildSketch(Plane.XY.offset(th)):
        RegularPolygon(radius=hex_d/2, side_count=6, major_radius=False)
    extrude(amount=hex_l)
    # hex slits
    with BuildSketch(Plane.XY.offset(th)):
        with PolarLocations(radius=hex_d/2, count=6):
            Rectangle(5, 0.2)
    extrude(amount=outer_h, mode=Mode.SUBTRACT)
    
epilogue(p)
