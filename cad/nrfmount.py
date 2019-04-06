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

w1 = 15.5
h1 = 10.7
l1 = 10
h2 = 3
l2 = 13

def block():
    return translate([-w1/2, 0, 0])(cube([w1, l1, h1]))

def tab():
    s = hole()(cylinder(h = 5, d = 1))
    d = 11.4
    d2 = 12
    return translate([-w1/2, 0, 0])(cube([w1, l2, h2]) +
                                    translate([w1/2 - d2/2, l2 - 1.5, 0])(s) +
                                    translate([w1/2 + d/2, l2 - 1.5, 0])(s))

def assembly():
    b = block()
    return b + translate([0, l1, h1 - h2])(tab())

if __name__ == '__main__':
    a = assembly()
    scad_render_to_file(a, file_header='$fn = %s;' % SEGMENTS, include_orig_code=False)
