///|
/// Create an empty label override set that keeps backend defaults.
pub fn DialogLabels::none() -> DialogLabels {
  { accept: "", reject: "", cancel: "" }
}

///|
/// Create a label override set for one-button dialogs.
pub fn DialogLabels::ok(label : StringView) -> DialogLabels {
  { accept: label.to_string(), reject: "", cancel: "" }
}

///|
/// Create a label override set for OK-cancel dialogs.
pub fn DialogLabels::ok_cancel(
  ok_label : StringView,
  cancel_label : StringView,
) -> DialogLabels {
  { accept: ok_label.to_string(), reject: "", cancel: cancel_label.to_string() }
}

///|
/// Create a label override set for yes-no dialogs.
pub fn DialogLabels::yes_no(
  yes_label : StringView,
  no_label : StringView,
) -> DialogLabels {
  { accept: yes_label.to_string(), reject: no_label.to_string(), cancel: "" }
}

///|
/// Create a label override set for yes-no-cancel dialogs.
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
pub fn supports_custom_labels(backend : DialogBackend) -> Bool {
  match backend {
    WindowsWin32 => false
    MacOSCoreFoundation => true
    LinuxZenity => true
    LinuxKDialog => false
    LinuxXMessage => true
  }
}

///|
/// Attach custom labels to a message dialog.
///
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
