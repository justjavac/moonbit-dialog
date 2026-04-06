///|
/// Show a native dialog and return an encoded success or failure code.
#borrow(title, message, accept_label, reject_label, cancel_label)
extern "c" fn show_dialog_ffi(
  level : Int,
  buttons : Int,
  title : Bytes,
  message : Bytes,
  accept_label : Bytes,
  reject_label : Bytes,
  cancel_label : Bytes,
) -> Int = "moonbit_dialog_show_dialog"

///|
const BACKEND_UNAVAILABLE_STATUS : Int = -1

///|
const UNSUPPORTED_PLATFORM_STATUS : Int = -2

///|
const NATIVE_FAILURE_BASE : Int = 1000000

///|
const NATIVE_FAILURE_STRIDE : Int = 100000

///|
const SUCCESS_RESPONSE_STRIDE : Int = 10

///|
const DIALOG_BUTTONS_OK_CODE : Int = 0

///|
const DIALOG_BUTTONS_OK_CANCEL_CODE : Int = 1

///|
const DIALOG_BUTTONS_YES_NO_CODE : Int = 2

///|
const DIALOG_BUTTONS_YES_NO_CANCEL_CODE : Int = 3

///|
/// Encode a MoonBit string as UTF-8 bytes for native APIs.
fn encode_utf8_bytes(text : String) -> Bytes {
  @utf8.encode(text[:])
}

///|
/// Convert a public level enum into the native FFI code.
fn dialog_level_to_code(level : DialogLevel) -> Int {
  match level {
    Info => 0
    Warning => 1
    Error => 2
    Question => 3
  }
}

///|
/// Convert a public button combination into the native FFI code.
fn dialog_buttons_to_code(buttons : DialogButtons) -> Int {
  match buttons {
    Ok => DIALOG_BUTTONS_OK_CODE
    OkCancel => DIALOG_BUTTONS_OK_CANCEL_CODE
    YesNo => DIALOG_BUTTONS_YES_NO_CODE
    YesNoCancel => DIALOG_BUTTONS_YES_NO_CANCEL_CODE
  }
}

///|
/// Convert a native backend code into the corresponding enum.
fn backend_from_code(code : Int) -> DialogBackend? {
  match code {
    0 => Some(WindowsWin32)
    1 => Some(MacOSCoreFoundation)
    2 => Some(LinuxZenity)
    3 => Some(LinuxKDialog)
    4 => Some(LinuxXMessage)
    _ => None
  }
}

///|
/// Decode the backend part of a successful native result.
fn backend_from_success(raw : Int) -> DialogBackend? {
  backend_from_code(raw % SUCCESS_RESPONSE_STRIDE)
}

///|
/// Decode the response part of a successful native result.
fn response_from_success(raw : Int) -> DialogResponse? {
  match raw / SUCCESS_RESPONSE_STRIDE {
    0 => Some(Ok)
    1 => Some(Cancel)
    2 => Some(Yes)
    3 => Some(No)
    _ => None
  }
}

///|
/// Decode backend failures from the C stub result.
fn decode_failure_status(raw : Int) -> (DialogBackend, Int)? {
  if raw > -NATIVE_FAILURE_BASE {
    None
  } else {
    let encoded = -raw - NATIVE_FAILURE_BASE
    match backend_from_code(encoded / NATIVE_FAILURE_STRIDE) {
      Some(backend) => Some((backend, encoded % NATIVE_FAILURE_STRIDE))
      None => None
    }
  }
}

///|
/// Decode a raw native dialog status into a backend-and-response result.
fn decode_dialog_outcome(
  platform : Platform,
  raw : Int,
) -> Result[DialogOutcome, DialogError] {
  if raw == BACKEND_UNAVAILABLE_STATUS {
    Err(BackendUnavailable(platform))
  } else if raw == UNSUPPORTED_PLATFORM_STATUS {
    Err(UnsupportedPlatform(platform))
  } else if raw >= 0 {
    match (backend_from_success(raw), response_from_success(raw)) {
      (Some(backend), Some(response)) => Ok({ backend, response })
      _ => Err(UnsupportedPlatform(platform))
    }
  } else {
    match decode_failure_status(raw) {
      Some((backend, detail)) => Err(BackendFailed(backend, detail))
      None => Err(UnsupportedPlatform(platform))
    }
  }
}

///|
/// Decode a raw native dialog status into a backend-only public result.
fn decode_backend_result(
  platform : Platform,
  raw : Int,
) -> Result[DialogBackend, DialogError] {
  match decode_dialog_outcome(platform, raw) {
    Ok(outcome) => Ok(outcome.backend)
    Err(error) => Err(error)
  }
}

///|
/// Show a dialog request and decode the backend plus selected response.
fn show_dialog_request(
  title : String,
  message : String,
  level : DialogLevel,
  buttons : DialogButtons,
  labels : DialogLabels,
) -> Result[DialogOutcome, DialogError] {
  let platform = current_platform_internal()
  if platform == Unknown {
    return Err(UnsupportedPlatform(Unknown))
  }
  let raw_result = show_dialog_ffi(
    dialog_level_to_code(level),
    dialog_buttons_to_code(buttons),
    encode_utf8_bytes(title),
    encode_utf8_bytes(message),
    encode_utf8_bytes(labels.accept),
    encode_utf8_bytes(labels.reject),
    encode_utf8_bytes(labels.cancel),
  )
  decode_dialog_outcome(platform, raw_result)
}

///|
/// Show a dialog request and decode only the backend that displayed it.
fn show_dialog_backend_request(
  title : String,
  message : String,
  level : DialogLevel,
  buttons : DialogButtons,
  labels : DialogLabels,
) -> Result[DialogBackend, DialogError] {
  let platform = current_platform_internal()
  if platform == Unknown {
    return Err(UnsupportedPlatform(Unknown))
  }
  let raw_result = show_dialog_ffi(
    dialog_level_to_code(level),
    dialog_buttons_to_code(buttons),
    encode_utf8_bytes(title),
    encode_utf8_bytes(message),
    encode_utf8_bytes(labels.accept),
    encode_utf8_bytes(labels.reject),
    encode_utf8_bytes(labels.cancel),
  )
  decode_backend_result(platform, raw_result)
}

///|
/// Resolve the native platform at compile time when the toolchain exposes it.
#cfg(platform="windows")
fn current_platform_internal() -> Platform {
  Windows
}

///|
#cfg(any(platform="macos", platform="darwin", platform="osx"))
fn current_platform_internal() -> Platform {
  MacOS
}

///|
#cfg(platform="linux")
fn current_platform_internal() -> Platform {
  Linux
}

///|
/// Fall back to `Unknown` when no recognized platform tag is available.
#cfg(not(any(platform="windows", platform="macos", platform="darwin", platform="osx", platform="linux")))
fn current_platform_internal() -> Platform {
  Unknown
}
