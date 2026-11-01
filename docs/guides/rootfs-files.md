# Browsing Linux environments in Files

Enable the Trames location in **Files → Browse → … → Edit**, then open
**Trames → environment name**. Alpine retains its original identity and store;
Debian and custom environments appear separately. See
[environment creation and boot modes](ipad-environments.md).

The extension reads atomic manifests and inode objects without opening a second
writable Linux/VFS instance or running recovery. Persisted files remain browsable
while Trames is paused or stopped. Regular files can be previewed, copied or
exported. Linux symlinks and special devices expose metadata; the provider does
not follow them into the host filesystem. Live `/proc`, `/sys` and `/dev` endpoints
are not exported as persisted files.

Editing an existing regular file saves a staged copy. When Trames returns to the
foreground with that environment selected, it commits the edit through the
running kernel using the expected inode and content digest. Conflicting or failed
edits retain their payload under **待写回的编辑** for recovery/export. Edits for a
stopped environment wait until it is opened again. They are never committed into
the currently selected different environment.

Imports, new files and replacements remain available through the app's **Files**
panel or Linux commands. Replacements publish only after a complete transfer;
cancellation preserves the old file. Export does not overwrite an existing host
file. Data copies and hashing stream in bounded chunks; ordinary guest I/O does
not pass through File Provider or a background synchronizer.

`roots.json` records stable environment IDs, display names and relative store
paths. Legacy catalogs default to Alpine/shell without moving user data. The app
and extension require matching valid App Group entitlements. If the signing setup
does not provide a shared container, the app retains its private store and manual
import/export; the Files panel explains that availability. Signing an unsigned
IPA alone does not create the required entitlement/profile.
