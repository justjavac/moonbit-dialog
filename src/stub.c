#include <moonbit.h>
#include <stdint.h>
#include <stdlib.h>

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
#endif

#define DIALOG_BACKEND_WINDOWS_WIN32 0
#define DIALOG_BACKEND_MACOS_CORE_FOUNDATION 1
#define DIALOG_BACKEND_LINUX_ZENITY 2
#define DIALOG_BACKEND_LINUX_KDIALOG 3
#define DIALOG_BACKEND_LINUX_XMESSAGE 4

#define DIALOG_STATUS_BACKEND_UNAVAILABLE -1
#define DIALOG_STATUS_UNSUPPORTED_PLATFORM -2

#define DIALOG_FAILURE_BASE 1000000
#define DIALOG_FAILURE_STRIDE 100000

static int32_t moonbit_dialog_encode_failure(int32_t backend, int32_t detail) {
  if (detail < 0) {
    detail = -detail;
  }
  if (detail >= DIALOG_FAILURE_STRIDE) {
    detail = DIALOG_FAILURE_STRIDE - 1;
  }
  return -(DIALOG_FAILURE_BASE + backend * DIALOG_FAILURE_STRIDE + detail);
}

MOONBIT_FFI_EXPORT
int32_t moonbit_dialog_platform_kind(void) {
#if defined(_WIN32)
  return 0;
#elif defined(__APPLE__)
  return 1;
#elif defined(__linux__)
  return 2;
#else
  return 3;
#endif
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

static int32_t moonbit_dialog_show_windows(
  const char* title_utf8,
  const char* message_utf8
) {
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
    MB_OK | MB_ICONINFORMATION | MB_SETFOREGROUND
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

  return DIALOG_BACKEND_WINDOWS_WIN32;
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
typedef SInt32 (*moonbit_dialog_cf_user_notification_display_notice_fn)(
  CFTimeInterval,
  CFOptionFlags,
  CFURLRef,
  CFURLRef,
  CFURLRef,
  CFStringRef,
  CFStringRef,
  CFStringRef
);
typedef void (*moonbit_dialog_cf_release_fn)(CFTypeRef);

#define DIALOG_CF_STRING_ENCODING_UTF8 0x08000100U

static void moonbit_dialog_cf_release_if_needed(
  moonbit_dialog_cf_release_fn release_fn,
  CFTypeRef value
) {
  if (value != NULL) {
    release_fn(value);
  }
}

static int32_t moonbit_dialog_show_macos(
  const char* title_utf8,
  const char* message_utf8
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
  moonbit_dialog_cf_user_notification_display_notice_fn display_notice =
    (moonbit_dialog_cf_user_notification_display_notice_fn)dlsym(
      core_foundation,
      "CFUserNotificationDisplayNotice"
    );
  moonbit_dialog_cf_release_fn cf_release =
    (moonbit_dialog_cf_release_fn)dlsym(core_foundation, "CFRelease");

  if (
    cf_string_create == NULL ||
    display_notice == NULL ||
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
  CFStringRef button = cf_string_create(
    NULL,
    "OK",
    DIALOG_CF_STRING_ENCODING_UTF8
  );

  if (title == NULL || message == NULL || button == NULL) {
    moonbit_dialog_cf_release_if_needed(cf_release, title);
    moonbit_dialog_cf_release_if_needed(cf_release, message);
    moonbit_dialog_cf_release_if_needed(cf_release, button);
    dlclose(core_foundation);
    return moonbit_dialog_encode_failure(
      DIALOG_BACKEND_MACOS_CORE_FOUNDATION,
      3
    );
  }

  SInt32 status = display_notice(
    0.0,
    0,
    NULL,
    NULL,
    NULL,
    title,
    message,
    button
  );
  moonbit_dialog_cf_release_if_needed(cf_release, button);
  moonbit_dialog_cf_release_if_needed(cf_release, message);
  moonbit_dialog_cf_release_if_needed(cf_release, title);
  dlclose(core_foundation);

  if (status != 0) {
    return moonbit_dialog_encode_failure(
      DIALOG_BACKEND_MACOS_CORE_FOUNDATION,
      status
    );
  }

  return DIALOG_BACKEND_MACOS_CORE_FOUNDATION;
}
#endif

#if defined(__linux__)
extern char** environ;

#define DIALOG_HELPER_MISSING -32768

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

static int32_t moonbit_dialog_linux_result(
  int32_t backend,
  int status
) {
  if (status == DIALOG_HELPER_MISSING) {
    return DIALOG_STATUS_BACKEND_UNAVAILABLE;
  }
  if (status == 0) {
    return backend;
  }
  return moonbit_dialog_encode_failure(backend, status);
}

static int32_t moonbit_dialog_show_linux(
  const char* title_utf8,
  const char* message_utf8
) {
  const char* zenity_args[] = {
    "zenity",
    "--info",
    "--no-wrap",
    "--title",
    title_utf8,
    "--text",
    message_utf8,
    NULL
  };
  int status = moonbit_dialog_run_program(zenity_args);
  if (status != DIALOG_HELPER_MISSING) {
    return moonbit_dialog_linux_result(DIALOG_BACKEND_LINUX_ZENITY, status);
  }

  const char* kdialog_args[] = {
    "kdialog",
    "--title",
    title_utf8,
    "--msgbox",
    message_utf8,
    NULL
  };
  status = moonbit_dialog_run_program(kdialog_args);
  if (status != DIALOG_HELPER_MISSING) {
    return moonbit_dialog_linux_result(DIALOG_BACKEND_LINUX_KDIALOG, status);
  }

  const char* xmessage_args[] = {
    "xmessage",
    "-center",
    "-title",
    title_utf8,
    message_utf8,
    NULL
  };
  status = moonbit_dialog_run_program(xmessage_args);
  if (status != DIALOG_HELPER_MISSING) {
    return moonbit_dialog_linux_result(DIALOG_BACKEND_LINUX_XMESSAGE, status);
  }

  return DIALOG_STATUS_BACKEND_UNAVAILABLE;
}
#endif

MOONBIT_FFI_EXPORT
int32_t moonbit_dialog_show_message(
  moonbit_bytes_t title,
  moonbit_bytes_t message
) {
#if defined(_WIN32)
  return moonbit_dialog_show_windows(
    (const char*)title,
    (const char*)message
  );
#elif defined(__APPLE__)
  return moonbit_dialog_show_macos(
    (const char*)title,
    (const char*)message
  );
#elif defined(__linux__)
  return moonbit_dialog_show_linux(
    (const char*)title,
    (const char*)message
  );
#else
  (void)title;
  (void)message;
  return DIALOG_STATUS_UNSUPPORTED_PLATFORM;
#endif
}
