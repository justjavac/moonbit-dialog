#include <moonbit.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#pragma comment(lib, "user32.lib")
#elif defined(__APPLE__)
#include <dlfcn.h>
#elif defined(__linux__)
#include <errno.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#define DIALOG_BACKEND_WINDOWS_WIN32 0
#define DIALOG_BACKEND_MACOS_CORE_FOUNDATION 1
#define DIALOG_BACKEND_LINUX_ZENITY 2
#define DIALOG_BACKEND_LINUX_KDIALOG 3
#define DIALOG_BACKEND_LINUX_XMESSAGE 4

#define DIALOG_LEVEL_INFO 0
#define DIALOG_LEVEL_WARNING 1
#define DIALOG_LEVEL_ERROR 2
#define DIALOG_LEVEL_QUESTION 3

#define DIALOG_BUTTONS_OK 0
#define DIALOG_BUTTONS_OK_CANCEL 1
#define DIALOG_BUTTONS_YES_NO 2
#define DIALOG_BUTTONS_YES_NO_CANCEL 3

#define DIALOG_RESPONSE_OK 0
#define DIALOG_RESPONSE_CANCEL 1
#define DIALOG_RESPONSE_YES 2
#define DIALOG_RESPONSE_NO 3

#define DIALOG_RESPONSE_STRIDE 10

#define DIALOG_STATUS_BACKEND_UNAVAILABLE -1
#define DIALOG_STATUS_UNSUPPORTED_PLATFORM -2

#define DIALOG_FAILURE_BASE 1000000
#define DIALOG_FAILURE_STRIDE 100000

static int32_t moonbit_dialog_encode_success(
  int32_t backend,
  int32_t response
) {
  return backend + response * DIALOG_RESPONSE_STRIDE;
}

static int32_t moonbit_dialog_encode_failure(int32_t backend, int32_t detail) {
  if (detail < 0) {
    detail = -detail;
  }
  if (detail >= DIALOG_FAILURE_STRIDE) {
    detail = DIALOG_FAILURE_STRIDE - 1;
  }
  return -(DIALOG_FAILURE_BASE + backend * DIALOG_FAILURE_STRIDE + detail);
}

static int moonbit_dialog_has_label(const char* text) {
  return text != NULL && text[0] != '\0';
}

static const char* moonbit_dialog_pick_label(
  const char* custom,
  const char* fallback
) {
  return moonbit_dialog_has_label(custom) ? custom : fallback;
}

static const char* moonbit_dialog_accept_label(
  int32_t buttons,
  const char* custom
) {
  switch (buttons) {
    case DIALOG_BUTTONS_YES_NO:
    case DIALOG_BUTTONS_YES_NO_CANCEL:
      return moonbit_dialog_pick_label(custom, "Yes");
    default:
      return moonbit_dialog_pick_label(custom, "OK");
  }
}

static const char* moonbit_dialog_reject_label(
  int32_t buttons,
  const char* custom
) {
  switch (buttons) {
    case DIALOG_BUTTONS_YES_NO:
    case DIALOG_BUTTONS_YES_NO_CANCEL:
      return moonbit_dialog_pick_label(custom, "No");
    default:
      return moonbit_dialog_pick_label(custom, "");
  }
}

static const char* moonbit_dialog_cancel_label(
  int32_t buttons,
  const char* custom
) {
  switch (buttons) {
    case DIALOG_BUTTONS_OK_CANCEL:
    case DIALOG_BUTTONS_YES_NO_CANCEL:
      return moonbit_dialog_pick_label(custom, "Cancel");
    default:
      return moonbit_dialog_pick_label(custom, "");
  }
}

static int32_t moonbit_dialog_response_for_close(int32_t buttons) {
  switch (buttons) {
    case DIALOG_BUTTONS_OK:
      return DIALOG_RESPONSE_OK;
    case DIALOG_BUTTONS_OK_CANCEL:
      return DIALOG_RESPONSE_CANCEL;
    case DIALOG_BUTTONS_YES_NO:
      return DIALOG_RESPONSE_NO;
    case DIALOG_BUTTONS_YES_NO_CANCEL:
      return DIALOG_RESPONSE_CANCEL;
    default:
      return DIALOG_RESPONSE_CANCEL;
  }
}

static int32_t moonbit_dialog_response_from_index(
  int32_t buttons,
  int index
) {
  switch (buttons) {
    case DIALOG_BUTTONS_OK:
      return DIALOG_RESPONSE_OK;
    case DIALOG_BUTTONS_OK_CANCEL:
      return index == 0 ? DIALOG_RESPONSE_OK : DIALOG_RESPONSE_CANCEL;
    case DIALOG_BUTTONS_YES_NO:
      return index == 0 ? DIALOG_RESPONSE_YES : DIALOG_RESPONSE_NO;
    case DIALOG_BUTTONS_YES_NO_CANCEL:
      if (index == 0) {
        return DIALOG_RESPONSE_YES;
      }
      if (index == 1) {
        return DIALOG_RESPONSE_NO;
      }
      return DIALOG_RESPONSE_CANCEL;
    default:
      return DIALOG_RESPONSE_CANCEL;
  }
}

#if defined(_WIN32)
static wchar_t* moonbit_dialog_utf8_to_wide(const char* text) {
  int length = MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);
  if (length <= 0) {
    return NULL;
  }

  wchar_t* wide = (wchar_t*)malloc((size_t)length * sizeof(wchar_t));
  if (wide == NULL) {
    return NULL;
  }

  if (MultiByteToWideChar(CP_UTF8, 0, text, -1, wide, length) == 0) {
    free(wide);
    return NULL;
  }

  return wide;
}

static UINT moonbit_dialog_windows_button_flags(int32_t buttons) {
  switch (buttons) {
    case DIALOG_BUTTONS_OK_CANCEL:
      return MB_OKCANCEL;
    case DIALOG_BUTTONS_YES_NO:
      return MB_YESNO;
    case DIALOG_BUTTONS_YES_NO_CANCEL:
      return MB_YESNOCANCEL;
    default:
      return MB_OK;
  }
}

static UINT moonbit_dialog_windows_icon_flags(int32_t level) {
  switch (level) {
    case DIALOG_LEVEL_WARNING:
      return MB_ICONWARNING;
    case DIALOG_LEVEL_ERROR:
      return MB_ICONERROR;
    case DIALOG_LEVEL_QUESTION:
      return MB_ICONQUESTION;
    default:
      return MB_ICONINFORMATION;
  }
}

static int32_t moonbit_dialog_show_windows(
  int32_t level,
  int32_t buttons,
  const char* title_utf8,
  const char* message_utf8,
  const char* accept_label,
  const char* reject_label,
  const char* cancel_label
) {
  (void)accept_label;
  (void)reject_label;
  (void)cancel_label;

  wchar_t* title = moonbit_dialog_utf8_to_wide(title_utf8);
  if (title == NULL) {
    DWORD error = GetLastError();
    return moonbit_dialog_encode_failure(
      DIALOG_BACKEND_WINDOWS_WIN32,
      error == 0 ? 1 : (int32_t)error
    );
  }

  wchar_t* message = moonbit_dialog_utf8_to_wide(message_utf8);
  if (message == NULL) {
    DWORD error = GetLastError();
    free(title);
    return moonbit_dialog_encode_failure(
      DIALOG_BACKEND_WINDOWS_WIN32,
      error == 0 ? 1 : (int32_t)error
    );
  }

  int result = MessageBoxW(
    NULL,
    message,
    title,
    moonbit_dialog_windows_button_flags(buttons) |
      moonbit_dialog_windows_icon_flags(level) |
      MB_SETFOREGROUND
  );
  free(message);
  free(title);

  if (result == 0) {
    DWORD error = GetLastError();
    return moonbit_dialog_encode_failure(
      DIALOG_BACKEND_WINDOWS_WIN32,
      error == 0 ? 1 : (int32_t)error
    );
  }

  switch (result) {
    case IDOK:
      return moonbit_dialog_encode_success(
        DIALOG_BACKEND_WINDOWS_WIN32,
        DIALOG_RESPONSE_OK
      );
    case IDCANCEL:
      return moonbit_dialog_encode_success(
        DIALOG_BACKEND_WINDOWS_WIN32,
        moonbit_dialog_response_for_close(buttons)
      );
    case IDYES:
      return moonbit_dialog_encode_success(
        DIALOG_BACKEND_WINDOWS_WIN32,
        DIALOG_RESPONSE_YES
      );
    case IDNO:
      return moonbit_dialog_encode_success(
        DIALOG_BACKEND_WINDOWS_WIN32,
        DIALOG_RESPONSE_NO
      );
    default:
      return moonbit_dialog_encode_failure(
        DIALOG_BACKEND_WINDOWS_WIN32,
        result
      );
  }
}
#endif

#if defined(__APPLE__)
typedef const void* CFAllocatorRef;
typedef const void* CFStringRef;
typedef const void* CFURLRef;
typedef const void* CFTypeRef;
typedef double CFTimeInterval;
typedef int32_t SInt32;
typedef uint32_t CFOptionFlags;
typedef uint32_t CFStringEncoding;

typedef CFStringRef (*moonbit_dialog_cf_string_create_with_cstring_fn)(
  CFAllocatorRef,
  const char*,
  CFStringEncoding
);
typedef SInt32 (*moonbit_dialog_cf_user_notification_display_alert_fn)(
  CFTimeInterval,
  CFOptionFlags,
  CFURLRef,
  CFURLRef,
  CFURLRef,
  CFStringRef,
  CFStringRef,
  CFStringRef,
  CFStringRef,
  CFStringRef,
  CFOptionFlags*
);
typedef void (*moonbit_dialog_cf_release_fn)(CFTypeRef);

#define DIALOG_CF_STRING_ENCODING_UTF8 0x08000100U
#define DIALOG_CF_NOTE_ALERT_LEVEL 1U
#define DIALOG_CF_CAUTION_ALERT_LEVEL 2U
#define DIALOG_CF_STOP_ALERT_LEVEL 0U
#define DIALOG_CF_DEFAULT_RESPONSE 0U
#define DIALOG_CF_ALTERNATE_RESPONSE 1U
#define DIALOG_CF_OTHER_RESPONSE 2U
#define DIALOG_CF_CANCEL_RESPONSE 3U

static void moonbit_dialog_cf_release_if_needed(
  moonbit_dialog_cf_release_fn release_fn,
  CFTypeRef value
) {
  if (value != NULL) {
    release_fn(value);
  }
}

static CFOptionFlags moonbit_dialog_macos_flags(int32_t level) {
  switch (level) {
    case DIALOG_LEVEL_WARNING:
      return DIALOG_CF_CAUTION_ALERT_LEVEL;
    case DIALOG_LEVEL_ERROR:
      return DIALOG_CF_STOP_ALERT_LEVEL;
    default:
      return DIALOG_CF_NOTE_ALERT_LEVEL;
  }
}

static int32_t moonbit_dialog_show_macos(
  int32_t level,
  int32_t buttons,
  const char* title_utf8,
  const char* message_utf8,
  const char* accept_label,
  const char* reject_label,
  const char* cancel_label
) {
  void* core_foundation = dlopen(
    "/System/Library/Frameworks/CoreFoundation.framework/CoreFoundation",
    RTLD_LAZY | RTLD_LOCAL
  );
  if (core_foundation == NULL) {
    return moonbit_dialog_encode_failure(
      DIALOG_BACKEND_MACOS_CORE_FOUNDATION,
      1
    );
  }

  moonbit_dialog_cf_string_create_with_cstring_fn cf_string_create =
    (moonbit_dialog_cf_string_create_with_cstring_fn)dlsym(
      core_foundation,
      "CFStringCreateWithCString"
    );
  moonbit_dialog_cf_user_notification_display_alert_fn display_alert =
    (moonbit_dialog_cf_user_notification_display_alert_fn)dlsym(
      core_foundation,
      "CFUserNotificationDisplayAlert"
    );
  moonbit_dialog_cf_release_fn cf_release =
    (moonbit_dialog_cf_release_fn)dlsym(core_foundation, "CFRelease");

  if (
    cf_string_create == NULL ||
    display_alert == NULL ||
    cf_release == NULL
  ) {
    dlclose(core_foundation);
    return moonbit_dialog_encode_failure(
      DIALOG_BACKEND_MACOS_CORE_FOUNDATION,
      2
    );
  }

  CFStringRef title = cf_string_create(
    NULL,
    title_utf8,
    DIALOG_CF_STRING_ENCODING_UTF8
  );
  CFStringRef message = cf_string_create(
    NULL,
    message_utf8,
    DIALOG_CF_STRING_ENCODING_UTF8
  );
  CFStringRef default_button = cf_string_create(
    NULL,
    moonbit_dialog_accept_label(buttons, accept_label),
    DIALOG_CF_STRING_ENCODING_UTF8
  );
  CFStringRef alternate_button = NULL;
  CFStringRef other_button = NULL;

  if (buttons == DIALOG_BUTTONS_OK_CANCEL) {
    alternate_button = cf_string_create(
      NULL,
      moonbit_dialog_cancel_label(buttons, cancel_label),
      DIALOG_CF_STRING_ENCODING_UTF8
    );
  } else if (buttons == DIALOG_BUTTONS_YES_NO) {
    alternate_button = cf_string_create(
      NULL,
      moonbit_dialog_reject_label(buttons, reject_label),
      DIALOG_CF_STRING_ENCODING_UTF8
    );
  } else if (buttons == DIALOG_BUTTONS_YES_NO_CANCEL) {
    alternate_button = cf_string_create(
      NULL,
      moonbit_dialog_reject_label(buttons, reject_label),
      DIALOG_CF_STRING_ENCODING_UTF8
    );
    other_button = cf_string_create(
      NULL,
      moonbit_dialog_cancel_label(buttons, cancel_label),
      DIALOG_CF_STRING_ENCODING_UTF8
    );
  }

  if (
    title == NULL ||
    message == NULL ||
    default_button == NULL ||
    (buttons != DIALOG_BUTTONS_OK && alternate_button == NULL) ||
    (buttons == DIALOG_BUTTONS_YES_NO_CANCEL && other_button == NULL)
  ) {
    moonbit_dialog_cf_release_if_needed(cf_release, other_button);
    moonbit_dialog_cf_release_if_needed(cf_release, alternate_button);
    moonbit_dialog_cf_release_if_needed(cf_release, default_button);
    moonbit_dialog_cf_release_if_needed(cf_release, message);
    moonbit_dialog_cf_release_if_needed(cf_release, title);
    dlclose(core_foundation);
    return moonbit_dialog_encode_failure(
      DIALOG_BACKEND_MACOS_CORE_FOUNDATION,
      3
    );
  }

  CFOptionFlags response_flags = 0;
  SInt32 status = display_alert(
    0.0,
    moonbit_dialog_macos_flags(level),
    NULL,
    NULL,
    NULL,
    title,
    message,
    default_button,
    alternate_button,
    other_button,
    &response_flags
  );
  moonbit_dialog_cf_release_if_needed(cf_release, other_button);
  moonbit_dialog_cf_release_if_needed(cf_release, alternate_button);
  moonbit_dialog_cf_release_if_needed(cf_release, default_button);
  moonbit_dialog_cf_release_if_needed(cf_release, message);
  moonbit_dialog_cf_release_if_needed(cf_release, title);
  dlclose(core_foundation);

  if (status != 0) {
    return moonbit_dialog_encode_failure(
      DIALOG_BACKEND_MACOS_CORE_FOUNDATION,
      status
    );
  }

  switch (response_flags) {
    case DIALOG_CF_DEFAULT_RESPONSE:
      return moonbit_dialog_encode_success(
        DIALOG_BACKEND_MACOS_CORE_FOUNDATION,
        moonbit_dialog_response_from_index(buttons, 0)
      );
    case DIALOG_CF_ALTERNATE_RESPONSE:
      return moonbit_dialog_encode_success(
        DIALOG_BACKEND_MACOS_CORE_FOUNDATION,
        moonbit_dialog_response_from_index(buttons, 1)
      );
    case DIALOG_CF_OTHER_RESPONSE:
      return moonbit_dialog_encode_success(
        DIALOG_BACKEND_MACOS_CORE_FOUNDATION,
        moonbit_dialog_response_from_index(buttons, 2)
      );
    case DIALOG_CF_CANCEL_RESPONSE:
      return moonbit_dialog_encode_success(
        DIALOG_BACKEND_MACOS_CORE_FOUNDATION,
        moonbit_dialog_response_for_close(buttons)
      );
    default:
      return moonbit_dialog_encode_failure(
        DIALOG_BACKEND_MACOS_CORE_FOUNDATION,
        (int32_t)response_flags
      );
  }
}
#endif

#if defined(__linux__)
extern char** environ;

#define DIALOG_HELPER_MISSING -32768
#define DIALOG_XMESSAGE_BUTTON_BASE 20

static int moonbit_dialog_wait_for_exit(pid_t pid) {
  int status = 0;
  if (waitpid(pid, &status, 0) < 0) {
    return -errno;
  }

  if (WIFEXITED(status)) {
    return WEXITSTATUS(status);
  }

  if (WIFSIGNALED(status)) {
    return 512 + WTERMSIG(status);
  }

  return 513;
}

static int moonbit_dialog_run_program(const char* const argv[]) {
  pid_t pid = 0;
  int spawn_status = posix_spawnp(
    &pid,
    argv[0],
    NULL,
    NULL,
    (char* const*)argv,
    environ
  );

  if (spawn_status == ENOENT) {
    return DIALOG_HELPER_MISSING;
  }
  if (spawn_status != 0) {
    return -spawn_status;
  }

  return moonbit_dialog_wait_for_exit(pid);
}

static void moonbit_dialog_trim_newlines(char* output) {
  size_t length = strlen(output);
  while (length > 0) {
    char current = output[length - 1];
    if (current != '\n' && current != '\r') {
      break;
    }
    output[length - 1] = '\0';
    length--;
  }
}

static int moonbit_dialog_run_program_capture_stdout(
  const char* const argv[],
  char* output,
  size_t output_len
) {
  int pipe_fds[2] = {0, 0};
  if (output_len > 0) {
    output[0] = '\0';
  }

  if (pipe(pipe_fds) != 0) {
    return -errno;
  }

  posix_spawn_file_actions_t actions;
  int action_status = posix_spawn_file_actions_init(&actions);
  if (action_status != 0) {
    close(pipe_fds[0]);
    close(pipe_fds[1]);
    return -action_status;
  }

  posix_spawn_file_actions_addclose(&actions, pipe_fds[0]);
  posix_spawn_file_actions_adddup2(&actions, pipe_fds[1], STDOUT_FILENO);
  posix_spawn_file_actions_addclose(&actions, pipe_fds[1]);

  pid_t pid = 0;
  int spawn_status = posix_spawnp(
    &pid,
    argv[0],
    &actions,
    NULL,
    (char* const*)argv,
    environ
  );
  posix_spawn_file_actions_destroy(&actions);
  close(pipe_fds[1]);

  if (spawn_status == ENOENT) {
    close(pipe_fds[0]);
    return DIALOG_HELPER_MISSING;
  }
  if (spawn_status != 0) {
    close(pipe_fds[0]);
    return -spawn_status;
  }

  if (output_len > 0) {
    size_t filled = 0;
    while (filled + 1 < output_len) {
      ssize_t count = read(
        pipe_fds[0],
        output + filled,
        output_len - filled - 1
      );
      if (count <= 0) {
        break;
      }
      filled += (size_t)count;
    }
    output[filled] = '\0';
    moonbit_dialog_trim_newlines(output);
  } else {
    char discard[64];
    while (read(pipe_fds[0], discard, sizeof(discard)) > 0) {
    }
  }

  close(pipe_fds[0]);
  return moonbit_dialog_wait_for_exit(pid);
}

static int moonbit_dialog_xmessage_label_safe(const char* label) {
  if (!moonbit_dialog_has_label(label)) {
    return 1;
  }
  return strchr(label, ':') == NULL && strchr(label, ',') == NULL;
}

static int moonbit_dialog_has_custom_labels(
  const char* accept_label,
  const char* reject_label,
  const char* cancel_label
) {
  return moonbit_dialog_has_label(accept_label) ||
    moonbit_dialog_has_label(reject_label) ||
    moonbit_dialog_has_label(cancel_label);
}

static const char* moonbit_dialog_zenity_mode(
  int32_t buttons,
  int32_t level
) {
  if (buttons != DIALOG_BUTTONS_OK) {
    return "--question";
  }

  switch (level) {
    case DIALOG_LEVEL_WARNING:
      return "--warning";
    case DIALOG_LEVEL_ERROR:
      return "--error";
    case DIALOG_LEVEL_QUESTION:
      return "--question";
    default:
      return "--info";
  }
}

static const char* moonbit_dialog_zenity_icon_name(int32_t level) {
  switch (level) {
    case DIALOG_LEVEL_WARNING:
      return "dialog-warning";
    case DIALOG_LEVEL_ERROR:
      return "dialog-error";
    case DIALOG_LEVEL_QUESTION:
      return "dialog-question";
    default:
      return "dialog-information";
  }
}

static int32_t moonbit_dialog_show_linux_zenity(
  int32_t level,
  int32_t buttons,
  const char* title_utf8,
  const char* message_utf8,
  const char* accept_label,
  const char* reject_label,
  const char* cancel_label
) {
  const char* ok_label = moonbit_dialog_accept_label(buttons, accept_label);
  const char* no_label = moonbit_dialog_reject_label(buttons, reject_label);
  const char* close_label = moonbit_dialog_cancel_label(buttons, cancel_label);
  const char* argv[20];
  int index = 0;

  argv[index++] = "zenity";
  argv[index++] = moonbit_dialog_zenity_mode(buttons, level);
  argv[index++] = "--no-wrap";
  argv[index++] = "--window-icon";
  argv[index++] = moonbit_dialog_zenity_icon_name(level);
  argv[index++] = "--title";
  argv[index++] = title_utf8;
  argv[index++] = "--text";
  argv[index++] = message_utf8;

  if (buttons == DIALOG_BUTTONS_OK && level == DIALOG_LEVEL_QUESTION) {
    argv[index++] = "--no-cancel";
  }

  if (moonbit_dialog_has_label(ok_label)) {
    argv[index++] = "--ok-label";
    argv[index++] = ok_label;
  }

  if (
    buttons == DIALOG_BUTTONS_OK_CANCEL ||
    buttons == DIALOG_BUTTONS_YES_NO ||
    buttons == DIALOG_BUTTONS_YES_NO_CANCEL
  ) {
    argv[index++] = "--cancel-label";
    argv[index++] =
      buttons == DIALOG_BUTTONS_YES_NO ? no_label : close_label;
  }

  if (buttons == DIALOG_BUTTONS_YES_NO_CANCEL) {
    argv[index++] = "--extra-button";
    argv[index++] = no_label;
  }

  argv[index] = NULL;

  char output[128];
  int status =
    buttons == DIALOG_BUTTONS_YES_NO_CANCEL
      ? moonbit_dialog_run_program_capture_stdout(
          argv,
          output,
          sizeof(output)
        )
      : moonbit_dialog_run_program(argv);

  if (status == DIALOG_HELPER_MISSING) {
    return DIALOG_STATUS_BACKEND_UNAVAILABLE;
  }
  if (status == 0) {
    int32_t response =
      buttons == DIALOG_BUTTONS_YES_NO_CANCEL && strcmp(output, no_label) == 0
        ? DIALOG_RESPONSE_NO
        : moonbit_dialog_response_from_index(buttons, 0);
    return moonbit_dialog_encode_success(DIALOG_BACKEND_LINUX_ZENITY, response);
  }
  if (status == 1 || status == 5) {
    return moonbit_dialog_encode_success(
      DIALOG_BACKEND_LINUX_ZENITY,
      moonbit_dialog_response_for_close(buttons)
    );
  }

  return moonbit_dialog_encode_failure(DIALOG_BACKEND_LINUX_ZENITY, status);
}

static const char* moonbit_dialog_kdialog_command(
  int32_t buttons,
  int32_t level
) {
  switch (buttons) {
    case DIALOG_BUTTONS_OK:
      switch (level) {
        case DIALOG_LEVEL_WARNING:
          return "--sorry";
        case DIALOG_LEVEL_ERROR:
          return "--error";
        default:
          return "--msgbox";
      }
    case DIALOG_BUTTONS_YES_NO:
      return level == DIALOG_LEVEL_WARNING || level == DIALOG_LEVEL_ERROR
        ? "--warningyesno"
        : "--yesno";
    case DIALOG_BUTTONS_YES_NO_CANCEL:
      return level == DIALOG_LEVEL_WARNING || level == DIALOG_LEVEL_ERROR
        ? "--warningyesnocancel"
        : "--yesnocancel";
    default:
      return NULL;
  }
}

static int32_t moonbit_dialog_show_linux_kdialog(
  int32_t level,
  int32_t buttons,
  const char* title_utf8,
  const char* message_utf8
) {
  const char* command = moonbit_dialog_kdialog_command(buttons, level);
  if (command == NULL) {
    return DIALOG_STATUS_BACKEND_UNAVAILABLE;
  }

  const char* argv[] = {
    "kdialog",
    command,
    message_utf8,
    "--title",
    title_utf8,
    NULL
  };
  int status = moonbit_dialog_run_program(argv);
  if (status == DIALOG_HELPER_MISSING) {
    return DIALOG_STATUS_BACKEND_UNAVAILABLE;
  }

  if (buttons == DIALOG_BUTTONS_OK) {
    if (status == 0 || status == 1 || status == 255) {
      return moonbit_dialog_encode_success(
        DIALOG_BACKEND_LINUX_KDIALOG,
        DIALOG_RESPONSE_OK
      );
    }
  } else if (buttons == DIALOG_BUTTONS_YES_NO) {
    if (status == 0) {
      return moonbit_dialog_encode_success(
        DIALOG_BACKEND_LINUX_KDIALOG,
        DIALOG_RESPONSE_YES
      );
    }
    if (status == 1 || status == 255) {
      return moonbit_dialog_encode_success(
        DIALOG_BACKEND_LINUX_KDIALOG,
        DIALOG_RESPONSE_NO
      );
    }
  } else if (buttons == DIALOG_BUTTONS_YES_NO_CANCEL) {
    if (status == 0) {
      return moonbit_dialog_encode_success(
        DIALOG_BACKEND_LINUX_KDIALOG,
        DIALOG_RESPONSE_YES
      );
    }
    if (status == 1) {
      return moonbit_dialog_encode_success(
        DIALOG_BACKEND_LINUX_KDIALOG,
        DIALOG_RESPONSE_NO
      );
    }
    if (status == 2 || status == 255) {
      return moonbit_dialog_encode_success(
        DIALOG_BACKEND_LINUX_KDIALOG,
        DIALOG_RESPONSE_CANCEL
      );
    }
  }

  return moonbit_dialog_encode_failure(DIALOG_BACKEND_LINUX_KDIALOG, status);
}

static int32_t moonbit_dialog_show_linux_xmessage(
  int32_t buttons,
  const char* title_utf8,
  const char* message_utf8,
  const char* accept_label,
  const char* reject_label,
  const char* cancel_label
) {
  const char* first = moonbit_dialog_accept_label(buttons, accept_label);
  const char* second =
    buttons == DIALOG_BUTTONS_OK_CANCEL
      ? moonbit_dialog_cancel_label(buttons, cancel_label)
      : moonbit_dialog_reject_label(buttons, reject_label);
  const char* third = moonbit_dialog_cancel_label(buttons, cancel_label);

  if (
    !moonbit_dialog_xmessage_label_safe(first) ||
    !moonbit_dialog_xmessage_label_safe(second) ||
    !moonbit_dialog_xmessage_label_safe(third)
  ) {
    return DIALOG_STATUS_BACKEND_UNAVAILABLE;
  }

  char button_spec[256];
  if (buttons == DIALOG_BUTTONS_OK) {
    snprintf(
      button_spec,
      sizeof(button_spec),
      "%s:%d",
      first,
      DIALOG_XMESSAGE_BUTTON_BASE
    );
  } else if (buttons == DIALOG_BUTTONS_YES_NO_CANCEL) {
    snprintf(
      button_spec,
      sizeof(button_spec),
      "%s:%d,%s:%d,%s:%d",
      first,
      DIALOG_XMESSAGE_BUTTON_BASE,
      second,
      DIALOG_XMESSAGE_BUTTON_BASE + 1,
      third,
      DIALOG_XMESSAGE_BUTTON_BASE + 2
    );
  } else {
    snprintf(
      button_spec,
      sizeof(button_spec),
      "%s:%d,%s:%d",
      first,
      DIALOG_XMESSAGE_BUTTON_BASE,
      second,
      DIALOG_XMESSAGE_BUTTON_BASE + 1
    );
  }

  const char* argv[] = {
    "xmessage",
    "-center",
    "-title",
    title_utf8,
    "-buttons",
    button_spec,
    message_utf8,
    NULL
  };
  int status = moonbit_dialog_run_program(argv);
  if (status == DIALOG_HELPER_MISSING) {
    return DIALOG_STATUS_BACKEND_UNAVAILABLE;
  }

  if (status == DIALOG_XMESSAGE_BUTTON_BASE) {
    return moonbit_dialog_encode_success(
      DIALOG_BACKEND_LINUX_XMESSAGE,
      moonbit_dialog_response_from_index(buttons, 0)
    );
  }
  if (status == DIALOG_XMESSAGE_BUTTON_BASE + 1) {
    return moonbit_dialog_encode_success(
      DIALOG_BACKEND_LINUX_XMESSAGE,
      moonbit_dialog_response_from_index(buttons, 1)
    );
  }
  if (status == DIALOG_XMESSAGE_BUTTON_BASE + 2) {
    return moonbit_dialog_encode_success(
      DIALOG_BACKEND_LINUX_XMESSAGE,
      moonbit_dialog_response_from_index(buttons, 2)
    );
  }
  if (status == 1) {
    return moonbit_dialog_encode_success(
      DIALOG_BACKEND_LINUX_XMESSAGE,
      moonbit_dialog_response_for_close(buttons)
    );
  }

  return moonbit_dialog_encode_failure(DIALOG_BACKEND_LINUX_XMESSAGE, status);
}

static int32_t moonbit_dialog_show_linux(
  int32_t level,
  int32_t buttons,
  const char* title_utf8,
  const char* message_utf8,
  const char* accept_label,
  const char* reject_label,
  const char* cancel_label
) {
  int custom_labels = moonbit_dialog_has_custom_labels(
    accept_label,
    reject_label,
    cancel_label
  );
  int32_t result = DIALOG_STATUS_BACKEND_UNAVAILABLE;

  if (buttons == DIALOG_BUTTONS_OK) {
    result = moonbit_dialog_show_linux_zenity(
      level,
      buttons,
      title_utf8,
      message_utf8,
      accept_label,
      reject_label,
      cancel_label
    );
    if (result != DIALOG_STATUS_BACKEND_UNAVAILABLE) {
      return result;
    }

    if (!custom_labels) {
      result = moonbit_dialog_show_linux_kdialog(
        level,
        buttons,
        title_utf8,
        message_utf8
      );
      if (result != DIALOG_STATUS_BACKEND_UNAVAILABLE) {
        return result;
      }
    }

    return moonbit_dialog_show_linux_xmessage(
      buttons,
      title_utf8,
      message_utf8,
      accept_label,
      reject_label,
      cancel_label
    );
  }

  if (buttons == DIALOG_BUTTONS_OK_CANCEL) {
    result = moonbit_dialog_show_linux_zenity(
      level,
      buttons,
      title_utf8,
      message_utf8,
      accept_label,
      reject_label,
      cancel_label
    );
    if (result != DIALOG_STATUS_BACKEND_UNAVAILABLE) {
      return result;
    }

    return moonbit_dialog_show_linux_xmessage(
      buttons,
      title_utf8,
      message_utf8,
      accept_label,
      reject_label,
      cancel_label
    );
  }

  if (buttons == DIALOG_BUTTONS_YES_NO) {
    result = moonbit_dialog_show_linux_zenity(
      level,
      buttons,
      title_utf8,
      message_utf8,
      accept_label,
      reject_label,
      cancel_label
    );
    if (result != DIALOG_STATUS_BACKEND_UNAVAILABLE) {
      return result;
    }

    if (!custom_labels) {
      result = moonbit_dialog_show_linux_kdialog(
        level,
        buttons,
        title_utf8,
        message_utf8
      );
      if (result != DIALOG_STATUS_BACKEND_UNAVAILABLE) {
        return result;
      }
    }

    return moonbit_dialog_show_linux_xmessage(
      buttons,
      title_utf8,
      message_utf8,
      accept_label,
      reject_label,
      cancel_label
    );
  }

  if (buttons == DIALOG_BUTTONS_YES_NO_CANCEL) {
    if (!custom_labels) {
      result = moonbit_dialog_show_linux_kdialog(
        level,
        buttons,
        title_utf8,
        message_utf8
      );
      if (result != DIALOG_STATUS_BACKEND_UNAVAILABLE) {
        return result;
      }
    }

    result = moonbit_dialog_show_linux_zenity(
      level,
      buttons,
      title_utf8,
      message_utf8,
      accept_label,
      reject_label,
      cancel_label
    );
    if (result != DIALOG_STATUS_BACKEND_UNAVAILABLE) {
      return result;
    }

    return moonbit_dialog_show_linux_xmessage(
      buttons,
      title_utf8,
      message_utf8,
      accept_label,
      reject_label,
      cancel_label
    );
  }

  return DIALOG_STATUS_BACKEND_UNAVAILABLE;
}
#endif

MOONBIT_FFI_EXPORT
int32_t moonbit_dialog_show_dialog(
  int32_t level,
  int32_t buttons,
  moonbit_bytes_t title,
  moonbit_bytes_t message,
  moonbit_bytes_t accept_label,
  moonbit_bytes_t reject_label,
  moonbit_bytes_t cancel_label
) {
#if defined(_WIN32)
  return moonbit_dialog_show_windows(
    level,
    buttons,
    (const char*)title,
    (const char*)message,
    (const char*)accept_label,
    (const char*)reject_label,
    (const char*)cancel_label
  );
#elif defined(__APPLE__)
  return moonbit_dialog_show_macos(
    level,
    buttons,
    (const char*)title,
    (const char*)message,
    (const char*)accept_label,
    (const char*)reject_label,
    (const char*)cancel_label
  );
#elif defined(__linux__)
  return moonbit_dialog_show_linux(
    level,
    buttons,
    (const char*)title,
    (const char*)message,
    (const char*)accept_label,
    (const char*)reject_label,
    (const char*)cancel_label
  );
#else
  (void)level;
  (void)buttons;
  (void)title;
  (void)message;
  (void)accept_label;
  (void)reject_label;
  (void)cancel_label;
  return DIALOG_STATUS_UNSUPPORTED_PLATFORM;
#endif
}
