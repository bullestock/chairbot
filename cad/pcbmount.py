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
    return up(th)(cylinder(d = sd, h = 2)) - down(1)(cylinder(d = 2.5, h = 4+th))

def assembly():
    s = stud()
    d = 60
    w = sd + 1
    l = d + sd
    h1 = 3
    h = down(1)(cylinder(d = 3, h = th + 1)) + up(th - h1 + 0.1)(cylinder(d1 = 3, d2 = 7, h = h1))
    studs = left(d/2)(s) + right(d/2)(s)
    dd = d - 20
    holes = left(dd/2)(h) + right(dd/2)(h)
    return studs + translate([-l/2, -w/2, 0])(cube([l, w, th])) - holes

if __name__ == '__main__':
    a = assembly()
    scad_render_to_file(a, file_header='$fn = %s;' % SEGMENTS, include_orig_code=False)
