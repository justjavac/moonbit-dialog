///|
fn main {
  let filters = [
    @dialog.FileFilter::new("Text Files", ["*.txt", "*.md"]),
    @dialog.FileFilter::new("All Files", ["*"]),
  ]

  match
    @dialog.OpenFileDialog::new(
      title="Open a document",
      directory="C:/Projects",
    )
    .with_filters(filters)
    .show() {
    Ok(outcome) =>
      match outcome.selection {
        Selected(path) => println("Opened \{path} with \{outcome.backend}")
        Cancelled => println("Open file cancelled")
      }
    Err(error) => println("Open file failed: \{error}")
  }

  match
    @dialog.SaveFileDialog::new(
      title="Export report",
      directory="C:/Projects",
      file_name="report",
    )
    .with_filters(filters)
    .with_default_extension("txt")
    .show() {
    Ok(outcome) =>
      match outcome.selection {
        Selected(path) => println("Saving to \{path} with \{outcome.backend}")
        Cancelled => println("Save file cancelled")
      }
    Err(error) => println("Save file failed: \{error}")
  }

  match
    @dialog.select_folder(
      title="Select a workspace folder",
      directory="C:/Projects",
    ) {
    Ok(outcome) =>
      match outcome.selection {
        Selected(path) =>
          println("Selected folder \{path} with \{outcome.backend}")
        Cancelled => println("Folder selection cancelled")
      }
    Err(error) => println("Select folder failed: \{error}")
  }
}
