# justjavac/dialog

Native-only dialogs and path pickers for MoonBit.

- Windows uses Win32 message and file dialogs.
- macOS uses CoreFoundation alerts and AppleScript path pickers.
- Linux tries `zenity`, then `kdialog`, then `xmessage` for message dialogs.

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

```mbt
let relabeled = @dialog.MessageDialog::new(
  "Build finished successfully.",
).with_labels(@dialog.DialogLabels::ok("Open report"))
```

```mbt
match @dialog.open_file(directory="C:/Projects") {
  Ok(outcome) => ignore(outcome)
  Err(error) => ignore(error)
}
```

```mbt
let filters = [
  @dialog.FileFilter::new("Text Files", ["*.txt", "*.md"]),
  @dialog.FileFilter::new("All Files", ["*"]),
]

let save_dialog = @dialog.SaveFileDialog::new(file_name="report")
  .with_filters(filters)
  .with_default_extension("txt")
```
