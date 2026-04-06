# justjavac/dialog

Native-only message dialogs for MoonBit with severity levels.

- Windows uses Win32 `MessageBoxW`.
- macOS uses a CoreFoundation notification API.
- Linux tries `zenity`, then `kdialog`, then `xmessage` via direct process spawning.

```mbt
let dialog = @dialog.MessageDialog::new(
  "Build finished successfully.",
  title="moonbit-dialog",
  level=Info,
)
```
