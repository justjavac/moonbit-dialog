# justjavac/dialog

Native-only message and confirmation dialogs for MoonBit with severity levels.

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

```mbt
let confirm = @dialog.ConfirmDialog::new(
  "Overwrite the generated files?",
)
```

```mbt
let choice = @dialog.ChoiceDialog::new(
  "Save changes before closing?",
  buttons=YesNoCancel,
  level=Question,
)
```
