///|
/// Create an open-file dialog request.
pub fn OpenFileDialog::new(
  title? : StringView = "Open File",
  directory? : StringView = "",
) -> OpenFileDialog {
  { title: title.to_string(), directory: directory.to_string() }
}

///|
/// Show an open-file dialog.
pub fn OpenFileDialog::show(
  self : OpenFileDialog,
) -> Result[PathDialogOutcome, DialogError] {
  show_open_file_request(self.title, self.directory)
}

///|
/// Open a file without constructing `OpenFileDialog` manually.
pub fn open_file(
  title? : StringView = "Open File",
  directory? : StringView = "",
) -> Result[PathDialogOutcome, DialogError] {
  OpenFileDialog::new(title~, directory~).show()
}

///|
/// Create a save-file dialog request.
pub fn SaveFileDialog::new(
  title? : StringView = "Save File",
  directory? : StringView = "",
  file_name? : StringView = "",
) -> SaveFileDialog {
  {
    title: title.to_string(),
    directory: directory.to_string(),
    file_name: file_name.to_string(),
  }
}

///|
/// Show a save-file dialog.
pub fn SaveFileDialog::show(
  self : SaveFileDialog,
) -> Result[PathDialogOutcome, DialogError] {
  show_save_file_request(self.title, self.directory, self.file_name)
}

///|
/// Save a file without constructing `SaveFileDialog` manually.
pub fn save_file(
  title? : StringView = "Save File",
  directory? : StringView = "",
  file_name? : StringView = "",
) -> Result[PathDialogOutcome, DialogError] {
  SaveFileDialog::new(title~, directory~, file_name~).show()
}

///|
/// Create a folder-selection dialog request.
pub fn SelectFolderDialog::new(
  title? : StringView = "Select Folder",
  directory? : StringView = "",
) -> SelectFolderDialog {
  { title: title.to_string(), directory: directory.to_string() }
}

///|
/// Show a folder-selection dialog.
pub fn SelectFolderDialog::show(
  self : SelectFolderDialog,
) -> Result[PathDialogOutcome, DialogError] {
  show_select_folder_request(self.title, self.directory)
}

///|
/// Select a folder without constructing `SelectFolderDialog` manually.
pub fn select_folder(
  title? : StringView = "Select Folder",
  directory? : StringView = "",
) -> Result[PathDialogOutcome, DialogError] {
  SelectFolderDialog::new(title~, directory~).show()
}
