# SPDX-License-Identifier: GPL-3.0-or-later
# Run only in an isolated rootfs: stock runc and /bundle/config.json/rootfs required.
# The OCI payload must exit successfully; stdout/stage logs are retained on failure.
set -eu
mount -t tmpfs -o mode=755 tmpfs /run
mkdir -p /sys/fs/cgroup /dev/shm /run/runc-logs
mount -t cgroup2 none /sys/fs/cgroup || :
mkdir -p /padding
for n in $(seq 1 32); do
    mkdir -p /padding/$n
    mount -t tmpfs -o size=1m tmpfs /padding/$n
done
cd /bundle
failed=0
total=0
for round in 1 2 3; do
    for job in 1 2 3 4 5 6 7 8; do
        id=probe-${round}-${job}
        (
            set +e
            runc --debug --log /run/runc-logs/$id.log --root /run/runc-repro run "$id" > /run/runc-logs/$id.out 2>&1
            result=$?
            printf '%s\n' "$result" > /run/runc-logs/$id.rc
        ) &
    done
    wait
    for job in 1 2 3 4 5 6 7 8; do
        id=probe-${round}-${job}
        total=$((total+1))
        rc=$(cat /run/runc-logs/$id.rc)
        if test "$rc" != 0; then
            failed=$((failed+1))
            printf '\nRUNC_FAILURE id=%s rc=%s\n' "$id" "$rc"
            cat /run/runc-logs/$id.out /run/runc-logs/$id.log
        fi
    done
    printf 'RUNC_ROUND round=%s total=%s failed=%s\n' "$round" "$total" "$failed"
done
printf 'RUNC_RESULT total=%s failed=%s\n' "$total" "$failed"
if test "$failed" != 0; then dmesg | tail -160; exit 1; fi
