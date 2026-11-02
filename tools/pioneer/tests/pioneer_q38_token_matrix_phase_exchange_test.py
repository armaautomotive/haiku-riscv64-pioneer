#!/usr/bin/env python3
"""Exact5ae9+559f/041 synthetic exchange endpoints;all unchanged controls FIRST."""
import pathlib
import subprocess
import sys

assert sys.argv[1:]==[]
subprocess.run([sys.executable,str(pathlib.Path(__file__).resolve().with_name(
    'pioneer_q38_frontier_failure_link_exchange_test.py')),'--matrix-phase-endpoints'],check=True)
