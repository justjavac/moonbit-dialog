///|
/// Create an open-files dialog request.
///
/// `title` defaults to `"Open Files"` and `directory` defaults to an empty
/// string, allowing the backend to choose its default starting location.
/// Newly created dialogs start without any filename filters.
///
/// Attach filters later with `with_filters` if you want to narrow visible file
/// types without rebuilding the request.
pub fn OpenFilesDialog::new(
  title? : StringView = "Open Files",
  directory? : StringView = "",
) -> OpenFilesDialog {
  { title: title.to_string(), directory: directory.to_string(), filters: [] }
}

///|
/// Show an open-files dialog.
///
/// The result contains both the backend that handled the request and either
/// the selected paths or `Cancelled` when the user dismisses the dialog.
///
/// On success the selected paths are returned after decoding the serialized
/// wire format produced by the native backend.
pub fn OpenFilesDialog::show(
  self : OpenFilesDialog,
) -> Result[MultiPathDialogOutcome, DialogError] {
  show_open_files_request(self.title, self.directory, self.filters)
}

///|
/// Open multiple files without constructing `OpenFilesDialog` manually.
///
/// This convenience function uses the same defaults as `OpenFilesDialog::new`
/// and returns the selected paths together with the backend that handled the
/// request.
///
/// Prefer the builder form when you need filters; otherwise this helper keeps a
/// simple multi-file pick to a single call.
pub fn open_files(
  title? : StringView = "Open Files",
  directory? : StringView = "",
) -> Result[MultiPathDialogOutcome, DialogError] {
  OpenFilesDialog::new(title~, directory~).show()
}
