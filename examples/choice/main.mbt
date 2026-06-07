///|
/// Run the standard-button dialog example with custom labels.
fn main {
  let dialog = @dialog.ChoiceDialog::new(
    "Save changes before closing?",
    title="moonbit-dialog",
    buttons=YesNoCancel,
    level=Question,
  ).with_labels(@dialog.DialogLabels::yes_no_cancel("Save", "Discard", "Stay"))

  match dialog.show() {
    Ok(outcome) => {
      println(
        "Selected \{to_repr(outcome.response)} with \{to_repr(outcome.backend)}",
      )
      println(
        "Custom labels applied: \{@dialog.supports_custom_labels(outcome.backend)}",
      )
    }
    Err(error) => println("Dialog failed: \{to_repr(error)}")
  }
}
