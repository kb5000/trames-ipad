#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Loopback TCP sendall repro; no site data, credentials or external network."""
import json
import socket
import threading


def check(size, chunk=None):
    payload = b'x' * size
    result = {'bytes': size, 'write_chunk': chunk, 'received': 0}
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as listener:
        listener.bind(('127.0.0.1', 0))
        listener.listen(1)
        listener.settimeout(10)

        def receive():
            try:
                connection, _ = listener.accept()
                with connection:
                    connection.settimeout(10)
                    while True:
                        data = connection.recv(65536)
                        if not data:
                            break
                        result['received'] += len(data)
            except OSError as error:
                result['receiver_error'] = str(error)

        thread = threading.Thread(target=receive, daemon=True)
        thread.start()
        try:
            with socket.create_connection(listener.getsockname(), timeout=10) as connection:
                if chunk:
                    for offset in range(0, size, chunk):
                        connection.sendall(payload[offset:offset + chunk])
                else:
                    connection.sendall(payload)
        except OSError as error:
            result.update(errno=error.errno, error=str(error))
        thread.join(12)
        result['passed'] = not thread.is_alive() and result['received'] == size and 'error' not in result
    return result


if __name__ == '__main__':
    print(json.dumps([check(65536), check(3777458), check(3777458, 65536)], indent=2))
