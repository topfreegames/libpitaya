#!/usr/bin/env python3

import argparse
import os
import os.path as path
import subprocess
import signal
import sys
import time

THIS_DIR = path.abspath(path.dirname(__file__))

TESTS_DIR = path.join(THIS_DIR, 'build', 'out', 'Release_x64', 'output')
TESTS_EXE = 'tests'

COMPILE_DIR = path.join(THIS_DIR, 'build', 'out', 'Release_x64')

MOCK_SERVERS_DIR = path.join(THIS_DIR, 'test', 'mock-servers')
MOCK_SERVERS = [
    ('mock-compression-server.js', 'mock-compression-server-log'),
    ('mock-disconnect-server.js', 'mock-disconnect-server-log'),
    ('mock-kick-server.js', 'mock-kick-server-log'),
    ('mock-timeout-server.js', 'mock-timeout-server-log'),
    ('mock-destroy-socket-server.js', 'mock-destroy-socket-server-log'),
]

PITAYA_SERVERS_DIR = path.join(THIS_DIR, 'pitaya-servers') 
PITAYA_SERVERS = [
    (path.join('json-server', 'server-exe'), path.join('json-server', 'server-exe-out')),
    (path.join('protobuf-server', 'server-exe'), path.join('protobuf-server', 'server-exe-out')),
]

mock_server_processes = []
pitaya_server_processes = []
file_descriptors = []


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--node-path', dest='node_path', help='Where is node located')
    parser.add_argument('--go-path', dest='go_path', help='Where is Go located.')
    return parser.parse_args()


def kill_all_servers():
    for p in mock_server_processes: p.terminate()
    for p in pitaya_server_processes: p.terminate()


def close_file_descriptors():
    for fd in file_descriptors: fd.close()


def signal_handler(signal, frame):
    kill_all_servers()
    close_file_descriptors()


def ensure_pitaya_servers(go_path):
    os.chdir(PITAYA_SERVERS_DIR)
    for (s, _) in PITAYA_SERVERS:
        os.chdir(path.dirname(s))
        if go_path:
            subprocess.call([
                go_path, 'build', '-o', 'server-exe', 'main.go', 
            ])
        else:
            subprocess.call([
                'go', 'build', '-o', 'server-exe', 'main.go', 
            ])
        os.chdir(PITAYA_SERVERS_DIR)
    os.chdir(THIS_DIR)


def start_pitaya_servers():
    os.chdir(PITAYA_SERVERS_DIR)
    port = 9091
    for (s, l) in PITAYA_SERVERS:
        os.chdir(path.dirname(s))
        print('Starting {}...'.format(s))

        os.environ['PITAYA_METRICS_PROMETHEUS_PORT'] = str(port)
        port += 1

        fd = open(path.join(PITAYA_SERVERS_DIR, l), 'wb')
        process = subprocess.Popen(
            './{}'.format(path.basename(s)), stdout=fd, stderr=fd)
        pitaya_server_processes.append(process)
        os.chdir(PITAYA_SERVERS_DIR)
    os.chdir(THIS_DIR)


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


def ensure_tests_executable():
    if not path.exists(COMPILE_DIR):
        print('Compile directory does not exist, run gyp first.')
        exit(1)

    os.chdir(COMPILE_DIR)
    print('Compiling tests executable')
    ret = subprocess.call('ninja')
    os.chdir(THIS_DIR)

    if ret != 0:
        print('Compilation failed')
        exit(1)

    assert(path.exists(path.join(TESTS_DIR, TESTS_EXE)))


def run_tests():
    os.chdir(TESTS_DIR)
    args = './{} {}'.format(TESTS_EXE, ' '.join(sys.argv[1:]))
    code = subprocess.call(args, shell=True)
    os.chdir(THIS_DIR)
    return code


def main():
    args = parse_args()

    remove_indices = []

    try:
        index = sys.argv.index('--node-path')
        if index >= 0:
            remove_indices.append(index)
            remove_indices.append(index+1)
    except ValueError:
        pass

    try:
        index = sys.argv.index('--go-path')
        if index >= 0:
            remove_indices.append(index)
            remove_indices.append(index+1)
    except ValueError:
        pass

    sys.argv = [i for j, i in enumerate(sys.argv) if j not in remove_indices]

    if args.node_path and not path.exists(args.node_path):
        print("Node path does not exist.")
        exit(1)

    if args.go_path and not path.exists(args.go_path):
        print("Go path does not exist.")
        exit(1)

    signal.signal(signal.SIGTERM, signal_handler)
    signal.signal(signal.SIGINT, signal_handler)

    ensure_tests_executable()
    start_mock_servers(args.node_path)
    ensure_pitaya_servers(args.go_path)
    start_pitaya_servers()
    time.sleep(1) 
    code = run_tests()
    kill_all_servers()
    close_file_descriptors()
    exit(code)


if __name__ == '__main__':
    main()