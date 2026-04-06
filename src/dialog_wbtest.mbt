///|
fn success_status_for_test(
  backend : DialogBackend,
  response? : DialogResponse = Ok,
) -> Int {
  let backend_code = match backend {
    WindowsWin32 => 0
    MacOSCoreFoundation => 1
    MacOSAppleScript => 2
    LinuxZenity => 3
    LinuxKDialog => 4
    LinuxXMessage => 5
  }
  let response_code = match response {
    Ok => 0
    Cancel => 1
    Yes => 2
    No => 3
  }
  backend_code + response_code * SUCCESS_RESPONSE_STRIDE
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
  success_status_for_test(backend) * NATIVE_FAILURE_STRIDE +
  detail)
}

///|
test "backend codes round-trip through the decoder" {
  assert_eq(
    backend_from_code(success_status_for_test(WindowsWin32)),
    Some(WindowsWin32),
  )
  assert_eq(
    backend_from_code(success_status_for_test(MacOSCoreFoundation)),
    Some(MacOSCoreFoundation),
  )
  assert_eq(
    backend_from_code(success_status_for_test(MacOSAppleScript)),
    Some(MacOSAppleScript),
  )
  assert_eq(
    backend_from_code(success_status_for_test(LinuxZenity)),
    Some(LinuxZenity),
  )
  assert_eq(
    backend_from_code(success_status_for_test(LinuxKDialog)),
    Some(LinuxKDialog),
  )
  assert_eq(
    backend_from_code(success_status_for_test(LinuxXMessage)),
    Some(LinuxXMessage),
  )
}

///|
test "unknown platforms are rejected" {
  let result = decode_backend_result(Unknown, UNSUPPORTED_PLATFORM_STATUS)
  assert_true(result is Err(UnsupportedPlatform(Unknown)))
}

///|
test "backend unavailable status maps to the public error" {
  assert_eq(
    decode_backend_result(Linux, BACKEND_UNAVAILABLE_STATUS),
    Err(BackendUnavailable(Linux)),
  )
}

///|
test "success statuses map to the right backend" {
  assert_eq(
    decode_backend_result(Windows, success_status_for_test(WindowsWin32)),
    Ok(WindowsWin32),
  )
  assert_eq(
    decode_backend_result(MacOS, success_status_for_test(MacOSCoreFoundation)),
    Ok(MacOSCoreFoundation),
  )
  assert_eq(
    decode_backend_result(Linux, success_status_for_test(LinuxXMessage)),
    Ok(LinuxXMessage),
  )
}

///|
test "dialog outcomes keep backend and response" {
  assert_eq(
    decode_dialog_outcome(
      Windows,
      success_status_for_test(WindowsWin32, response=Yes),
    ),
    Ok({ backend: WindowsWin32, response: Yes }),
  )
  assert_eq(
    decode_dialog_outcome(
      Linux,
      success_status_for_test(LinuxZenity, response=No),
    ),
    Ok({ backend: LinuxZenity, response: No }),
  )
}

///|
test "backend-only decoder ignores the response code" {
  assert_eq(
    decode_backend_result(
      Linux,
      success_status_for_test(LinuxKDialog, response=Cancel),
    ),
    Ok(LinuxKDialog),
  )
}

///|
test "failure statuses keep backend and detail" {
  let result = decode_backend_result(
    Windows,
    failure_status(WindowsWin32, 1234),
  )
  assert_eq(result, Err(BackendFailed(WindowsWin32, 1234)))
}

///|
test "failure statuses clamp oversized details" {
  let result = decode_failure_status(
    failure_status(LinuxZenity, NATIVE_FAILURE_STRIDE + 77),
  )
  assert_eq(result, Some((LinuxZenity, NATIVE_FAILURE_STRIDE - 1)))
}
