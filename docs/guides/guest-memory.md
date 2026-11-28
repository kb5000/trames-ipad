# Guest Memory Configuration

Guest memory in Trames is **demand-backed**: virtual reservations (such as `PROT_NONE` arenas used by modern runtimes like Node.js or LLVM) do not consume RAM until pages are actually given read/write access and touched.

---

## Configuring Memory on iPad

Memory is configured **per environment** rather than globally:

1. Open **+ → Linux Environments → `<environment>` → Configuration**.
2. Keep **Memory** set to **`auto`** (recommended default) to use host-managed accounting without preallocating a fixed pool, or specify an explicit limit in the environment's TOML profile.
3. Tap **Save Configuration** and restart the environment for changes to take effect.

> **Important**: Guest memory limits cap committed guest pages—they do not reserve physical RAM from iPadOS or cap total app RSS (which also includes the interpreter cache, VFS metadata, and native libraries). Raising a limit does not increase the memory ceiling iPadOS enforces for the app.

---

## Configuring Memory via CLI (`trames-run`)

When running environments from the host CLI, pass explicit budgets if needed:

```sh
trames-run --storage ./store --memory-budget 3GiB \
  --process-memory-budget 1GiB --guest /bin/ash
```

- Budgets must be positive multiples of 4 KiB.
- Per-process budget cannot exceed the aggregate session budget.
- `CLONE_VM` threads sharing an address space are accounted once; `fork` accounts each independent address space conservatively.
- `/proc/meminfo` and `sysinfo` report the configured aggregate capacity; memory `getrlimit` reports the per-process committed-page limit.
