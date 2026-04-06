///|
test "path dialog selection keeps the selected path" {
  let result = decode_path_dialog_outcome(
    Windows,
    success_status_for_test(WindowsWin32),
    encode_utf8_bytes("C:/Temp/example.txt"),
  )
  assert_eq(
    result,
    Ok({ backend: WindowsWin32, selection: Selected("C:/Temp/example.txt") }),
  )
}

///|
test "path dialog cancellation keeps the backend" {
  let result = decode_path_dialog_outcome(
    MacOS,
    success_status_for_test(MacOSAppleScript, response=Cancel),
    encode_utf8_bytes(""),
  )
  assert_eq(result, Ok({ backend: MacOSAppleScript, selection: Cancelled }))
}

///|
test "path dialog failure keeps backend and detail" {
  let result = decode_path_dialog_outcome(
    Linux,
    failure_status(LinuxZenity, 42),
    encode_utf8_bytes(""),
  )
  assert_eq(result, Err(BackendFailed(LinuxZenity, 42)))
}

///|
test "file filter encoding escapes separators" {
  let encoded = encode_file_filters([
    FileFilter::{
      name: "Source\tFiles",
      patterns: ["*.mbt", "*.txt\nbackup", "literal\\name"],
    },
  ])
  assert_eq(
    utf8_bytes_to_mbt_string(encoded),
    "Source\\tFiles\t*.mbt\t*.txt\\nbackup\tliteral\\\\name",
  )
}
