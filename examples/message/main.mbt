fn main {
  let dialog = @dialog.MessageDialog::new(
    "Hello from the moonbit-dialog example.",
    title="moonbit-dialog",
  )

  match dialog.show() {
    Ok(backend) => println("Dialog shown with \{backend}")
    Err(error) => println("Dialog failed: \{error}")
  }
}
