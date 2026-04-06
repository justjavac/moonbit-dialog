///|
/// Create a generic dialog request with a standard button combination.
pub fn ChoiceDialog::new(
  message : StringView,
  title? : StringView = "Dialog",
  buttons? : DialogButtons = Ok,
  level? : DialogLevel = Info,
) -> ChoiceDialog {
  {
    title: title.to_string(),
    message: message.to_string(),
    level,
    buttons,
    labels: DialogLabels::none(),
  }
}

///|
/// Show a dialog and return the backend plus the selected response.
pub fn ChoiceDialog::show(
  self : ChoiceDialog,
) -> Result[DialogOutcome, DialogError] {
  show_dialog_request(
    self.title,
    self.message,
    self.level,
    self.buttons,
    self.labels,
  )
}

///|
/// Show a dialog without constructing `ChoiceDialog` manually.
pub fn show_dialog(
  message : StringView,
  title? : StringView = "Dialog",
  buttons? : DialogButtons = Ok,
  level? : DialogLevel = Info,
) -> Result[DialogOutcome, DialogError] {
  ChoiceDialog::new(message, title~, buttons~, level~).show()
}

///|
/// Show an OK-or-cancel dialog.
pub fn show_ok_cancel(
  message : StringView,
  title? : StringView = "Confirm",
  level? : DialogLevel = Question,
) -> Result[DialogOutcome, DialogError] {
  show_dialog(message, title~, buttons=OkCancel, level~)
}

///|
/// Show a yes-no-cancel dialog.
pub fn ask_yes_no_cancel(
  message : StringView,
  title? : StringView = "Confirm",
  level? : DialogLevel = Question,
) -> Result[DialogOutcome, DialogError] {
  show_dialog(message, title~, buttons=YesNoCancel, level~)
}
