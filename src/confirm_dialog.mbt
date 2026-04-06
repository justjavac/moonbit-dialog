///|
/// Create a yes-or-no confirmation dialog request.
///
/// `title` defaults to `"Confirm"` and `level` defaults to `Question`.
pub fn ConfirmDialog::new(
  message : StringView,
  title? : StringView = "Confirm",
  level? : DialogLevel = Question,
) -> ConfirmDialog {
  { title: title.to_string(), message: message.to_string(), level }
}

///|
/// Show a confirmation dialog and return both the backend and the response.
pub fn ConfirmDialog::show(
  self : ConfirmDialog,
) -> Result[DialogOutcome, DialogError] {
  show_dialog_request(self.title, self.message, self.level, YesNo)
}

///|
/// Ask a yes-or-no question without constructing `ConfirmDialog` manually.
pub fn ask_yes_no(
  message : StringView,
  title? : StringView = "Confirm",
  level? : DialogLevel = Question,
) -> Result[DialogOutcome, DialogError] {
  ConfirmDialog::new(message, title~, level~).show()
}
