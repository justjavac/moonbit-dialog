///|
/// Create an empty label override set that keeps backend defaults.
///
/// This is useful when building dialogs incrementally and wanting to reset any
/// previously configured custom captions.
pub fn DialogLabels::none() -> DialogLabels {
  { accept: "", reject: "", cancel: "" }
}

///|
/// Create a label override set for one-button dialogs.
///
/// The value is stored in the `accept` slot, which is used for `Ok` in message
/// dialogs and the affirmative action in other single-button flows.
pub fn DialogLabels::ok(label : StringView) -> DialogLabels {
  { accept: label.to_string(), reject: "", cancel: "" }
}

///|
/// Create a label override set for OK-cancel dialogs.
///
/// `ok_label` is mapped to the accept button and `cancel_label` is mapped to
/// the cancel button, while the reject slot remains unused.
pub fn DialogLabels::ok_cancel(
  ok_label : StringView,
  cancel_label : StringView,
) -> DialogLabels {
  { accept: ok_label.to_string(), reject: "", cancel: cancel_label.to_string() }
}

///|
/// Create a label override set for yes-no dialogs.
///
/// `yes_label` is stored as the accept caption and `no_label` is stored as the
/// reject caption.
pub fn DialogLabels::yes_no(
  yes_label : StringView,
  no_label : StringView,
) -> DialogLabels {
  { accept: yes_label.to_string(), reject: no_label.to_string(), cancel: "" }
}

///|
/// Create a label override set for yes-no-cancel dialogs.
///
/// This fills all three standard caption slots so a dialog can fully override
/// its affirmative, negative, and cancel labels where the backend allows it.
pub fn DialogLabels::yes_no_cancel(
  yes_label : StringView,
  no_label : StringView,
  cancel_label : StringView,
) -> DialogLabels {
  {
    accept: yes_label.to_string(),
    reject: no_label.to_string(),
    cancel: cancel_label.to_string(),
  }
}

///|
/// Return whether a backend applies custom button labels directly.
///
/// Windows and the current `kdialog` path fall back to native default labels.
/// Callers can use this to decide whether exposing label customization is worth
/// surfacing for a known backend.
pub fn supports_custom_labels(backend : DialogBackend) -> Bool {
  match backend {
    WindowsWin32 => false
    MacOSCoreFoundation => true
    MacOSAppleScript => false
    LinuxZenity => true
    LinuxKDialog => false
    LinuxXMessage => true
  }
}

///|
/// Attach custom labels to a message dialog.
///
/// This returns a new dialog value and leaves the original unchanged.
/// Unsupported backends keep their native default captions.
pub fn MessageDialog::with_labels(
  self : MessageDialog,
  labels : DialogLabels,
) -> MessageDialog {
  { title: self.title, message: self.message, level: self.level, labels }
}

///|
/// Attach custom labels to a confirmation dialog.
///
/// This returns a new dialog value and leaves the original unchanged.
/// Unsupported backends keep their native default captions.
pub fn ConfirmDialog::with_labels(
  self : ConfirmDialog,
  labels : DialogLabels,
) -> ConfirmDialog {
  { title: self.title, message: self.message, level: self.level, labels }
}

///|
/// Attach custom labels to a generic dialog.
///
/// This returns a new dialog value and leaves the original unchanged.
/// Unsupported backends keep their native default captions.
pub fn ChoiceDialog::with_labels(
  self : ChoiceDialog,
  labels : DialogLabels,
) -> ChoiceDialog {
  {
    title: self.title,
    message: self.message,
    level: self.level,
    buttons: self.buttons,
    labels,
  }
}
