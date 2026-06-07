///|
/// Run the simple warning dialog example.
fn main {
  match
    @dialog.show_warning(
      "Hello from the moonbit-dialog example.",
      title="moonbit-dialog",
    ) {
    Ok(backend) => println("Dialog shown with \{to_repr(backend)}")
    Err(error) => println("Dialog failed: \{to_repr(error)}")
  }
}
