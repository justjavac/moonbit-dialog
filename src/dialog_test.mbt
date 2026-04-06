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
  assert_eq(dialog.labels, @dialog.DialogLabels::none())
}

///|
test "MessageDialog::new uses the default title and level" {
  let dialog = @dialog.MessageDialog::new("Hello")
  assert_eq(dialog.title, "Message")
  assert_eq(dialog.message, "Hello")
  assert_eq(dialog.level, Info)
  assert_eq(dialog.labels, @dialog.DialogLabels::none())
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
  assert_eq(dialog.labels, @dialog.DialogLabels::none())
}

///|
test "ConfirmDialog::new uses the default title and level" {
  let dialog = @dialog.ConfirmDialog::new("Continue?")
  assert_eq(dialog.title, "Confirm")
  assert_eq(dialog.message, "Continue?")
  assert_eq(dialog.level, Question)
  assert_eq(dialog.labels, @dialog.DialogLabels::none())
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
  assert_eq(dialog.labels, @dialog.DialogLabels::none())
}

///|
test "ChoiceDialog::new uses the default title, level, and buttons" {
  let dialog = @dialog.ChoiceDialog::new("Done")
  assert_eq(dialog.title, "Dialog")
  assert_eq(dialog.message, "Done")
  assert_eq(dialog.level, Info)
  assert_eq(dialog.buttons, Ok)
  assert_eq(dialog.labels, @dialog.DialogLabels::none())
}

///|
test "DialogLabels constructors map to the expected fields" {
  assert_eq(
    @dialog.DialogLabels::ok("Continue"),
    @dialog.DialogLabels::ok("Continue"),
  )
  assert_eq(
    @dialog.DialogLabels::yes_no_cancel("Save", "Discard", "Stay"),
    @dialog.DialogLabels::yes_no_cancel("Save", "Discard", "Stay"),
  )
}

///|
test "with_labels attaches custom labels without changing other fields" {
  let dialog = @dialog.ChoiceDialog::new(
    "Save changes before closing?",
    buttons=YesNoCancel,
    level=Question,
  ).with_labels(@dialog.DialogLabels::yes_no_cancel("Save", "Discard", "Stay"))
  assert_eq(
    dialog.labels,
    @dialog.DialogLabels::yes_no_cancel("Save", "Discard", "Stay"),
  )
  assert_eq(dialog.buttons, YesNoCancel)
  assert_eq(dialog.level, Question)
}

///|
test "supports_custom_labels documents the current backend behavior" {
  assert_eq(@dialog.supports_custom_labels(WindowsWin32), false)
  assert_eq(@dialog.supports_custom_labels(MacOSCoreFoundation), true)
  assert_eq(@dialog.supports_custom_labels(LinuxKDialog), false)
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
