///|
/// Create an empty label override set that keeps backend defaults.
///
/// This is useful when building dialogs incrementally and wanting to reset any
/// previously configured custom captions.
///
/// Passing the returned value to `with_labels` is equivalent to opting back
/// into native button text for whichever backend eventually shows the dialog.
pub fn DialogLabels::none() -> DialogLabels {
  { accept: "", reject: "", cancel: "" }
}

///|
/// Create a label override set for one-button dialogs.
///
/// The value is stored in the `accept` slot, which is used for `Ok` in message
/// dialogs and the affirmative action in other single-button flows.
///
/// This helper is primarily intended for `MessageDialog` or generic dialogs
/// that use the `Ok` button layout.
pub fn DialogLabels::ok(label : StringView) -> DialogLabels {
  { accept: label.to_string(), reject: "", cancel: "" }
}

///|
/// Create a label override set for OK-cancel dialogs.
///
/// `ok_label` is mapped to the accept button and `cancel_label` is mapped to
/// the cancel button, while the reject slot remains unused.
///
/// Backends that do not support relabeling ignore these values and keep their
/// native captions, so callers can pair this with `supports_custom_labels`.
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
///
/// This mapping matches the response model used by `ConfirmDialog` and generic
/// dialogs with the `YesNo` layout.
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
///
/// It is useful when a flow needs domain-specific wording such as "Save",
/// "Discard", and "Stay" instead of generic yes/no/cancel captions.
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
///
/// This describes the capability of the integration path used by this library,
/// not a general property of every dialog API on the operating system.
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
///
/// The provided labels are interpreted as a one-button caption set when the
/// dialog is eventually shown.
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
///
/// `DialogLabels::yes_no` is usually the most natural companion for this API,
/// though all label helpers write into the same underlying slots.
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
///
/// The stored labels are mapped according to the dialog's `buttons` layout when
/// `ChoiceDialog::show` runs.
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
