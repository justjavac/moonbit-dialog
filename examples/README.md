# Examples

Run each example from the examples submodule:

```bash
moon -C examples run message
moon -C examples run confirm
moon -C examples run choice
moon -C examples run paths
moon -C examples run open_files
```

Each package is intentionally small and focuses on one slice of the API:

- `message`: severity helpers and a simple message dialog
- `confirm`: yes/no confirmation flow
- `choice`: standard buttons plus custom labels
- `paths`: open/save/folder dialogs with filters and default extensions
- `open_files`: multi-file selection
