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
