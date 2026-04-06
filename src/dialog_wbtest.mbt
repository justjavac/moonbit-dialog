///|
fn backend_success_status(backend : DialogBackend) -> Int {
  match backend {
    WindowsWin32 => 0
    MacOSCoreFoundation => 1
    LinuxZenity => 2
    LinuxKDialog => 3
    LinuxXMessage => 4
  }
}

///|
fn failure_status(backend : DialogBackend, detail : Int) -> Int {
  let detail = if detail < 0 {
    -detail
  } else if detail >= NATIVE_FAILURE_STRIDE {
    NATIVE_FAILURE_STRIDE - 1
  } else {
    detail
  }
  -(NATIVE_FAILURE_BASE +
  backend_success_status(backend) * NATIVE_FAILURE_STRIDE +
  detail)
}

///|
test "backend codes round-trip through the decoder" {
  assert_eq(
    backend_from_code(backend_success_status(WindowsWin32)),
    Some(WindowsWin32),
  )
  assert_eq(
    backend_from_code(backend_success_status(MacOSCoreFoundation)),
    Some(MacOSCoreFoundation),
  )
  assert_eq(
    backend_from_code(backend_success_status(LinuxZenity)),
    Some(LinuxZenity),
  )
  assert_eq(
    backend_from_code(backend_success_status(LinuxKDialog)),
    Some(LinuxKDialog),
  )
  assert_eq(
    backend_from_code(backend_success_status(LinuxXMessage)),
    Some(LinuxXMessage),
  )
}

///|
test "unknown platforms are rejected" {
  let result = decode_native_result(Unknown, UNSUPPORTED_PLATFORM_STATUS)
  assert_true(result is Err(UnsupportedPlatform(Unknown)))
}

///|
test "backend unavailable status maps to the public error" {
  assert_eq(
    decode_native_result(Linux, BACKEND_UNAVAILABLE_STATUS),
    Err(BackendUnavailable(Linux)),
  )
}

///|
test "success statuses map to the right backend" {
  assert_eq(
    decode_native_result(Windows, backend_success_status(WindowsWin32)),
    Ok(WindowsWin32),
  )
  assert_eq(
    decode_native_result(MacOS, backend_success_status(MacOSCoreFoundation)),
    Ok(MacOSCoreFoundation),
  )
  assert_eq(
    decode_native_result(Linux, backend_success_status(LinuxXMessage)),
    Ok(LinuxXMessage),
  )
}

///|
test "failure statuses keep backend and detail" {
  let result = decode_native_result(Windows, failure_status(WindowsWin32, 1234))
  assert_eq(result, Err(BackendFailed(WindowsWin32, 1234)))
}

///|
test "failure statuses clamp oversized details" {
  let result = decode_failure_status(
    failure_status(LinuxZenity, NATIVE_FAILURE_STRIDE + 77),
  )
  assert_eq(result, Some((LinuxZenity, NATIVE_FAILURE_STRIDE - 1)))
}
