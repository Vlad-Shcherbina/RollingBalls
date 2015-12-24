import ast
import sys
import multiprocessing
import os
import re
import subprocess
from timeit import default_timer
import multiprocessing
import pprint

import run_db


def run_solution(command, seed):
    try:
        start = default_timer()
        p = subprocess.Popen(
            'java -jar tester/tester.jar -exec "{}" '
            '-novis -seed {}'.format(command, seed),
            shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

        out, err = p.communicate()
        out = out.decode()
        err = err.decode()

        p.wait()
        assert p.returncode == 0

        result = dict(
            seed=str(seed),
            time=default_timer() - start)

        for line in out.splitlines() + err.splitlines():
            m = re.match(r'Score = (.+)$', line)
            if m is not None:
                result['Score'] = float(m.group(1))

            m = re.match(r'# (\w+) = (.*)$', line)
            if m is not None:
                result[m.group(1)] = ast.literal_eval(m.group(2))

        assert 'Score' in result
        if 'score' in result:
            assert abs(result['Score'] - result['score']) < 1e-5
        return result

    except Exception as e:
        raise Exception('seed={}, out={}, err={}'.format(seed, out, err)) from e


def worker(task):
    return run_solution(*task)


def main():
    subprocess.check_call(
        #'g++ --std=c++11 -Wall -Wno-sign-compare -O2 main.cc -o main',
        'g++ --std=c++0x -W -Wall -Wno-sign-compare '
        '-DNDEBUG '
        '-O2 -s -pipe -mmmx -msse -msse2 -msse3 main.cpp -o main',
        shell=True)
    command = './main'

    tasks = [(command, seed) for seed in range(100, 200)]

    map = multiprocessing.Pool(5).imap

    with run_db.RunRecorder() as run:
        for result in map(worker, tasks):
            print(result['seed'], result['score'])
            run.add_result(result)
            run.save()


if __name__ == '__main__':
    main()
