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

def partA():
    return cylinder(r = 22, h = 2) - down(0.1)(cylinder(r = 12.5, h = 2.2))

def partB():
    return translate([-12.5, 9, 0])(cube([25, 10, 5]))

def screwhole():
    return translate([0, 12, -1])(cylinder(r = 1, h = 7))

def assembly():
    return partA() + partB() - left(5)(screwhole()) - right(5)(screwhole())

if __name__ == '__main__':
    a = assembly()
    scad_render_to_file(a, file_header='$fn = %s;' % SEGMENTS, include_orig_code=False)
