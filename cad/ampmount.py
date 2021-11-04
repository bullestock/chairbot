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

def assembly():
    d = 10
    p1 = translate([0, -9, 0])(cube([15, 18, d]))
    p2 = translate([11, -7.5, -1])(cube([1.7, 15, d+2]))
    p3 = translate([9, -6.5, -1])(cube([7, 13, d+2]))
    return p1 - p2 - p3
# - p2 - p3

if __name__ == '__main__':
    a = assembly()
    scad_render_to_file(a, file_header='$fn = %s;' % SEGMENTS, include_orig_code=False)
