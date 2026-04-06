///|
/// Build a synthetic successful status code for decoder tests.
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
/// Build a synthetic backend failure status code for decoder tests.
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

///|
test "UTF-8 decoding handles multibyte code points and truncated tails" {
  let text = @utf8.decode([0xC3, 0xA9, 0xE4, 0xB8, 0xAD, 0xF0, 0x9F, 0x99, 0x82])
  assert_eq(utf8_bytes_to_mbt_string(encode_utf8_bytes(text)), text)

  let incomplete_two : Bytes = [0xC3]
  let incomplete_three : Bytes = [0xE4, 0xB8]
  let incomplete_four : Bytes = [0xF0, 0x9F, 0x99]
  assert_eq(utf8_bytes_to_mbt_string(incomplete_two), "")
  assert_eq(utf8_bytes_to_mbt_string(incomplete_three), "")
  assert_eq(utf8_bytes_to_mbt_string(incomplete_four), "")
}

///|
test "dialog level and button enums map to native codes" {
  assert_eq(dialog_level_to_code(Info), 0)
  assert_eq(dialog_level_to_code(Warning), 1)
  assert_eq(dialog_level_to_code(Error), 2)
  assert_eq(dialog_level_to_code(Question), 3)

  assert_eq(dialog_buttons_to_code(Ok), DIALOG_BUTTONS_OK_CODE)
  assert_eq(dialog_buttons_to_code(OkCancel), DIALOG_BUTTONS_OK_CANCEL_CODE)
  assert_eq(dialog_buttons_to_code(YesNo), DIALOG_BUTTONS_YES_NO_CODE)
  assert_eq(
    dialog_buttons_to_code(YesNoCancel),
    DIALOG_BUTTONS_YES_NO_CANCEL_CODE,
  )
}

///|
test "invalid backend and response codes are rejected" {
  assert_eq(backend_from_code(99), None)
  assert_eq(response_from_success(SUCCESS_RESPONSE_STRIDE * 4), None)
}

///|
test "failure status decoding rejects non-failure and unknown backend codes" {
  assert_eq(decode_failure_status(-12345), None)
  assert_eq(
    decode_failure_status(
      -(NATIVE_FAILURE_BASE + 6 * NATIVE_FAILURE_STRIDE + 17),
    ),
    None,
  )
}

///|
test "dialog outcome decoder rejects malformed payloads" {
  assert_eq(decode_dialog_outcome(Linux, 6), Err(UnsupportedPlatform(Linux)))
  assert_eq(
    decode_dialog_outcome(MacOS, SUCCESS_RESPONSE_STRIDE * 4),
    Err(UnsupportedPlatform(MacOS)),
  )
  assert_eq(
    decode_dialog_outcome(
      Windows,
      -(NATIVE_FAILURE_BASE + 6 * NATIVE_FAILURE_STRIDE + 9),
    ),
    Err(UnsupportedPlatform(Windows)),
  )
}
