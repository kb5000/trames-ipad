# SPDX-License-Identifier: GPL-3.0-or-later
"""Read-only accounting for explicitly named host processes; no sampling thread."""
import ctypes as C
import os
from pathlib import Path
import sys


class ProcessGone(RuntimeError):
    pass


_apple_api = None


class AppleUsage(C.Structure):
    # Public SDK sys/resource.h rusage_info_v2, used by proc_pid_rusage(2).
    _fields_ = [('uuid', C.c_ubyte * 16)] + [(name, C.c_uint64) for name in (
        'user_time', 'system_time', 'pkg_idle_wkups', 'interrupt_wkups', 'pageins',
        'wired_size', 'resident_size', 'phys_footprint', 'proc_start_abstime',
        'proc_exit_abstime', 'child_user_time', 'child_system_time',
        'child_pkg_idle_wkups', 'child_interrupt_wkups', 'child_pageins',
        'child_elapsed_abstime', 'diskio_bytesread', 'diskio_byteswritten')]


def read(pid):
    global _apple_api
    if pid <= 0:
        raise ValueError('an explicit positive process ID is required')
    if sys.platform == 'darwin':
        if _apple_api is None:
            _apple_api = C.CDLL('/usr/lib/libproc.dylib', use_errno=True)
            _apple_api.proc_pid_rusage.argtypes = [C.c_int, C.c_int, C.c_void_p]
            _apple_api.proc_pid_rusage.restype = C.c_int
        usage = AppleUsage()
        if _apple_api.proc_pid_rusage(pid, 2, C.byref(usage)) != 0:
            code = C.get_errno()
            if code == 3:
                raise ProcessGone(f'process {pid} exited')
            raise OSError(code, os.strerror(code))
        if usage.proc_exit_abstime:
            raise ProcessGone(f'process {pid} exited')
        return dict(pid=pid, identity=usage.proc_start_abstime,
                    resident_bytes=usage.resident_size,
                    physical_footprint_bytes=usage.phys_footprint,
                    read_bytes=usage.diskio_bytesread, written_bytes=usage.diskio_byteswritten,
                    io_scope='named host process; descendants are not included')
    if sys.platform.startswith('linux'):
        root = Path('/proc') / str(pid)
        try:
            stat = (root / 'stat').read_text().rsplit(')', 1)[1].split()
            if stat[0] == 'Z':
                raise ProcessGone(f'process {pid} is a zombie')
            pages = int((root / 'statm').read_text().split()[1])
            io = {}
            try:
                io = dict(line.split(':', 1) for line in (root / 'io').read_text().splitlines())
            except PermissionError:
                pass
            return dict(pid=pid, identity=int(stat[19]),
                        resident_bytes=pages * os.sysconf('SC_PAGE_SIZE'),
                        physical_footprint_bytes=None,
                        read_bytes=int(io['read_bytes']) if 'read_bytes' in io else None,
                        written_bytes=int(io['write_bytes']) if 'write_bytes' in io else None,
                        io_scope='named host process; descendants are not included')
        except (FileNotFoundError, ProcessLookupError) as error:
            raise ProcessGone(f'process {pid} exited') from error
    raise RuntimeError('host process accounting supports Linux and Apple hosts')


class Process:
    def __init__(self, pid):
        self.pid = pid
        self.first = read(pid)

    def snapshot(self):
        result = read(self.pid)
        if result['identity'] != self.first['identity']:
            raise ProcessGone(f'process {self.pid} was replaced; refusing PID reuse')
        return result
