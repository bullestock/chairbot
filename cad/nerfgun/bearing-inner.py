from pathlib import Path
from build123d import *
from ocp_vscode import *

bearing_th = 13
base_th = 3
th = bearing_th + base_th
bearing_od = 69
base_od = 80
slipring_d = 14
bearing_mount_hole_radius = 50/2
bearing_mount_insert_d = 4.4
b_crush = 1.2

with BuildPart() as p:
    # cylinder
    with BuildSketch():
        Circle(radius=bearing_od/2)
    extrude(amount=bearing_th)
    with BuildSketch(p.faces().sort_by(Axis.Z).last):
        Circle(radius=base_od/2)
    extrude(amount=base_th)
    e = p.edges().sort_by(Axis.Z)
    fillet(e[0], 1)
    fillet(e[2], 1)
    # crush ribs
    with BuildSketch(p.faces().sort_by(Axis.Z)[0].offset(-b_crush)):
        with PolarLocations(radius=bearing_od/2 - b_crush/2 - 0.1, count=10):
            Circle(b_crush)
    extrude(amount=-bearing_th)
    # slipring hole
    with BuildSketch():
        Circle(radius=slipring_d/2)
    extrude(amount=th, mode=Mode.SUBTRACT)
    # mount holes
    with Locations(p.faces().sort_by(Axis.Z)[0]):
        with PolarLocations(radius=bearing_mount_hole_radius, count=3, start_angle=-30):
            CounterSinkHole(4.2/2, 8/2, th)

show(p, reset_camera=Camera.KEEP)

export_step(p.part, f'{Path(__file__).stem}.step')
