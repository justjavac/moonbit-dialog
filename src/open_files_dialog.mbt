///|
/// Create an open-files dialog request.
pub fn OpenFilesDialog::new(
  title? : StringView = "Open Files",
  directory? : StringView = "",
) -> OpenFilesDialog {
  { title: title.to_string(), directory: directory.to_string(), filters: [] }
}

///|
/// Show an open-files dialog.
pub fn OpenFilesDialog::show(
  self : OpenFilesDialog,
) -> Result[MultiPathDialogOutcome, DialogError] {
  show_open_files_request(self.title, self.directory, self.filters)
}

///|
/// Open multiple files without constructing `OpenFilesDialog` manually.
pub fn open_files(
  title? : StringView = "Open Files",
  directory? : StringView = "",
) -> Result[MultiPathDialogOutcome, DialogError] {
  OpenFilesDialog::new(title~, directory~).show()
}
