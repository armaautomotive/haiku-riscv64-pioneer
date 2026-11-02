#!/usr/bin/env python3
"""Own exact db03/041 fixture endpoints; all unchanged phase controls first."""
import pathlib
import subprocess
import sys

assert sys.argv[1:] == []
subprocess.run([sys.executable, str(pathlib.Path(__file__).resolve().with_name(
    'pioneer_q38_frontier_failure_link_exchange_test.py')), '--constant-q4-endpoints'], check=True)
