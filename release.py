#!/usr/bin/env python3

import argparse
import subprocess as sp
from distutils.version import StrictVersion
from os.path import dirname, abspath, join
import sys
import re
import xml.etree.ElementTree


THIS_DIR = abspath(dirname(__file__))
VERSION_FILE = join(THIS_DIR, 'include', 'pitaya_version.h')
VERSION_FILE_UNITY = join(THIS_DIR, 'unity', 'PitayaExample', 'LibPitaya.nuspec')


def parse_args():
    parser = argparse.ArgumentParser(description='Manage the version of Pitaya')
    subparsers = parser.add_subparsers(dest='command', help='Commands')
    subparsers.required = True

    parser_check = subparsers.add_parser('check', help='Check version against the lib')
    parser_check.add_argument('--unity', help='Check unity version', action='store_true')
    parser_check.add_argument('--against', help='The version to check against', required=False)

    parser_new = subparsers.add_parser('new', help='Build for Windows')
    parser_new.add_argument('version', help='The new version')
    parser_new.add_argument('--unity', help='If should create a new unity version', action='store_true')
    parser_new.add_argument('--release-notes', dest='release_notes', help='Release notes to the unity release')

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


def get_curr_version_unity():
    e = xml.etree.ElementTree.parse(VERSION_FILE_UNITY).getroot()
    return e.find('metadata').find('version').text


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


def save_new_unity_version(version, release_notes):
    print('Saving new unity version: {}'.format(version))
    tree = xml.etree.ElementTree.parse(VERSION_FILE_UNITY)
    version_el = tree.getroot().find('metadata').find('version')
    version_el.text = version

    if release_notes != None:
        release_notes_el = tree.getroot().find('metadata').find('releaseNotes')
        release_notes_el.text = release_notes

    tree.write(VERSION_FILE_UNITY, xml_declaration=True, encoding='utf-8')


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


def check_release_unity(against_version):
    curr_version = get_curr_version_unity()

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

    save_new_version(new_version)


def new_release_unity(new_version, release_notes):
    curr_version = get_curr_version_unity()

    try:
        if StrictVersion(new_version) <= StrictVersion(curr_version):
            print('the new version should be larger than the older version (new {}, old {})'
                .format(new_version, curr_version))
            exit(1)
    except ValueError:
        print('invalid version string ({} <= {})'.format(new_version, curr_version))
        exit(1)

    save_new_unity_version(new_version, release_notes)


def main():
    args = parse_args()

    if args.command == 'check':
        if args.unity:
            check_release_unity(args.against)
        else:
            check_release(args.against)
    elif args.command == 'new':
        if args.unity:
            new_release_unity(args.version, args.release_notes)
        else:
            new_release(args.version)


if __name__ == '__main__':
    main()
