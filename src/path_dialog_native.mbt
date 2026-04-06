///|
/// Native path-selection dialogs write their selected path into a fixed buffer.
const PATH_DIALOG_BUFFER_SIZE : Int = 65536

///|
/// A successful path dialog wrote a selected path into the output buffer.
const PATH_DIALOG_SELECTED_CODE : Int = 0

///|
/// A successful path dialog was canceled by the user.
const PATH_DIALOG_CANCELLED_CODE : Int = 1

///|
#borrow(title, directory, filters, path_out)
extern "c" fn open_file_dialog_ffi(
  title : Bytes,
  directory : Bytes,
  filters : Bytes,
  path_out : Bytes,
  path_out_len : Int,
) -> Int = "moonbit_dialog_open_file"

///|
#borrow(title, directory, file_name, filters, default_extension, path_out)
extern "c" fn save_file_dialog_ffi(
  title : Bytes,
  directory : Bytes,
  file_name : Bytes,
  filters : Bytes,
  default_extension : Bytes,
  path_out : Bytes,
  path_out_len : Int,
) -> Int = "moonbit_dialog_save_file"

///|
#borrow(title, directory, path_out)
extern "c" fn select_folder_dialog_ffi(
  title : Bytes,
  directory : Bytes,
  path_out : Bytes,
  path_out_len : Int,
) -> Int = "moonbit_dialog_select_folder"

///|
fn path_dialog_kind_from_success(raw : Int) -> Int {
  raw / SUCCESS_RESPONSE_STRIDE
}

///|
fn decode_path_dialog_outcome(
  platform : Platform,
  raw : Int,
  path_out : Bytes,
) -> Result[PathDialogOutcome, DialogError] {
  if raw == BACKEND_UNAVAILABLE_STATUS {
    Err(BackendUnavailable(platform))
  } else if raw == UNSUPPORTED_PLATFORM_STATUS {
    Err(UnsupportedPlatform(platform))
  } else if raw >= 0 {
    match backend_from_success(raw) {
      Some(backend) =>
        match path_dialog_kind_from_success(raw) {
          PATH_DIALOG_SELECTED_CODE =>
            Ok({
              backend,
              selection: Selected(utf8_bytes_to_mbt_string(path_out)),
            })
          PATH_DIALOG_CANCELLED_CODE => Ok({ backend, selection: Cancelled })
          _ => Err(UnsupportedPlatform(platform))
        }
      None => Err(UnsupportedPlatform(platform))
    }
  } else {
    match decode_failure_status(raw) {
      Some((backend, detail)) => Err(BackendFailed(backend, detail))
      None => Err(UnsupportedPlatform(platform))
    }
  }
}

///|
fn show_open_file_request(
  title : String,
  directory : String,
  filters : Array[FileFilter],
) -> Result[PathDialogOutcome, DialogError] {
  let platform = current_platform_internal()
  if platform == Unknown {
    return Err(UnsupportedPlatform(Unknown))
  }
  let encoded_filters = encode_file_filters(filters)
  let path_out = Bytes::new(PATH_DIALOG_BUFFER_SIZE)
  let raw_result = open_file_dialog_ffi(
    encode_utf8_bytes(title),
    encode_utf8_bytes(directory),
    encoded_filters,
    path_out,
    path_out.length(),
  )
  decode_path_dialog_outcome(platform, raw_result, path_out)
}

///|
fn show_save_file_request(
  title : String,
  directory : String,
  file_name : String,
  filters : Array[FileFilter],
  default_extension : String,
) -> Result[PathDialogOutcome, DialogError] {
  let platform = current_platform_internal()
  if platform == Unknown {
    return Err(UnsupportedPlatform(Unknown))
  }
  let encoded_filters = encode_file_filters(filters)
  let path_out = Bytes::new(PATH_DIALOG_BUFFER_SIZE)
  let raw_result = save_file_dialog_ffi(
    encode_utf8_bytes(title),
    encode_utf8_bytes(directory),
    encode_utf8_bytes(file_name),
    encoded_filters,
    encode_utf8_bytes(default_extension),
    path_out,
    path_out.length(),
  )
  decode_path_dialog_outcome(platform, raw_result, path_out)
}

///|
fn show_select_folder_request(
  title : String,
  directory : String,
) -> Result[PathDialogOutcome, DialogError] {
  let platform = current_platform_internal()
  if platform == Unknown {
    return Err(UnsupportedPlatform(Unknown))
  }
  let path_out = Bytes::new(PATH_DIALOG_BUFFER_SIZE)
  let raw_result = select_folder_dialog_ffi(
    encode_utf8_bytes(title),
    encode_utf8_bytes(directory),
    path_out,
    path_out.length(),
  )
  decode_path_dialog_outcome(platform, raw_result, path_out)
}
