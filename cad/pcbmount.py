#! /usr/bin/env python
# -*- coding: utf-8 -*-
from __future__ import division
import os
import sys
import re

# Assumes SolidPython is in site-packages or elsewhwere in sys.path
from solid2 import *
#from solid2.utils import *

SEGMENTS = 500
set_global_fn(100)

sd = 8
th = 5
hole_cc = 57.6278


def stud():
    return up(th)(cylinder(d = sd, h = 2))

def studhole():
    inner = down(1)(cylinder(d = 4.2, h = 2*th))
    return inner

def assembly():
    s = stud()
    sh = studhole()
    d = 60
    w = sd + 1
    l = d + sd
    h1 = 3
    h = down(1)(cylinder(d = 3, h = th + 1)) + up(th - h1 + 0.1)(cylinder(d1 = 3, d2 = 7, h = h1))
    studs = left(hole_cc/2)(s) + right(hole_cc/2)(s)
    studholes = left(hole_cc/2)(sh) + right(hole_cc/2)(sh)
    dd = d - 20
    holes = left(dd/2)(h) + right(dd/2)(h)
    return studs + translate([-l/2, -w/2, 0])(cube([l, 2*w, th])) - forward(w)(holes) - studholes

if __name__ == '__main__':
    a = assembly()
    a.save_as_scad()
    #scad_render_to_file(a, file_header='$fn = %s;' % SEGMENTS, include_orig_code=False)
