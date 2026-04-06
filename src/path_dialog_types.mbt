///|
/// The result of a path-based dialog.
pub(all) enum PathDialogSelection {
  Selected(String)
  Cancelled
} derive(Eq, Show)

///|
/// A named set of filename patterns for open/save dialogs.
pub struct FileFilter {
  name : String
  patterns : Array[String]
} derive(Eq, Show)

///|
/// A completed path dialog result including the backend and the selection.
pub struct PathDialogOutcome {
  backend : DialogBackend
  selection : PathDialogSelection
} derive(Eq, Show)

///|
/// The result of a multi-path dialog.
pub(all) enum MultiPathSelection {
  Selected(Array[String])
  Cancelled
} derive(Eq, Show)

///|
/// A completed multi-path dialog result including the backend and the selection.
pub struct MultiPathDialogOutcome {
  backend : DialogBackend
  selection : MultiPathSelection
} derive(Eq, Show)

///|
/// An open-file dialog request.
pub struct OpenFileDialog {
  title : String
  directory : String
  filters : Array[FileFilter]
} derive(Eq, Show)

///|
/// A save-file dialog request.
pub struct SaveFileDialog {
  title : String
  directory : String
  file_name : String
  filters : Array[FileFilter]
  default_extension : String
} derive(Eq, Show)

///|
/// A multi-file open dialog request.
pub struct OpenFilesDialog {
  title : String
  directory : String
  filters : Array[FileFilter]
} derive(Eq, Show)

///|
/// A folder-selection dialog request.
pub struct SelectFolderDialog {
  title : String
  directory : String
} derive(Eq, Show)
