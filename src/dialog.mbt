///|
/// Create a message dialog request.
///
/// `title` defaults to `"Message"` so callers can use the short form when only
/// the message body matters.
pub fn MessageDialog::new(
  message : StringView,
  title? : StringView = "Message",
) -> MessageDialog {
  { title: title.to_string(), message: message.to_string() }
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
  let platform = current_platform_internal()
  if platform == Unknown {
    return Err(UnsupportedPlatform(Unknown))
  }
  let raw_result = show_message_ffi(
    encode_utf8_bytes(self.title),
    encode_utf8_bytes(self.message),
  )
  decode_native_result(platform, raw_result)
}

///|
/// Return the current native desktop platform.
///
/// This is mainly useful for diagnostics, logging, and platform-aware tests.
pub fn current_platform() -> Platform {
  current_platform_internal()
}

///|
/// Show a simple message dialog without creating `MessageDialog` manually.
///
/// This is the smallest entry point for the library and is intended for the
/// common case where a single title and message are enough.
pub fn show_message(
  message : StringView,
  title? : StringView = "Message",
) -> Result[DialogBackend, DialogError] {
  MessageDialog::new(message, title~).show()
}
