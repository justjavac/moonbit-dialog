///|
test "OpenFileDialog::new uses the provided title and directory" {
  let dialog = @dialog.OpenFileDialog::new(
    title="Open a project file",
    directory="C:/Projects",
  )
  assert_eq(dialog.title, "Open a project file")
  assert_eq(dialog.directory, "C:/Projects")
}

///|
test "OpenFileDialog::new uses default values" {
  let dialog = @dialog.OpenFileDialog::new()
  assert_eq(dialog.title, "Open File")
  assert_eq(dialog.directory, "")
}

///|
test "SaveFileDialog::new stores title directory and file name" {
  let dialog = @dialog.SaveFileDialog::new(
    title="Export report",
    directory="C:/Reports",
    file_name="weekly.txt",
  )
  assert_eq(dialog.title, "Export report")
  assert_eq(dialog.directory, "C:/Reports")
  assert_eq(dialog.file_name, "weekly.txt")
}

///|
test "SaveFileDialog::new uses default values" {
  let dialog = @dialog.SaveFileDialog::new()
  assert_eq(dialog.title, "Save File")
  assert_eq(dialog.directory, "")
  assert_eq(dialog.file_name, "")
}

///|
test "SelectFolderDialog::new uses the provided title and directory" {
  let dialog = @dialog.SelectFolderDialog::new(
    title="Select a workspace",
    directory="C:/Workspaces",
  )
  assert_eq(dialog.title, "Select a workspace")
  assert_eq(dialog.directory, "C:/Workspaces")
}

///|
test "SelectFolderDialog::new uses default values" {
  let dialog = @dialog.SelectFolderDialog::new()
  assert_eq(dialog.title, "Select Folder")
  assert_eq(dialog.directory, "")
}
