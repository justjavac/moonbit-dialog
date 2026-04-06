///|
fn main {
  match
    @dialog.ask_yes_no("Overwrite the generated files?", title="moonbit-dialog") {
    Ok(outcome) =>
      match outcome.response {
        Yes => println("Confirmed with \{outcome.backend}")
        No => println("Declined on \{outcome.backend}")
        _ => println("Unexpected response: \{outcome.response}")
      }
    Err(error) => println("Dialog failed: \{error}")
  }
}
