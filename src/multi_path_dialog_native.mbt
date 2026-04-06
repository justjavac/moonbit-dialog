///|
/// Native multi-path dialogs write their selections into a larger fixed buffer.
const MULTI_PATH_DIALOG_BUFFER_SIZE : Int = 262144

///|
/// Show a native multi-file dialog and write serialized paths into `path_out`.
#borrow(title, directory, filters, path_out)
extern "c" fn open_files_dialog_ffi(
  title : Bytes,
  directory : Bytes,
  filters : Bytes,
  path_out : Bytes,
  path_out_len : Int,
) -> Int = "moonbit_dialog_open_files"

///|
/// Decode a raw native multi-path dialog status and selected paths.
fn decode_multi_path_dialog_outcome(
  platform : Platform,
  raw : Int,
  path_out : Bytes,
) -> Result[MultiPathDialogOutcome, DialogError] {
  match decode_status_error(platform, raw) {
    Some(error) => Err(error)
    None =>
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
  }
}

///|
/// Run a multi-path dialog callback for a known platform and decode the result.
fn run_multi_path_dialog_request_for_platform(
  platform : Platform,
  callback : (Bytes) -> Int,
) -> Result[MultiPathDialogOutcome, DialogError] {
  let path_out = Bytes::new(MULTI_PATH_DIALOG_BUFFER_SIZE)
  let raw_result = callback(path_out)
  decode_multi_path_dialog_outcome(platform, raw_result, path_out)
}

///|
/// Run a multi-path dialog callback and decode its raw native result.
fn run_multi_path_dialog_request(
  callback : (Bytes) -> Int,
) -> Result[MultiPathDialogOutcome, DialogError] {
  match supported_platform_result() {
    Ok(platform) =>
      run_multi_path_dialog_request_for_platform(platform, callback)
    Err(error) => Err(error)
  }
}

///|
/// Show a multi-file open request through the native FFI and decode the result.
fn show_open_files_request(
  title : String,
  directory : String,
  filters : Array[FileFilter],
) -> Result[MultiPathDialogOutcome, DialogError] {
  let encoded_filters = encode_file_filters(filters)
  run_multi_path_dialog_request(path_out => {
    open_files_dialog_ffi(
      encode_utf8_bytes(title),
      encode_utf8_bytes(directory),
      encoded_filters,
      path_out,
      path_out.length(),
    )
  })
}
