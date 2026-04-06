///|
/// Native multi-path dialogs write their selections into a larger fixed buffer.
const MULTI_PATH_DIALOG_BUFFER_SIZE : Int = 262144

///|
#borrow(title, directory, filters, path_out)
extern "c" fn open_files_dialog_ffi(
  title : Bytes,
  directory : Bytes,
  filters : Bytes,
  path_out : Bytes,
  path_out_len : Int,
) -> Int = "moonbit_dialog_open_files"

///|
fn decode_multi_path_dialog_outcome(
  platform : Platform,
  raw : Int,
  path_out : Bytes,
) -> Result[MultiPathDialogOutcome, DialogError] {
  if raw == BACKEND_UNAVAILABLE_STATUS {
    Err(BackendUnavailable(platform))
  } else if raw == UNSUPPORTED_PLATFORM_STATUS {
    Err(UnsupportedPlatform(platform))
  } else if raw >= 0 {
    match backend_from_success(raw) {
      Some(backend) =>
        match path_dialog_kind_from_success(raw) {
          PATH_DIALOG_SELECTED_CODE =>
            Ok({ backend, selection: Selected(decode_path_list(path_out)) })
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
fn show_open_files_request(
  title : String,
  directory : String,
  filters : Array[FileFilter],
) -> Result[MultiPathDialogOutcome, DialogError] {
  let platform = current_platform_internal()
  if platform == Unknown {
    return Err(UnsupportedPlatform(Unknown))
  }
  let encoded_filters = encode_file_filters(filters)
  let path_out = Bytes::new(MULTI_PATH_DIALOG_BUFFER_SIZE)
  let raw_result = open_files_dialog_ffi(
    encode_utf8_bytes(title),
    encode_utf8_bytes(directory),
    encoded_filters,
    path_out,
    path_out.length(),
  )
  decode_multi_path_dialog_outcome(platform, raw_result, path_out)
}
