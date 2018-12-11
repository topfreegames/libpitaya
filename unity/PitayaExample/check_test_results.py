#!/usr/bin/env python3

import xml.etree.ElementTree
import sys

filepath = sys.argv[1]

e = xml.etree.ElementTree.parse(filepath).getroot()

total = int(e.attrib['total'])
passed = int(e.attrib['passed'])
failed = int(e.attrib['failed'])

if total == 0:
    print("No tests were run")
    exit(0)

print("Test results:")
print("  * Passed: {}/{}".format(passed, total))
print("  * Failed: {}/{}".format(failed, total))

if failed > 0:
    exit(1)


