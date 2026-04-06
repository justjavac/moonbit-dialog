///|
/// The result of a path-based dialog.
pub(all) enum PathDialogSelection {
  Selected(String)
  Cancelled
} derive(Eq, Show)

///|
/// A completed path dialog result including the backend and the selection.
pub struct PathDialogOutcome {
  backend : DialogBackend
  selection : PathDialogSelection
} derive(Eq, Show)

///|
/// An open-file dialog request.
pub struct OpenFileDialog {
  title : String
  directory : String
} derive(Eq, Show)

///|
/// A save-file dialog request.
pub struct SaveFileDialog {
  title : String
  directory : String
  file_name : String
} derive(Eq, Show)

///|
/// A folder-selection dialog request.
pub struct SelectFolderDialog {
  title : String
  directory : String
} derive(Eq, Show)
