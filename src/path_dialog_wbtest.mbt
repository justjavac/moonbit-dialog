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

///|
fn encode_path_list_for_test(paths : Array[String]) -> Bytes {
  let encoded : Array[String] = []
  for path in paths {
    encoded.push(escape_serialized_component(path))
  }
  encode_utf8_bytes(encoded.join("\n"))
}

///|
test "path list encoding keeps separators round-trippable" {
  let encoded = encode_path_list_for_test([
    "C:/Temp/a.txt", "D:\\Projects\\notes.md", "line\nbreak.txt",
  ])
  assert_eq(decode_path_list(encoded), [
    "C:/Temp/a.txt", "D:\\Projects\\notes.md", "line\nbreak.txt",
  ])
}

///|
test "multi path dialog selection keeps the selected paths" {
  let result = decode_multi_path_dialog_outcome(
    Windows,
    success_status_for_test(WindowsWin32),
    encode_path_list_for_test(["C:/Temp/a.txt", "C:/Temp/b.txt"]),
  )
  assert_eq(
    result,
    Ok({
      backend: WindowsWin32,
      selection: Selected(["C:/Temp/a.txt", "C:/Temp/b.txt"]),
    }),
  )
}

///|
test "multi path dialog cancellation keeps the backend" {
  let result = decode_multi_path_dialog_outcome(
    Linux,
    success_status_for_test(LinuxZenity, response=Cancel),
    encode_path_list_for_test([]),
  )
  assert_eq(result, Ok({ backend: LinuxZenity, selection: Cancelled }))
}
