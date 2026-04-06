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
  let platform = current_platform_internal()
  if platform == Unknown {
    return Err(UnsupportedPlatform(Unknown))
  }
  let raw_result = show_dialog_ffi(
    dialog_level_to_code(self.level),
    DIALOG_BUTTONS_YES_NO_CODE,
    encode_utf8_bytes(self.title),
    encode_utf8_bytes(self.message),
    encode_utf8_bytes(""),
    encode_utf8_bytes(""),
    encode_utf8_bytes(""),
  )
  decode_dialog_outcome(platform, raw_result)
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
