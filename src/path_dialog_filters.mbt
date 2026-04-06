///|
/// Create a named file filter for open/save dialogs.
pub fn FileFilter::new(
  name : StringView,
  patterns : Array[String],
) -> FileFilter {
  { name: name.to_string(), patterns: patterns.copy() }
}

///|
/// Attach filename filters to an open-file dialog.
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
fn copy_file_filter(filter : FileFilter) -> FileFilter {
  { name: filter.name, patterns: filter.patterns.copy() }
}

///|
fn copy_file_filters(filters : Array[FileFilter]) -> Array[FileFilter] {
  let copied : Array[FileFilter] = []
  for filter in filters {
    copied.push(copy_file_filter(filter))
  }
  copied
}

///|
fn escape_file_filter_component(text : String) -> String {
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
fn encode_file_filters(filters : Array[FileFilter]) -> Bytes {
  if filters.length() == 0 {
    return encode_utf8_bytes("")
  }

  let encoded_filters : Array[String] = []
  for filter in filters {
    if filter.patterns.length() == 0 {
      continue
    }
    let parts : Array[String] = [escape_file_filter_component(filter.name)]
    for pattern in filter.patterns {
      parts.push(escape_file_filter_component(pattern))
    }
    encoded_filters.push(parts.join("\t"))
  }
  encode_utf8_bytes(encoded_filters.join("\n"))
}
