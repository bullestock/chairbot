from pathlib import Path
from build123d import *
from ocp_vscode import *

th = 6
od = 160
slipring_d = 12.75
slipring_hole_radius = 18/2
slipring_hole_d = 3.2
support_x_dist = 50
support_y_dist = 107
support_hole_d = 8.5
mounting_hole_radius = 65.25
mounting_hole_d = 5.5
bearing_mount_hole_radius = 50/2
bearing_mount_hole_d = 4.2

with BuildPart() as p:
    # disc
    with BuildSketch():
        Circle(radius=od/2)
    extrude(amount=th)
    e = p.edges().sort_by(Axis.Z)
    fillet(e[2], 3)
    # slipring holes
    with BuildSketch():
        Circle(radius=slipring_d/2)
        with PolarLocations(radius=slipring_hole_radius, count=3, start_angle=360/6):
            Circle(radius=slipring_hole_d/2)
    extrude(amount=th, mode=Mode.SUBTRACT)
    # stepper mount holes
    with Locations(p.faces().sort_by(Axis.Z)[0]):
        with Locations((-8 + 3.2/2, +(34 + 3.2/2)+3), (23 + 3.2/2, +(34 + 3.2/2)+3)):
            CounterSinkHole(radius=3.2/2, counter_sink_radius=6/2)
    with Locations(p.faces().sort_by(Axis.Z)[0]):
        # support holes
        with Locations((support_x_dist/2, -support_y_dist/2),
                       (-support_x_dist/2, -support_y_dist/2),
                       (-support_x_dist/2, support_y_dist/2),
                       (support_x_dist/2 + 12.5, support_y_dist/2)):
            CounterSinkHole(radius=support_hole_d/2, counter_sink_radius=18.25/2)
    with Locations(p.faces().sort_by(Axis.Z)[-1]):
        # gear mount holes
        with PolarLocations(radius=mounting_hole_radius, count=3, start_angle=90):
            CounterSinkHole(mounting_hole_d/2, 10/2)

show(p, reset_camera=Camera.KEEP)

export_step(p.part, f'{Path(__file__).stem}.step')
