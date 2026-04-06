///|
/// Create a message dialog request.
///
/// `title` defaults to `"Message"` so callers can use the short form when only
/// the message body matters. `level` defaults to `Info`.
///
/// The returned value starts with `DialogLabels::none()`, so callers can opt
/// into custom captions later with `with_labels` when the chosen backend
/// supports them.
pub fn MessageDialog::new(
  message : StringView,
  title? : StringView = "Message",
  level? : DialogLevel = Info,
) -> MessageDialog {
  {
    title: title.to_string(),
    message: message.to_string(),
    level,
    labels: DialogLabels::none(),
  }
}

///|
/// Show a message dialog on the current desktop platform.
///
/// Backend selection rules:
/// - Windows uses the Win32 `MessageBoxW` API.
/// - macOS uses a CoreFoundation user notification API.
/// - Linux tries `zenity`, then `kdialog`, then `xmessage` via direct process
///   spawning without shell command construction.
///
/// The function returns the backend that succeeded so callers can log or debug
/// platform-specific behavior when needed.
pub fn MessageDialog::show(
  self : MessageDialog,
) -> Result[DialogBackend, DialogError] {
  show_dialog_backend_request(
    self.title,
    self.message,
    self.level,
    Ok,
    self.labels,
  )
}

///|
/// Return the current native desktop platform.
///
/// This reports the compile-time platform targeted by the native backend and is
/// mainly useful for diagnostics, logging, and platform-aware tests.
pub fn current_platform() -> Platform {
  current_platform_internal()
}

///|
/// Show a simple message dialog without creating `MessageDialog` manually.
///
/// This is the smallest entry point for the library and is intended for the
/// common case where a single title and message are enough.
///
/// It uses the same defaults as `MessageDialog::new` and returns the backend
/// that successfully displayed the dialog.
pub fn show_message(
  message : StringView,
  title? : StringView = "Message",
  level? : DialogLevel = Info,
) -> Result[DialogBackend, DialogError] {
  MessageDialog::new(message, title~, level~).show()
}

///|
/// Show an informational dialog using the `Info` level.
///
/// This is equivalent to `show_message(message, title~, level=Info)` and uses
/// `"Information"` as the default title.
pub fn show_info(
  message : StringView,
  title? : StringView = "Information",
) -> Result[DialogBackend, DialogError] {
  show_message(message, title~, level=Info)
}

///|
/// Show a warning dialog using the `Warning` level.
///
/// This is equivalent to `show_message(message, title~, level=Warning)` and
/// uses `"Warning"` as the default title.
pub fn show_warning(
  message : StringView,
  title? : StringView = "Warning",
) -> Result[DialogBackend, DialogError] {
  show_message(message, title~, level=Warning)
}

///|
/// Show an error dialog using the `Error` level.
///
/// This is equivalent to `show_message(message, title~, level=Error)` and uses
/// `"Error"` as the default title.
pub fn show_error(
  message : StringView,
  title? : StringView = "Error",
) -> Result[DialogBackend, DialogError] {
  show_message(message, title~, level=Error)
}
