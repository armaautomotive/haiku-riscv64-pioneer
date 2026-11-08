#!/usr/bin/env python3
"""Exercise the actual candidate client's syscall error capture without a device."""
from pathlib import Path
import subprocess
import tempfile

source = (Path(__file__).resolve().parent.parent / 'pioneer_q38_qgw8_full_diagnostic.cpp').read_text()
transport = source[source.index('struct Transport {'):source.index('static uint64_t Now()')]
report = source[source.index('static void ReportTransportFailure('):source.index('int main(')]
program = r'''
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstddef>
#define O_RDWR 2
#define O_CLOEXEC 4
static int open_result=5,ioctl_result=0,close_result=0,simulated_errno=0;
static int fake_open(const char*,int){errno=simulated_errno;return open_result;}
static int fake_ioctl(int,unsigned,void*,size_t){errno=simulated_errno;return ioctl_result;}
static int fake_close(int){errno=simulated_errno;return close_result;}
#define open fake_open
#define ioctl fake_ioctl
#define close fake_close
''' + transport + report + r'''
int main(){
 Transport io;
 simulated_errno=EACCES;open_result=-1;assert(io.Open()==-1 && io.openError==EACCES);
 simulated_errno=EIO;ioctl_result=-1;assert(!io.Call(5,1234,nullptr,0));
 assert(io.lastOperation==1234 && io.lastError==EIO);
 simulated_errno=EBUSY;close_result=-1;assert(io.Close(5)==-1 && io.closeError==EBUSY);
 errno=EINVAL;ReportTransportFailure(io); // Later calls cannot overwrite saved errors.
 ioctl_result=0;simulated_errno=EACCES;assert(io.Call(5,5678,nullptr,0));
 assert(io.lastOperation==5678 && io.lastError==0); // Successful calls must not report stale errno.
 return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='qgw8-transport-test-') as directory:
    source_path = Path(directory) / 'test.cpp'
    binary = Path(directory) / 'test'
    source_path.write_text(program)
    subprocess.run(['c++', '-std=c++17', '-Wall', '-Wextra', '-Werror', str(source_path), '-o', str(binary)], check=True)
    result = subprocess.run([str(binary)], text=True, capture_output=True, check=True)
    assert 'HYBRID_OPEN_FAILURE errno=13;' in result.stderr
    assert 'HYBRID_IOCTL_FAILURE operation=1234 errno=5;' in result.stderr
    assert 'HYBRID_CLOSE_FAILURE errno=16;' in result.stderr
print('PASS actual transport error capture; synthetic syscalls only, no card access')
