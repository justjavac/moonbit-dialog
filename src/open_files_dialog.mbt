///|
/// Create an open-files dialog request.
///
/// `title` defaults to `"Open Files"` and `directory` defaults to an empty
/// string, allowing the backend to choose its default starting location.
/// Newly created dialogs start without any filename filters.
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
pub fn open_files(
  title? : StringView = "Open Files",
  directory? : StringView = "",
) -> Result[MultiPathDialogOutcome, DialogError] {
  OpenFilesDialog::new(title~, directory~).show()
}
