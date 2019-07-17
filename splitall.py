#!/usr/bin/env python3

import os

files = []

with open('all.bling') as fp:
    file = None
    for line in fp:
        if line.startswith('//'):
            name = line[2:].strip()
            file = dict(name=name, lines=[])
            files.append(file)
        else:
            file['lines'].append(line)
for file in files:
    file['lines'] = ''.join(file['lines'])

pkgs = {}
for file in files:
    pkg, _ = os.path.split(file['name'])
    _, name = os.path.split(pkg)
    newfile = os.path.join(pkg, name + '.bling')
    if newfile not in pkgs:
        pkgs[newfile] = []
    pkgs[newfile].extend(file['lines'])
for pkg, lines in pkgs.items():
    print('writing %s' % pkg)
    with open(pkg, 'w') as fp:
        fp.writelines(lines)
