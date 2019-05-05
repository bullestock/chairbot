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
    return up(th)(cylinder(d = sd, h = 2))

def studhole():
    inner = down(1)(cylinder(d = 2.5, h = 4+th)) + up(-0.1)(cylinder(d1 = 5, d2 = 2, h = 2))
    ww = 4
    c = translate([-ww/2, -ww, -1])(cube([ww, 2*ww, 20]))
    return intersection()(c, inner)

def assembly():
    s = stud()
    sh = studhole()
    d = 60
    w = sd + 1
    l = d + sd
    h1 = 3
    h = down(1)(cylinder(d = 3, h = th + 1)) + up(th - h1 + 0.1)(cylinder(d1 = 3, d2 = 7, h = h1))
    studs = left(d/2)(s) + right(d/2)(s)
    studholes = left(d/2)(sh) + right(d/2)(sh)
    dd = d - 20
    holes = left(dd/2)(h) + right(dd/2)(h)
    return studs + translate([-l/2, -w/2, 0])(cube([l, 2*w, th])) - forward(w)(holes) - studholes

if __name__ == '__main__':
    a = assembly()
    scad_render_to_file(a, file_header='$fn = %s;' % SEGMENTS, include_orig_code=False)
