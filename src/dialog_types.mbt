///|
/// The detected operating system for the current native process.
///
/// `Unknown` is returned when the runtime is not one of the currently
/// supported desktop platforms.
pub(all) enum Platform {
  Windows
  MacOS
  Linux
  Unknown
} derive(Eq, Show)

///|
/// The concrete backend that displayed a message dialog.
///
/// Windows and macOS use direct native APIs, while Linux tries several common
/// desktop helpers without going through a shell.
pub(all) enum DialogBackend {
  WindowsWin32
  MacOSCoreFoundation
  LinuxZenity
  LinuxKDialog
  LinuxXMessage
} derive(Eq, Show)

///|
/// The requested severity or intent of a dialog.
///
/// Backends map these levels to the closest native icon or emphasis style they
/// support.
pub(all) enum DialogLevel {
  Info
  Warning
  Error
  Question
} derive(Eq, Show)

///|
/// The standard button combinations supported by the generic dialog API.
pub(all) enum DialogButtons {
  Ok
  OkCancel
  YesNo
  YesNoCancel
} derive(Eq, Show)

///|
/// The user's response to a dialog.
pub(all) enum DialogResponse {
  Ok
  Cancel
  Yes
  No
} derive(Eq, Show)

///|
/// A completed dialog result including the backend and the chosen response.
pub struct DialogOutcome {
  backend : DialogBackend
  response : DialogResponse
} derive(Eq, Show)

///|
/// Errors returned when a dialog cannot be shown.
///
/// - `UnsupportedPlatform` means the current OS is outside the current scope.
/// - `BackendUnavailable` means no supported Linux dialog helper was found.
/// - `BackendFailed` means a chosen backend or native API reported failure.
pub(all) enum DialogError {
  UnsupportedPlatform(Platform)
  BackendUnavailable(Platform)
  BackendFailed(DialogBackend, Int)
} derive(Eq, Show)

///|
/// A message dialog request.
///
/// This type keeps the public API explicit and easy to extend later when the
/// module grows beyond a single message box implementation.
pub struct MessageDialog {
  title : String
  message : String
  level : DialogLevel
} derive(Eq, Show)

///|
/// A yes-or-no confirmation dialog request.
pub struct ConfirmDialog {
  title : String
  message : String
  level : DialogLevel
} derive(Eq, Show)

///|
/// A generic dialog request with one of the standard button combinations.
pub struct ChoiceDialog {
  title : String
  message : String
  level : DialogLevel
  buttons : DialogButtons
} derive(Eq, Show)
