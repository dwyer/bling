#!/usr/bin/env python
# -*-coding:utf-8-*-
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import os
import subprocess

packages = [
    'builtin',
    'kc/token',
    'kc/ast',
    'kc/scanner',
    'kc/parser',
    'io/ioutil',
    'emit',
]

commands = [
    'cmd/compile',
]

def call(cmd):
    print(' '.join(cmd))
    subprocess.call(cmd)

def remove(names):
    call(['rm', '-f'] + names)

def get_files(path, ext=None):
    return [os.path.join(path, name) for name in os.listdir(path)
            if not ext or os.path.splitext(name)[-1] == ext]

def compile_package(path):
    objs = []
    c_files = get_files(path, '.c')
    for name in c_files:
        obj = os.path.splitext(name)[0] + '.o'
        objs.append(obj)
        call(['cc', '-I.', '-c', '-o', obj, name])
    return objs

objs = []

for path in packages:
    objs.extend(compile_package(path))

for path in commands:
    cmd_objs = compile_package(path)
    call(['cc', '-o', 'main'] + objs + cmd_objs)
    remove(cmd_objs)
remove(objs)
