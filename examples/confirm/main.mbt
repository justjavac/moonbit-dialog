///|
/// Run the yes-no confirmation dialog example.
fn main {
  match
    @dialog.ask_yes_no("Overwrite the generated files?", title="moonbit-dialog") {
    Ok(outcome) =>
      match outcome.response {
        Yes => println("Confirmed with \{to_repr(outcome.backend)}")
        No => println("Declined on \{to_repr(outcome.backend)}")
        _ => println("Unexpected response: \{to_repr(outcome.response)}")
      }
    Err(error) => println("Dialog failed: \{to_repr(error)}")
  }
}
