///|
/// Create a yes-or-no confirmation dialog request.
///
/// `title` defaults to `"Confirm"` and `level` defaults to `Question`.
///
/// The dialog starts with `DialogLabels::none()`, which keeps native button
/// captions until callers explicitly override them with `with_labels`.
///
/// Like the other dialog builders, this only stores request data and does not
/// interact with the operating system until `show` is invoked.
pub fn ConfirmDialog::new(
  message : StringView,
  title? : StringView = "Confirm",
  level? : DialogLevel = Question,
) -> ConfirmDialog {
  {
    title: title.to_string(),
    message: message.to_string(),
    level,
    labels: DialogLabels::none(),
  }
}

///|
/// Show a confirmation dialog and return both the backend and the response.
///
/// The result distinguishes which backend handled the request and whether the
/// user picked the affirmative or negative action.
///
/// Successful outcomes normally use `Yes` or `No`, while transport and native
/// failures are surfaced separately through `Err(DialogError)`.
pub fn ConfirmDialog::show(
  self : ConfirmDialog,
) -> Result[DialogOutcome, DialogError] {
  show_dialog_request(self.title, self.message, self.level, YesNo, self.labels)
}

///|
/// Ask a yes-or-no question without constructing `ConfirmDialog` manually.
///
/// This convenience function uses the same defaults as `ConfirmDialog::new`
/// and returns the selected response together with the backend that displayed
/// the dialog.
///
/// It is the shortest way to request a typed confirmation while preserving the
/// same backend and error details as `ConfirmDialog::show`.
pub fn ask_yes_no(
  message : StringView,
  title? : StringView = "Confirm",
  level? : DialogLevel = Question,
) -> Result[DialogOutcome, DialogError] {
  ConfirmDialog::new(message, title~, level~).show()
}
