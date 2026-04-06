///|
/// Show a native message dialog and return a backend or failure code.
#borrow(title, message)
extern "c" fn show_message_ffi(title : Bytes, message : Bytes) -> Int = "moonbit_dialog_show_message"

///|
const BACKEND_UNAVAILABLE_STATUS : Int = -1

///|
const UNSUPPORTED_PLATFORM_STATUS : Int = -2

///|
const NATIVE_FAILURE_BASE : Int = 1000000

///|
const NATIVE_FAILURE_STRIDE : Int = 100000

///|
/// Encode a MoonBit string as UTF-8 bytes for native APIs.
fn encode_utf8_bytes(text : String) -> Bytes {
  @utf8.encode(text[:])
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
/// Decode a raw native dialog status into the public result type.
fn decode_native_result(
  platform : Platform,
  raw : Int,
) -> Result[DialogBackend, DialogError] {
  if raw == BACKEND_UNAVAILABLE_STATUS {
    Err(BackendUnavailable(platform))
  } else if raw == UNSUPPORTED_PLATFORM_STATUS {
    Err(UnsupportedPlatform(platform))
  } else {
    match backend_from_code(raw) {
      Some(backend) => Ok(backend)
      None =>
        match decode_failure_status(raw) {
          Some((backend, detail)) => Err(BackendFailed(backend, detail))
          None => Err(UnsupportedPlatform(platform))
        }
    }
  }
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
