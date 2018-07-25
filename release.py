#!/usr/bin/env python3

import argparse
import subprocess as sp
from distutils.version import StrictVersion
from os.path import dirname, abspath, join
import sys
import re

VERSION_FILE = join(abspath(dirname(__file__)), 'include', 'pitaya_version.h')


def parse_args():
    parser = argparse.ArgumentParser(description='Manage the version of Pitaya')
    subparsers = parser.add_subparsers(dest='command', help='Commands')
    subparsers.required = True

    parser_check = subparsers.add_parser('check', help='Check version against the lib')
    parser_check.add_argument('--against', help='The version to check against', required=False)

    parser_new = subparsers.add_parser('new', help='Build for Windows')
    parser_new.add_argument('version', help='The new version')

    return parser.parse_args(sys.argv[1:])


def get_curr_version():
    with open(VERSION_FILE) as f:
        lines = f.readlines()
        lines = list(map(lambda l: l.rstrip('\n'), lines))
        lines = list(filter(lambda l: len(l) > 0, lines))

    assert(len(lines) == 4)

    major = lines[0].replace('#define PC_VERSION_MAJOR', '').strip()
    minor = lines[1].replace('#define PC_VERSION_MINOR', '').strip()
    revision = lines[2].replace('#define PC_VERSION_REVISION', '').strip()
    suffix = lines[3].replace('#define PC_VERSION_SUFFIX', '').strip().lstrip('"').rstrip('"')

    return '{}.{}.{}{}'.format(major, minor, revision, suffix)


def save_new_version(version):
    print('Saving new version {}'.format(version))
    comp = version.split('.')
    assert(len(comp) == 3)

    # Extract suffix
    r = re.search(r'\d+(a\d+|b\d+)', comp[2])
    if r == None:
        # There is not suffix
        suffix = ""
    else:
        suffix = r.groups()[0]
    revision = comp[2].replace(suffix, '')

    with open(VERSION_FILE, 'w') as f:
        f.write('#define PC_VERSION_MAJOR {}\n'.format(comp[0]))
        f.write('#define PC_VERSION_MINOR {}\n'.format(comp[1]))
        f.write('#define PC_VERSION_REVISION {}\n'.format(revision))
        f.write('#define PC_VERSION_SUFFIX "{}"'.format(suffix))


def check_release(against_version):
    curr_version = get_curr_version()

    if against_version == None or against_version == '':
        print(curr_version)
        return

    try:
        if StrictVersion(against_version) != StrictVersion(curr_version):
            print('The build version should be equal to the lib version (build {}, lib {})'
                .format(against_version, curr_version))
            exit(1)
    except ValueError:
        print('invalid version string ({} != {})'.format(against_version, curr_version))
        exit(1)


def new_release(new_version):
    curr_version = get_curr_version()

    try:
        if StrictVersion(new_version) <= StrictVersion(curr_version):
            print('the new version should be larger than the older version (new {}, old {})'
                .format(new_version, curr_version))
            exit(1)
    except ValueError:
        print('invalid version string ({} <= {})'.format(new_version, curr_version))
        exit(1)

    status_text = str(sp.check_output('git status', shell=True))
    if not re.search(r".*nothing to commit, working tree clean.*", status_text):
        print('Please commit all changes first.')
        exit(1)

    save_new_version(new_version)

    sp.call('git add include/pitaya_version.h', shell=True)
    sp.call('git commit -m"New release {}"'.format(new_version), shell=True)
    sp.call('git tag {}'.format(new_version), shell=True)
    print('tag {} created, now you can push your changes'.format(new_version))


def main():
    args = parse_args()

    if args.command == 'check':
        check_release(args.against)
    elif args.command == 'new':
        new_release(args.version)


if __name__ == '__main__':
    main()
