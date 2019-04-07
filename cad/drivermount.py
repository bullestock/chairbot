#! /usr/bin/env python
# -*- coding: utf-8 -*-
from __future__ import division
import os
import sys
import re

# Assumes SolidPython is in site-packages or elsewhwere in sys.path
from solid import *
from solid.utils import *

SEGMENTS = 32

sd = 7
th = 5

def stud():
    return cylinder(d = sd, h = 3) - hole()((down(7)(cylinder(d = 2.5, h = 15))))

def studpair():
    return stud() + right(40)(stud())

def brace():
    return up(5)(studpair()) + translate([-10, -3.5, 0])(cube([54, 7, 5]))

def assembly():
    b1 = brace()
    b2 = brace()
    b3 = brace()
    b4 = brace()
    braces = b1 + back(40)(b2) + back(13.34+40)(b3) + back(13.34+80)(b2)
    plate = translate([-15+4.3, -107, 0])(cube([5, 120, 25]))
    h1 = 3
    h = down(1)(cylinder(d = 3, h = th + 1)) + up(th - h1 + 0.1)(cylinder(d1 = 3, d2 = 7, h = h1))
    hr = rotate([90, 0, 90])(h)
    return braces + plate + h - translate([-10, 8, 12])(hr) - translate([-10, -102, 12])(hr)

if __name__ == '__main__':
    a = assembly()
    scad_render_to_file(a, file_header='$fn = %s;' % SEGMENTS, include_orig_code=False)
