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
test "ConfirmDialog::new stores title, message, and level" {
  let dialog = @dialog.ConfirmDialog::new(
    "Delete the build folder?",
    title="moonbit-dialog",
    level=Warning,
  )
  assert_eq(dialog.title, "moonbit-dialog")
  assert_eq(dialog.message, "Delete the build folder?")
  assert_eq(dialog.level, Warning)
}

///|
test "ConfirmDialog::new uses the default title and level" {
  let dialog = @dialog.ConfirmDialog::new("Continue?")
  assert_eq(dialog.title, "Confirm")
  assert_eq(dialog.message, "Continue?")
  assert_eq(dialog.level, Question)
}

///|
test "ChoiceDialog::new stores buttons and level" {
  let dialog = @dialog.ChoiceDialog::new(
    "Save changes before closing?",
    title="moonbit-dialog",
    buttons=YesNoCancel,
    level=Question,
  )
  assert_eq(dialog.title, "moonbit-dialog")
  assert_eq(dialog.message, "Save changes before closing?")
  assert_eq(dialog.level, Question)
  assert_eq(dialog.buttons, YesNoCancel)
}

///|
test "ChoiceDialog::new uses the default title, level, and buttons" {
  let dialog = @dialog.ChoiceDialog::new("Done")
  assert_eq(dialog.title, "Dialog")
  assert_eq(dialog.message, "Done")
  assert_eq(dialog.level, Info)
  assert_eq(dialog.buttons, Ok)
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
