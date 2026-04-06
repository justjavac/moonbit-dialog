///|
/// Create a named file filter for open/save dialogs.
///
/// `patterns` is copied defensively so later mutations to the caller-owned
/// array do not affect the dialog request. Backends may ignore filters that do
/// not contain any patterns.
///
/// Typical patterns look like `"*.txt"` or `"*.png"`, and several patterns can
/// be grouped under the same display name.
pub fn FileFilter::new(
  name : StringView,
  patterns : Array[String],
) -> FileFilter {
  { name: name.to_string(), patterns: patterns.copy() }
}

///|
/// Attach filename filters to an open-file dialog.
///
/// This replaces any existing filters, copies the provided array defensively,
/// and preserves the dialog title and starting directory.
///
/// Use this when you want the picker UI to emphasize a narrow set of file
/// types while keeping the original request immutable.
pub fn OpenFileDialog::with_filters(
  self : OpenFileDialog,
  filters : Array[FileFilter],
) -> OpenFileDialog {
  {
    title: self.title,
    directory: self.directory,
    filters: copy_file_filters(filters),
  }
}

///|
/// Attach filename filters to a save-file dialog.
///
/// This replaces any existing filters, copies the provided array defensively,
/// and preserves the dialog title, directory, file name, and default
/// extension.
///
/// The native backend may apply these filters differently, but the request
/// always keeps the full structured filter list.
pub fn SaveFileDialog::with_filters(
  self : SaveFileDialog,
  filters : Array[FileFilter],
) -> SaveFileDialog {
  {
    title: self.title,
    directory: self.directory,
    file_name: self.file_name,
    filters: copy_file_filters(filters),
    default_extension: self.default_extension,
  }
}

///|
/// Set a default file extension for a save-file dialog.
///
/// This updates only the stored default extension and preserves the current
/// title, directory, file name, and filters.
///
/// Callers may pass either `"txt"` or `".txt"`; the native layer normalizes
/// the value before appending it to extensionless paths.
pub fn SaveFileDialog::with_default_extension(
  self : SaveFileDialog,
  extension : StringView,
) -> SaveFileDialog {
  {
    title: self.title,
    directory: self.directory,
    file_name: self.file_name,
    filters: copy_file_filters(self.filters),
    default_extension: extension.to_string(),
  }
}

///|
/// Copy a single file filter and duplicate its pattern array.
fn copy_file_filter(filter : FileFilter) -> FileFilter {
  { name: filter.name, patterns: filter.patterns.copy() }
}

///|
/// Copy a filter list so dialog builders do not retain caller-owned arrays.
fn copy_file_filters(filters : Array[FileFilter]) -> Array[FileFilter] {
  let copied : Array[FileFilter] = []
  for filter in filters {
    copied.push(copy_file_filter(filter))
  }
  copied
}

///|
/// Attach filename filters to a multi-file open dialog.
///
/// This replaces any existing filters, copies the provided array defensively,
/// and preserves the dialog title and starting directory.
///
/// It behaves like `OpenFileDialog::with_filters`, but targets the multi-select
/// request type instead.
pub fn OpenFilesDialog::with_filters(
  self : OpenFilesDialog,
  filters : Array[FileFilter],
) -> OpenFilesDialog {
  {
    title: self.title,
    directory: self.directory,
    filters: copy_file_filters(filters),
  }
}

///|
/// Escape separator characters before serializing a filter or path component.
fn escape_serialized_component(text : String) -> String {
  let builder = StringBuilder::new(size_hint=text.length())
  for ch in text {
    match ch {
      '\\' => builder.write_view("\\\\")
      '\n' => builder.write_view("\\n")
      '\r' => builder.write_view("\\r")
      '\t' => builder.write_view("\\t")
      _ => builder.write_char(ch)
    }
  }
  builder.to_string()
}

///|
/// Restore escaped separator characters from a serialized component.
fn unescape_serialized_component(text : StringView) -> String {
  let builder = StringBuilder::new()
  let mut is_escaped = false
  for ch in text.to_string() {
    if is_escaped {
      match ch {
        'n' => builder.write_char('\n')
        'r' => builder.write_char('\r')
        't' => builder.write_char('\t')
        _ => builder.write_char(ch)
      }
      is_escaped = false
    } else if ch == '\\' {
      is_escaped = true
    } else {
      builder.write_char(ch)
    }
  }
  if is_escaped {
    builder.write_char('\\')
  }
  builder.to_string()
}

///|
/// Serialize file filters into the UTF-8 format expected by the native stubs.
fn encode_file_filters(filters : Array[FileFilter]) -> Bytes {
  if filters.length() == 0 {
    return encode_utf8_bytes("")
  }

  let encoded_filters : Array[String] = []
  for filter in filters {
    if filter.patterns.length() == 0 {
      continue
    }
    let parts : Array[String] = [escape_serialized_component(filter.name)]
    for pattern in filter.patterns {
      parts.push(escape_serialized_component(pattern))
    }
    encoded_filters.push(parts.join("\t"))
  }
  encode_utf8_bytes(encoded_filters.join("\n"))
}

///|
/// Decode a newline-delimited UTF-8 path list emitted by the native stubs.
fn decode_path_list(bytes : Bytes) -> Array[String] {
  let raw = utf8_bytes_to_mbt_string(bytes)
  if raw == "" {
    return []
  }

  let decoded : Array[String] = []
  for component in raw.split("\n") {
    decoded.push(unescape_serialized_component(component))
  }
  decoded
}
