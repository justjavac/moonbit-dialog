///|
/// Create a generic dialog request with a standard button combination.
///
/// `title` defaults to `"Dialog"`, `buttons` defaults to `Ok`, and `level`
/// defaults to `Info`.
///
/// The returned dialog starts with `DialogLabels::none()` and can be refined
/// with `with_labels` before it is shown.
///
/// This builder is the most general request type in the package and is useful
/// when the button layout itself is part of the caller's configuration.
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
///
/// This method uses the configured button combination and any custom labels,
/// while preserving the backend information for diagnostics or analytics.
///
/// The `response` inside `DialogOutcome` is interpreted relative to the chosen
/// `DialogButtons`, letting one typed result model cover all standard layouts.
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
///
/// This is the most flexible convenience entry point and mirrors the defaults
/// of `ChoiceDialog::new`.
///
/// Prefer this helper for one-off prompts and switch to the builder form when
/// you need to reuse the request or attach custom labels.
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
///
/// This is equivalent to `show_dialog(message, title~, buttons=OkCancel,
/// level~)` and defaults to the `"Confirm"` title with `Question` severity.
///
/// Successful results use `DialogResponse::Ok` for acceptance and
/// `DialogResponse::Cancel` when the user backs out of the dialog.
pub fn show_ok_cancel(
  message : StringView,
  title? : StringView = "Confirm",
  level? : DialogLevel = Question,
) -> Result[DialogOutcome, DialogError] {
  show_dialog(message, title~, buttons=OkCancel, level~)
}

///|
/// Show a yes-no-cancel dialog.
///
/// This is equivalent to `show_dialog(message, title~, buttons=YesNoCancel,
/// level~)` and defaults to the `"Confirm"` title with `Question` severity.
///
/// Use this helper when dismissal needs to remain a first-class branch instead
/// of being folded into a negative answer.
pub fn ask_yes_no_cancel(
  message : StringView,
  title? : StringView = "Confirm",
  level? : DialogLevel = Question,
) -> Result[DialogOutcome, DialogError] {
  show_dialog(message, title~, buttons=YesNoCancel, level~)
}
