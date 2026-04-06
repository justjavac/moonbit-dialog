///|
test "MessageDialog::new stores title and message" {
  let dialog = @dialog.MessageDialog::new(
    "Build finished.",
    title="moonbit-dialog",
    level=Warning,
  )
  assert_eq(dialog.title, "moonbit-dialog")
  assert_eq(dialog.message, "Build finished.")
  assert_eq(dialog.level, Warning)
}

///|
test "MessageDialog::new uses the default title and level" {
  let dialog = @dialog.MessageDialog::new("Hello")
  assert_eq(dialog.title, "Message")
  assert_eq(dialog.message, "Hello")
  assert_eq(dialog.level, Info)
}

///|
test "current_platform returns a known enum" {
  let platform = @dialog.current_platform()
  assert_true(
    platform is Windows ||
    platform is MacOS ||
    platform is Linux ||
    platform is Unknown,
  )
}
