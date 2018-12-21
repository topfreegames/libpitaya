#!/usr/bin/env python3

import sys
import os
import xml.etree.ElementTree

if len(sys.argv) != 2:
    print('Usage: ./check-leaks.py <xml-file>')
    exit(1)

filepath = sys.argv[1]

if not os.path.exists(filepath):
    print('The file {} does not exist.'.format(filepath))
    exit(1)

e = xml.etree.ElementTree.parse(filepath).getroot()
errors = e.findall('error')

print('================  Valgrind output ({} errors) ================'.format(len(errors)))

leaks = []
possible_leaks = []

for err in errors:
    kind = err.findall('kind')[0]

    if kind.text == 'Leak_DefinitelyLost':
        leaks.append(err)
    elif kind.text == 'Leak_PossiblyLost':
        possible_leaks.append(err)

num_leaks = len(leaks)
num_possible_leaks = len(possible_leaks)

print('Leaks'.format(len(leaks)))
print('  * certain: {}'.format(num_leaks))
print('  * possible: {}'.format(num_possible_leaks))
print('==============================================================')

if num_leaks > 0:
    exit(1)
