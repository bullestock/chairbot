from pathlib import Path
from build123d import *
from ocp_vscode import *

bearing_th = 13
base_th = 3
th = bearing_th + base_th
bearing_od = 110
gear_id = 118
gear_th = 19
b_crush = 1.2

with BuildPart() as p:
    with BuildSketch():
        Circle(radius=(gear_id - b_crush)/2)
    extrude(amount=gear_th)
    with BuildSketch():
        Circle(radius=(bearing_od + b_crush)/2)
    extrude(amount=bearing_th, mode=Mode.SUBTRACT)
    with BuildSketch():
        Circle(radius=(bearing_od/2 - 5))
    extrude(amount=(gear_th + bearing_th), mode=Mode.SUBTRACT)
    # outer crush ribs
    with BuildSketch(p.faces().sort_by(Axis.Z)[0].offset(-b_crush)):
        with PolarLocations(radius=gear_id/2 - b_crush + 0.25, count=10):
            Circle(b_crush)
    extrude(amount=-(gear_th - 2*b_crush))
    # inner crush ribs
    with BuildSketch(p.faces().sort_by(Axis.Z)[0].offset(-b_crush)):
        with PolarLocations(radius=bearing_od/2 + b_crush - 0.25, count=10):
            Circle(b_crush)
    extrude(amount=-(gear_th - 2*b_crush))
    

show(p, reset_camera=Camera.KEEP)

export_step(p.part, f'{Path(__file__).stem}.step')
