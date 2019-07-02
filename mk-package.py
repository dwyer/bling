#!/usr/bin/env python
# -*-coding:utf-8-*-
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import os
import subprocess
import sys

DEBUG = True

INCLUDE_PATH = os.path.dirname(os.path.abspath(__file__))
CFLAGS = ['-I', INCLUDE_PATH]
LDFLAGS = []

if DEBUG:
    CFLAGS.append('-g')
    LDFLAGS.append('-g')

packages = [
    'builtin',
    'runtime',
    'subc/token',
    'subc/ast',
    'subc/scanner',
    'subc/parser',
    'subc/emitter',
    'io/ioutil',
]

commands = [
    'compile',
]

tests = ['test_map']

def call(cmd):
    print(' '.join(cmd))
    res = subprocess.check_call(cmd)
    if res:
        exit(res)

def remove(names):
    pass # call(['rm', '-f'] + names)

def get_files(path, ext=None):
    return [os.path.join(path, name) for name in os.listdir(path)
            if not ext or os.path.splitext(name)[-1] == ext]

def compile_package(path):
    objs = []
    c_files = get_files(path, '.c')
    for name in c_files:
        obj = os.path.splitext(name)[0] + '.o'
        objs.append(obj)
        call(['cc'] + CFLAGS + ['-c', '-o', obj, name])
    return objs

def compile_packages():
    objs = []
    for path in packages:
        objs.extend(compile_package(path))
    return objs

def compile_command(name, objs):
    path = os.path.join('cmd', name)
    cmd_objs = compile_package(path)
    call(['cc'] + LDFLAGS + ['-o', name] + objs + cmd_objs)
    return cmd_objs

def compile_test(name, objs):
    path = os.path.join('tests', name)
    cmd_objs = compile_package(path)
    call(['cc'] + LDFLAGS + ['-o', name] + objs + cmd_objs)
    return cmd_objs

objs = compile_packages()
for name in commands:
    remove(compile_command(name, objs))
for name in tests:
    remove(compile_test(name, objs))
remove(objs)
