#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Exercise real streamed downloads, their progress delegate, and the native installer ABI."""
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import argparse
import os
import subprocess
import threading
import time

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--filter", default="ConnectivityRetryTests|DownloadProgressTests|InstallationProgressTests")
args = parser.parse_args()

class Handler(BaseHTTPRequestHandler):
    def do_GET(self):
        self.send_response(200)
        self.send_header("Content-Length", str(4 * 65536))
        self.end_headers()
        delays = [0, 20, 20, 25] if self.path == "/slow" else [0, 0.6, 0.6, 0.6]
        try:
            for delay in delays:
                time.sleep(delay)
                self.wfile.write(b"a" * 65536)
                self.wfile.flush()
        except (BrokenPipeError, ConnectionResetError):
            pass
    def log_message(self, *_):
        pass

server = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
threading.Thread(target=server.serve_forever, daemon=True).start()
library = os.environ.get("TRAMES_RUST_LIB_DIR", str(ROOT / "target/debug"))
environment = dict(os.environ, TRAMES_RUST_LIB_DIR=library, DYLD_LIBRARY_PATH=library,
                   TRAMES_BOOTSTRAP_TEST_URL=f"http://127.0.0.1:{server.server_port}")
try:
    result = subprocess.run(["xcrun", "swift", "test", "--package-path", "apps/ios/TramesKit", "--jobs", "3",
                             "--filter", args.filter],
                            cwd=ROOT, env=environment, timeout=300)
finally:
    server.shutdown()
raise SystemExit(result.returncode)
