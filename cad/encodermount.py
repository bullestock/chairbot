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
    return cylinder(r = 25, h = 2) - down(0.5)(cylinder(r = 13, h = 3))

def screwhole():
    return translate([0, 12, 5])(cylinder(r = 1.2, h = 15))

def assembly():
    sd = 13
    return partA()

if __name__ == '__main__':
    a = assembly()
    scad_render_to_file(a, file_header='$fn = %s;' % SEGMENTS, include_orig_code=False)
