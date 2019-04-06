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
    return cylinder(d = sd, h = 3) - down(1)(cylinder(d = 3, h = 5))

def studpair():
    return stud() + right(40)(stud())

def brace():
    return up(5)(studpair()) + translate([-13, -3.5, 0])(cube([57, 7, 5]))

def assembly():
    b1 = brace()
    b2 = brace()
    b3 = brace()
    b4 = brace()
    #braces = b1 + back(40)(b2) + back(13.34+40)(b3) + back(13.34+80)(b2)
    #plate = translate([-15, -107, 0])(cube([5, 120, 25]))
    # TEST
    braces = b1
    plate = translate([-15, -10, 0])(cube([5, 20, 25]))
    return braces + plate

if __name__ == '__main__':
    a = assembly()
    scad_render_to_file(a, file_header='$fn = %s;' % SEGMENTS, include_orig_code=False)
