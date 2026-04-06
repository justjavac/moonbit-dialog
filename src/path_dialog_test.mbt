///|
test "OpenFileDialog::new uses the provided title and directory" {
  let dialog = @dialog.OpenFileDialog::new(
    title="Open a project file",
    directory="C:/Projects",
  )
  assert_eq(dialog.title, "Open a project file")
  assert_eq(dialog.directory, "C:/Projects")
  assert_eq(dialog.filters, [])
}

///|
test "OpenFileDialog::new uses default values" {
  let dialog = @dialog.OpenFileDialog::new()
  assert_eq(dialog.title, "Open File")
  assert_eq(dialog.directory, "")
  assert_eq(dialog.filters, [])
}

///|
test "FileFilter::new stores its name and patterns" {
  let patterns = ["*.png", "*.jpg"]
  let filter = @dialog.FileFilter::new("Images", patterns)
  patterns.push("*.gif")
  assert_eq(filter.name, "Images")
  assert_eq(filter.patterns, ["*.png", "*.jpg"])
}

///|
test "OpenFileDialog::with_filters replaces filters" {
  let filters = [@dialog.FileFilter::new("Text", ["*.txt", "*.md"])]
  let dialog = @dialog.OpenFileDialog::new().with_filters(filters)
  filters.push(@dialog.FileFilter::new("Images", ["*.png"]))
  assert_eq(dialog.filters, [@dialog.FileFilter::new("Text", ["*.txt", "*.md"])])
}

///|
test "OpenFilesDialog::new uses the provided title and directory" {
  let dialog = @dialog.OpenFilesDialog::new(
    title="Open source files",
    directory="C:/Projects",
  )
  assert_eq(dialog.title, "Open source files")
  assert_eq(dialog.directory, "C:/Projects")
  assert_eq(dialog.filters, [])
}

///|
test "OpenFilesDialog::with_filters replaces filters" {
  let filters = [@dialog.FileFilter::new("Source", ["*.mbt", "*.c"])]
  let dialog = @dialog.OpenFilesDialog::new().with_filters(filters)
  filters.push(@dialog.FileFilter::new("Docs", ["*.md"]))
  assert_eq(dialog.filters, [
    @dialog.FileFilter::new("Source", ["*.mbt", "*.c"]),
  ])
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
  assert_eq(dialog.filters, [])
  assert_eq(dialog.default_extension, "")
}

///|
test "SaveFileDialog::new uses default values" {
  let dialog = @dialog.SaveFileDialog::new()
  assert_eq(dialog.title, "Save File")
  assert_eq(dialog.directory, "")
  assert_eq(dialog.file_name, "")
  assert_eq(dialog.filters, [])
  assert_eq(dialog.default_extension, "")
}

///|
test "SaveFileDialog builders keep filters and default extension" {
  let filters = [@dialog.FileFilter::new("Text", ["*.txt"])]
  let dialog = @dialog.SaveFileDialog::new(file_name="report")
    .with_filters(filters)
    .with_default_extension("txt")
  filters.push(@dialog.FileFilter::new("Notes", ["*.md"]))
  assert_eq(dialog.file_name, "report")
  assert_eq(dialog.filters, [@dialog.FileFilter::new("Text", ["*.txt"])])
  assert_eq(dialog.default_extension, "txt")
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
