///|
fn main {
  match
    @dialog.show_warning(
      "Hello from the moonbit-dialog example.",
      title="moonbit-dialog",
    ) {
    Ok(backend) => println("Dialog shown with \{backend}")
    Err(error) => println("Dialog failed: \{error}")
  }
}
