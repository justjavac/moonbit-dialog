///|
/// Create an open-file dialog request.
///
/// `title` defaults to `"Open File"` and `directory` defaults to an empty
/// string, allowing the backend to choose its default starting location.
/// Newly created dialogs start without any filename filters.
///
/// Call `with_filters` on the returned value if you want the picker to focus on
/// a limited set of file types.
pub fn OpenFileDialog::new(
  title? : StringView = "Open File",
  directory? : StringView = "",
) -> OpenFileDialog {
  { title: title.to_string(), directory: directory.to_string(), filters: [] }
}

///|
/// Show an open-file dialog.
///
/// The result contains both the backend that handled the request and either
/// the selected path or `Cancelled` when the user dismisses the dialog.
///
/// This keeps cancellation distinct from operational failure so callers do not
/// need to infer meaning from an empty string.
pub fn OpenFileDialog::show(
  self : OpenFileDialog,
) -> Result[PathDialogOutcome, DialogError] {
  show_open_file_request(self.title, self.directory, self.filters)
}

///|
/// Open a file without constructing `OpenFileDialog` manually.
///
/// This convenience function uses the same defaults as `OpenFileDialog::new`
/// and returns the selected path together with the backend that produced it.
///
/// Choose this helper for the no-filter case and switch to the builder form
/// when you need more request customization.
pub fn open_file(
  title? : StringView = "Open File",
  directory? : StringView = "",
) -> Result[PathDialogOutcome, DialogError] {
  OpenFileDialog::new(title~, directory~).show()
}

///|
/// Create a save-file dialog request.
///
/// `title` defaults to `"Save File"`, `directory` defaults to an empty
/// string, and `file_name` defaults to an empty string. Newly created dialogs
/// start without filters or a default extension.
///
/// The returned request can be refined with `with_filters` and
/// `with_default_extension` before it is shown.
pub fn SaveFileDialog::new(
  title? : StringView = "Save File",
  directory? : StringView = "",
  file_name? : StringView = "",
) -> SaveFileDialog {
  {
    title: title.to_string(),
    directory: directory.to_string(),
    file_name: file_name.to_string(),
    filters: [],
    default_extension: "",
  }
}

///|
/// Show a save-file dialog.
///
/// The result contains both the backend that handled the request and either
/// the chosen save path or `Cancelled` when the user dismisses the dialog.
///
/// When the chosen path has no extension, the native layer may append the
/// configured default extension before returning the final selection.
pub fn SaveFileDialog::show(
  self : SaveFileDialog,
) -> Result[PathDialogOutcome, DialogError] {
  show_save_file_request(
    self.title,
    self.directory,
    self.file_name,
    self.filters,
    self.default_extension,
  )
}

///|
/// Save a file without constructing `SaveFileDialog` manually.
///
/// This convenience function uses the same defaults as `SaveFileDialog::new`
/// and returns the resulting path selection together with the backend that
/// handled the request.
///
/// Prefer the builder form when you need filters, a reusable initial file name,
/// or save-time extension behavior.
pub fn save_file(
  title? : StringView = "Save File",
  directory? : StringView = "",
  file_name? : StringView = "",
) -> Result[PathDialogOutcome, DialogError] {
  SaveFileDialog::new(title~, directory~, file_name~).show()
}

///|
/// Create a folder-selection dialog request.
///
/// `title` defaults to `"Select Folder"` and `directory` defaults to an empty
/// string so the backend can choose its default starting location.
///
/// This request type stays minimal because folder pickers do not use file
/// filters or default extensions.
pub fn SelectFolderDialog::new(
  title? : StringView = "Select Folder",
  directory? : StringView = "",
) -> SelectFolderDialog {
  { title: title.to_string(), directory: directory.to_string() }
}

///|
/// Show a folder-selection dialog.
///
/// The result contains both the backend that handled the request and either
/// the selected folder path or `Cancelled`.
///
/// Like the file pickers, dismissal is returned as data rather than being
/// treated as an operational error.
pub fn SelectFolderDialog::show(
  self : SelectFolderDialog,
) -> Result[PathDialogOutcome, DialogError] {
  show_select_folder_request(self.title, self.directory)
}

///|
/// Select a folder without constructing `SelectFolderDialog` manually.
///
/// This convenience function uses the same defaults as
/// `SelectFolderDialog::new` and returns the resulting folder selection
/// together with the backend that handled the request.
///
/// It is the shortest entry point when you only need a one-off folder choice.
pub fn select_folder(
  title? : StringView = "Select Folder",
  directory? : StringView = "",
) -> Result[PathDialogOutcome, DialogError] {
  SelectFolderDialog::new(title~, directory~).show()
}
