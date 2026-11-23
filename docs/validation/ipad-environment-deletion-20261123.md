# iPad environment deletion — 2026-11-23

The environment manager previously offered reset but no deletion operation.
It now offers **More → Delete Environment…** for every environment, including
stopped Android profiles, and the same action in a sidebar context menu. A
destructive confirmation names the environment and explains permanent deletion.
Unsaved configuration edits use the existing save/discard routing first.

Deleting the active environment closes its sessions and waits for preparation
to exit. Store removal reuses the native reset's existing owner-lock boundary;
the catalog, profile, grants, archived import and pending File Provider edits
are then removed only for that catalogued identifier. Caller-supplied paths are
not used. The lock inode is retained. Other environments and shared directory
authorizations remain intact. Deleting the final profile leaves an empty
catalog; stale open/reset requests cannot silently recreate it.

Validation:

- Physical iPad Air 11-inch (M3), iPadOS 27.0.1: **7 passed**, zero failures or
  skips: three deletion tests, two environment-configuration tests and two
  localization tests. Disposable test roots checked unrelated-environment/data
  preservation, archived import/config/grant/edit cleanup, authoritative catalog
  paths, empty-catalog persistence, stale requests, and owner-lock wait behavior.
  Result: `target/environment-delete-check/ipad.xcresult`.
- iPad Pro 13-inch (M5) simulator, iPadOS 27.0: **1 UI test passed**. The real
  menu for a stopped Android fixture supports cancelling and confirming deletion,
  with Alpine and Debian fixtures retained. The fixture is separate from user
  environments. Result: `target/environment-delete-check/ui-simulator-final.xcresult`.
- The physical UI test driver disconnected before executing a test (exit 74);
  its result is not reported as a UI pass. The simulator provided the UI check.
- Both signed iPhoneOS Release builds passed and preserve USB file-sharing keys.
  Dev and Stable **1.5.4 (67)** were installed with their existing bundle IDs and
  respective App Groups, preserving both app containers. Clean installation
  copies exclude injected XCTest components and pass deep/strict signature checks.

Builds use the same pre-existing native iOS archive as USB candidate 66, checked
against its recorded hash and Vulkan inputs. No Runtime code or pin is changed;
the independent unfinished Runtime workspace edits are not included. The local
build specs retain guest payload checks while reusing this archive, as recorded
in `ipad-usb-file-sharing-20261123.md`. No CPU/Linux/network/graphics acceptance
suite was rerun for this frontend change.
