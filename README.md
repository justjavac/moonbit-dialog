# moonbit-dialog

[![coverage](https://img.shields.io/codecov/c/github/justjavac/moonbit-dialog/main?label=coverage)](https://codecov.io/gh/justjavac/moonbit-dialog)
[![linux](https://img.shields.io/codecov/c/github/justjavac/moonbit-dialog/main?flag=linux&label=linux)](https://codecov.io/gh/justjavac/moonbit-dialog)
[![macos](https://img.shields.io/codecov/c/github/justjavac/moonbit-dialog/main?flag=macos&label=macos)](https://codecov.io/gh/justjavac/moonbit-dialog)
[![windows](https://img.shields.io/codecov/c/github/justjavac/moonbit-dialog/main?flag=windows&label=windows)](https://codecov.io/gh/justjavac/moonbit-dialog)

`moonbit-dialog` is a native-only MoonBit library that shows a simple message dialog on Windows, macOS, and Linux.

This repository intentionally starts with a very small surface area:

- one public dialog type
- one convenience function
- one cross-platform result model
- one message box implementation

That keeps the first version easy to read, test, and extend when we add richer dialog types later.

## Features

- Native-only MoonBit package
- Windows, macOS, and Linux support
- Small public API focused on readability
- Detailed public API documentation in source
- Coverage-aware tests plus Codecov badges
- Example program under [`examples/`](./examples)

## Project Layout

```text
.
|-- src/                  # main MoonBit package
|-- examples/             # example MoonBit submodule
|-- .github/workflows/    # CI and coverage upload
|-- README.md             # detailed GitHub README
`-- README.mbt.md         # concise package README
```

## Installation

Add the package to your MoonBit module and import `justjavac/dialog`.

`moonbit-dialog` is designed for the native backend only, so keep your module on `native` when you run or test code that shows dialogs.

## Quick Start

```moonbit
let result = @dialog.show_message(
  "Hello from MoonBit!",
  title="moonbit-dialog",
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
)

match dialog.show() {
  Ok(backend) => println("Dialog shown with \{backend}")
  Err(error) => println("Dialog failed: \{error}")
}
```

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
- `DialogError`
- `MessageDialog`
- `current_platform()`
- `show_message(message, title?)`

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

## CI and Coverage

GitHub Actions runs the native test suite on:

- Ubuntu
- macOS
- Windows

Each platform uploads a separate Codecov flag so the README can expose total, Linux, macOS, and Windows badges.

Badges only update after CI runs on the repository's `main` branch and Codecov finishes processing the uploaded reports.

## Scope of This Version

This initial release only implements a message dialog with an OK-style acknowledgement flow.

Planned future work can build on the same structure to add:

- confirmation dialogs
- custom button sets
- icons and severity levels
- richer Linux backend detection
- native platform bindings where they improve behavior

## License

MIT
