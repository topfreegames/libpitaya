#!/usr/bin/env python3

import argparse
import os
import os.path as path
import subprocess
import signal
import sys
import time

THIS_DIR = path.abspath(path.dirname(__file__))

if sys.platform == 'win32' or sys.platform == 'cygwin':
    TESTS_EXE = 'tests.exe'
else:
    TESTS_EXE = 'tests'

MOCK_SERVERS_DIR = path.join(THIS_DIR, 'test', 'mock-servers')
MOCK_SERVERS = [
    ('mock-compression-server.js', 'mock-compression-server-log'),
    ('mock-disconnect-server.js', 'mock-disconnect-server-log'),
    ('mock-kick-server.js', 'mock-kick-server-log'),
    ('mock-timeout-server.js', 'mock-timeout-server-log'),
    ('mock-destroy-socket-server.js', 'mock-destroy-socket-server-log'),
    ('mock-kill-client-server.js', 'mock-kill-client-server-log'),
]

mock_server_processes = []
file_descriptors = []

def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--node-path', dest='node_path', help='Where is node located')
    parser.add_argument('--tests-dir', dest='tests_dir',
                        help='Where the test executable is located',
                        required=True)
    return parser.parse_known_args()


def kill_all_servers():
    for p in mock_server_processes: p.terminate()


def close_file_descriptors():
    for fd in file_descriptors: fd.close()


def signal_handler(signal, frame):
    kill_all_servers()
    close_file_descriptors()


def start_mock_servers(node_path):
    os.chdir(MOCK_SERVERS_DIR)
    for (s, l) in MOCK_SERVERS:
        print('Starting {}...'.format(s))
        fd = open(path.join(MOCK_SERVERS_DIR, l), 'wb')

        if node_path:
            process = subprocess.Popen(
                [node_path, s], stdout=fd, stderr=fd,
            )
        else:
            process = subprocess.Popen(
                ['node', s], stdout=fd, stderr=fd,
            )

        mock_server_processes.append(process)
        file_descriptors.append(fd)
    os.chdir(THIS_DIR)


def ensure_tests_executable(tests_dir):
    tests_path = path.join(tests_dir, TESTS_EXE)
    if not path.exists(tests_path):
        print('Tests executable {} does not exist.'.format(tests_path))
        exit(1)


def run_tests(tests_dir):
    os.chdir(tests_dir)
    if sys.platform == 'win32' or sys.platform == 'cygwin':
        args = '{} {}'.format(TESTS_EXE, ' '.join(sys.argv))
    else:
        args = './{} {}'.format(TESTS_EXE, ' '.join(sys.argv))
    code = subprocess.call(args, shell=True)
    os.chdir(THIS_DIR)
    return code


def main():
    args, unknown_args = parse_args()
    sys.argv = unknown_args

    if args.node_path and not path.exists(args.node_path):
        print("Node path does not exist.")
        exit(1)

    signal.signal(signal.SIGTERM, signal_handler)
    signal.signal(signal.SIGINT, signal_handler)

    ensure_tests_executable(args.tests_dir)
    start_mock_servers(args.node_path)
    time.sleep(1)
    code = run_tests(args.tests_dir)
    kill_all_servers()
    close_file_descriptors()
    exit(code)


if __name__ == '__main__':
    main()
