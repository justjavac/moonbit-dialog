///|
/// Run the multi-file picker example with named filters.
fn main {
  let dialog = @dialog.OpenFilesDialog::new(
    title="Open source files",
    directory="C:/Projects",
  ).with_filters([
    @dialog.FileFilter::new("MoonBit and C", ["*.mbt", "*.c", "*.h"]),
    @dialog.FileFilter::new("All Files", ["*"]),
  ])

  match dialog.show() {
    Ok(outcome) =>
      match outcome.selection {
        Selected(paths) => {
          println("Picked \{paths.length()} files with \{outcome.backend}")
          for path in paths {
            println(" - \{path}")
          }
        }
        Cancelled => println("Open files cancelled")
      }
    Err(error) => println("Dialog failed: \{error}")
  }
}
