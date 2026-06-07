///|
test "MessageDialog::new stores title and message" {
  let dialog = @dialog.MessageDialog::new(
    "Build finished.",
    title="moonbit-dialog",
    level=Warning,
  )
  @debug.assert_eq(dialog.title, "moonbit-dialog")
  @debug.assert_eq(dialog.message, "Build finished.")
  @debug.assert_eq(dialog.level, Warning)
  @debug.assert_eq(dialog.labels, @dialog.DialogLabels::none())
}

///|
test "MessageDialog::new uses the default title and level" {
  let dialog = @dialog.MessageDialog::new("Hello")
  @debug.assert_eq(dialog.title, "Message")
  @debug.assert_eq(dialog.message, "Hello")
  @debug.assert_eq(dialog.level, Info)
  @debug.assert_eq(dialog.labels, @dialog.DialogLabels::none())
}

///|
test "ConfirmDialog::new stores title, message, and level" {
  let dialog = @dialog.ConfirmDialog::new(
    "Delete the build folder?",
    title="moonbit-dialog",
    level=Warning,
  )
  @debug.assert_eq(dialog.title, "moonbit-dialog")
  @debug.assert_eq(dialog.message, "Delete the build folder?")
  @debug.assert_eq(dialog.level, Warning)
  @debug.assert_eq(dialog.labels, @dialog.DialogLabels::none())
}

///|
test "ConfirmDialog::new uses the default title and level" {
  let dialog = @dialog.ConfirmDialog::new("Continue?")
  @debug.assert_eq(dialog.title, "Confirm")
  @debug.assert_eq(dialog.message, "Continue?")
  @debug.assert_eq(dialog.level, Question)
  @debug.assert_eq(dialog.labels, @dialog.DialogLabels::none())
}

///|
test "ChoiceDialog::new stores buttons and level" {
  let dialog = @dialog.ChoiceDialog::new(
    "Save changes before closing?",
    title="moonbit-dialog",
    buttons=YesNoCancel,
    level=Question,
  )
  @debug.assert_eq(dialog.title, "moonbit-dialog")
  @debug.assert_eq(dialog.message, "Save changes before closing?")
  @debug.assert_eq(dialog.level, Question)
  @debug.assert_eq(dialog.buttons, YesNoCancel)
  @debug.assert_eq(dialog.labels, @dialog.DialogLabels::none())
}

///|
test "ChoiceDialog::new uses the default title, level, and buttons" {
  let dialog = @dialog.ChoiceDialog::new("Done")
  @debug.assert_eq(dialog.title, "Dialog")
  @debug.assert_eq(dialog.message, "Done")
  @debug.assert_eq(dialog.level, Info)
  @debug.assert_eq(dialog.buttons, Ok)
  @debug.assert_eq(dialog.labels, @dialog.DialogLabels::none())
}

///|
test "DialogLabels constructors populate the expected slots" {
  let none = @dialog.DialogLabels::none()
  @debug.assert_eq(none.accept, "")
  @debug.assert_eq(none.reject, "")
  @debug.assert_eq(none.cancel, "")

  let ok = @dialog.DialogLabels::ok("Continue")
  @debug.assert_eq(ok.accept, "Continue")
  @debug.assert_eq(ok.reject, "")
  @debug.assert_eq(ok.cancel, "")

  let yes_no_cancel = @dialog.DialogLabels::yes_no_cancel(
    "Save", "Discard", "Stay",
  )
  @debug.assert_eq(yes_no_cancel.accept, "Save")
  @debug.assert_eq(yes_no_cancel.reject, "Discard")
  @debug.assert_eq(yes_no_cancel.cancel, "Stay")
}

///|
test "DialogLabels convenience constructors store the expected button captions" {
  let ok_cancel = @dialog.DialogLabels::ok_cancel("Proceed", "Back")
  @debug.assert_eq(ok_cancel.accept, "Proceed")
  @debug.assert_eq(ok_cancel.reject, "")
  @debug.assert_eq(ok_cancel.cancel, "Back")

  let yes_no = @dialog.DialogLabels::yes_no("Ship", "Review")
  @debug.assert_eq(yes_no.accept, "Ship")
  @debug.assert_eq(yes_no.reject, "Review")
  @debug.assert_eq(yes_no.cancel, "")
}

///|
test "MessageDialog::with_labels attaches custom labels without changing other fields" {
  let dialog = @dialog.MessageDialog::new(
    "Build finished.",
    title="moonbit-dialog",
    level=Info,
  ).with_labels(@dialog.DialogLabels::ok("Open report"))
  @debug.assert_eq(dialog.title, "moonbit-dialog")
  @debug.assert_eq(dialog.message, "Build finished.")
  @debug.assert_eq(dialog.level, Info)
  @debug.assert_eq(dialog.labels, @dialog.DialogLabels::ok("Open report"))
}

///|
test "ConfirmDialog::with_labels attaches custom labels without changing other fields" {
  let dialog = @dialog.ConfirmDialog::new(
    "Delete the build folder?",
    title="moonbit-dialog",
    level=Warning,
  ).with_labels(@dialog.DialogLabels::yes_no("Delete", "Keep"))
  @debug.assert_eq(dialog.title, "moonbit-dialog")
  @debug.assert_eq(dialog.message, "Delete the build folder?")
  @debug.assert_eq(dialog.level, Warning)
  @debug.assert_eq(
    dialog.labels,
    @dialog.DialogLabels::yes_no("Delete", "Keep"),
  )
}

///|
test "with_labels attaches custom labels without changing other fields" {
  let dialog = @dialog.ChoiceDialog::new(
    "Save changes before closing?",
    buttons=YesNoCancel,
    level=Question,
  ).with_labels(@dialog.DialogLabels::yes_no_cancel("Save", "Discard", "Stay"))
  @debug.assert_eq(
    dialog.labels,
    @dialog.DialogLabels::yes_no_cancel("Save", "Discard", "Stay"),
  )
  @debug.assert_eq(dialog.buttons, YesNoCancel)
  @debug.assert_eq(dialog.level, Question)
}

///|
test "supports_custom_labels documents the current backend behavior" {
  @debug.assert_eq(@dialog.supports_custom_labels(WindowsWin32), false)
  @debug.assert_eq(@dialog.supports_custom_labels(MacOSCoreFoundation), true)
  @debug.assert_eq(@dialog.supports_custom_labels(MacOSAppleScript), false)
  @debug.assert_eq(@dialog.supports_custom_labels(LinuxZenity), true)
  @debug.assert_eq(@dialog.supports_custom_labels(LinuxKDialog), false)
  @debug.assert_eq(@dialog.supports_custom_labels(LinuxXMessage), true)
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
