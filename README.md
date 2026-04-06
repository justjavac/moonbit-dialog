# moonbit-dialog

[![coverage](https://img.shields.io/codecov/c/github/justjavac/moonbit-dialog/main?label=coverage)](https://codecov.io/gh/justjavac/moonbit-dialog)
[![linux](https://img.shields.io/codecov/c/github/justjavac/moonbit-dialog/main?flag=linux&label=linux)](https://codecov.io/gh/justjavac/moonbit-dialog)
[![macos](https://img.shields.io/codecov/c/github/justjavac/moonbit-dialog/main?flag=macos&label=macos)](https://codecov.io/gh/justjavac/moonbit-dialog)
[![windows](https://img.shields.io/codecov/c/github/justjavac/moonbit-dialog/main?flag=windows&label=windows)](https://codecov.io/gh/justjavac/moonbit-dialog)

`moonbit-dialog` is a native-only MoonBit library that shows dialog on Windows, macOS, and Linux.

## Features

- Native-only MoonBit package
- Windows, macOS, and Linux support
- Info, warning, error, and question dialog levels
- Yes/no confirmation dialogs with explicit responses
- Generic standard-button dialogs (`Ok`, `OkCancel`, `YesNo`, `YesNoCancel`)
- Best-effort custom button labels with backend capability checks
- Small public API focused on readability
- Detailed public API documentation in source
- Coverage-aware tests plus Codecov badges
- Example program under [`examples/`](./examples)

## Installation

Add the package to your MoonBit module and import `justjavac/dialog`.

`moonbit-dialog` is designed for the native backend only, so keep your module on `native` when you run or test code that shows dialogs.

## Quick Start

```moonbit
let result = @dialog.show_message(
  "Hello from MoonBit!",
  title="moonbit-dialog",
  level=Warning,
)

match result {
  Ok(backend) => println("Dialog shown with \{backend}")
  Err(error) => println("Dialog failed: \{error}")
}
```

If you prefer an explicit value object, use `MessageDialog`:

```moonbit
let dialog = @dialog.MessageDialog::new(
  "Build finished successfully.",
  title="moonbit-dialog",
  level=Info,
)

match dialog.show() {
  Ok(backend) => println("Dialog shown with \{backend}")
  Err(error) => println("Dialog failed: \{error}")
}
```

There are also convenience helpers for common severity levels:

```moonbit
ignore(@dialog.show_warning("Configuration file is missing."))
ignore(@dialog.show_error("Build failed."))
```

Use `ConfirmDialog` or `ask_yes_no` when you need a typed answer:

```moonbit
match @dialog.ask_yes_no("Overwrite the existing build output?") {
  Ok(outcome) =>
    match outcome.response {
      Yes => println("Continuing with \{outcome.backend}")
      No => println("Cancelled by user")
      _ => ()
    }
  Err(error) => println("Dialog failed: \{error}")
}
```

For more control, use `ChoiceDialog` or `show_dialog` with `DialogButtons`:

```moonbit
match @dialog.show_dialog(
  "Save changes before closing?",
  buttons=YesNoCancel,
  level=Question,
) {
  Ok(outcome) => println("Selected \{outcome.response} with \{outcome.backend}")
  Err(error) => println("Dialog failed: \{error}")
}
```

If you want custom captions, attach `DialogLabels` and optionally inspect
`supports_custom_labels` on the backend that actually displayed the dialog:

```moonbit
let dialog = @dialog.ChoiceDialog::new(
  "Save changes before closing?",
  buttons=YesNoCancel,
  level=Question,
).with_labels(
  @dialog.DialogLabels::yes_no_cancel("Save", "Discard", "Stay"),
)
```

Backends that cannot rename native buttons keep their default captions, so use
`supports_custom_labels(outcome.backend)` if that distinction matters.

## Backend Strategy

This first version chooses the smallest dependable implementation on each platform:

- Windows: Win32 `MessageBoxW`
- macOS: CoreFoundation user notification API
- Linux: `zenity`, then `kdialog`, then `xmessage` via direct process spawning

Linux desktop stacks vary a lot, so the library tries several common tools in a predictable order. If none are installed, the API returns `Err(BackendUnavailable(Linux))`.

## Public API

The package currently exposes:

- `Platform`
- `DialogBackend`
- `DialogLevel`
- `DialogButtons`
- `DialogResponse`
- `DialogOutcome`
- `DialogLabels`
- `DialogError`
- `MessageDialog`
- `ConfirmDialog`
- `ChoiceDialog`
- `current_platform()`
- `supports_custom_labels(backend)`
- `show_message(message, title?, level?)`
- `show_info(message, title?)`
- `show_warning(message, title?)`
- `show_error(message, title?)`
- `ask_yes_no(message, title?, level?)`
- `show_dialog(message, title?, buttons?, level?)`
- `show_ok_cancel(message, title?, level?)`
- `ask_yes_no_cancel(message, title?, level?)`

The API returns `Result[DialogBackend, DialogError]` so callers can handle missing backends or backend failures without guessing.

## Running the Example

The example lives in a separate MoonBit submodule so the main package can keep `src/` as its source root.

```bash
moon -C examples run message
```

## Testing

The test suite is split into:

- black-box tests for the public API shape
- white-box tests for native result decoding and backend selection

Run the package tests with:

```bash
moon test
```

Generate a local coverage report with:

```bash
moon coverage analyze -p justjavac/dialog -- -f cobertura -o coverage.xml
```

## Scope of This Version

This initial release only implements a message dialog with an OK-style acknowledgement flow.

Planned future work can build on the same structure to add:

- custom button sets
- richer Linux backend detection
- native platform bindings where they improve behavior

## License

MIT
