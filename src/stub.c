#include <moonbit.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>
#include <wchar.h>
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#elif defined(__APPLE__)
#include <dlfcn.h>
#include <errno.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
#elif defined(__linux__)
#include <errno.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#define DIALOG_BACKEND_WINDOWS_WIN32 0
#define DIALOG_BACKEND_MACOS_CORE_FOUNDATION 1
#define DIALOG_BACKEND_MACOS_APPLE_SCRIPT 2
#define DIALOG_BACKEND_LINUX_ZENITY 3
#define DIALOG_BACKEND_LINUX_KDIALOG 4
#define DIALOG_BACKEND_LINUX_XMESSAGE 5

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

#define DIALOG_HELPER_MISSING -32768
#define DIALOG_APPLESCRIPT_CANCELLED "__MOONBIT_DIALOG_CANCELLED__"

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

static int moonbit_dialog_has_text(const char* text) {
  return text != NULL && text[0] != '\0';
}

static int moonbit_dialog_has_label(const char* text) {
  return moonbit_dialog_has_text(text);
}

static void moonbit_dialog_clear_output(
  moonbit_bytes_t output,
  int32_t output_len
) {
  if (output_len > 0) {
    output[0] = 0;
  }
}

static int moonbit_dialog_copy_utf8_to_output(
  const char* text,
  moonbit_bytes_t output,
  int32_t output_len
) {
  moonbit_dialog_clear_output(output, output_len);
  if (output_len <= 0) {
    return 0;
  }
  if (text == NULL) {
    return 1;
  }

  size_t text_len = strlen(text);
  if ((size_t)output_len <= text_len) {
    return 0;
  }

  memcpy(output, text, text_len + 1);
  return 1;
}

static size_t moonbit_dialog_serialized_component_length(const char* text) {
  size_t total = 0;
  for (const char* cursor = text; *cursor != '\0'; cursor++) {
    switch (*cursor) {
      case '\\':
      case '\n':
      case '\r':
      case '\t':
        total += 2;
        break;
      default:
        total += 1;
        break;
    }
  }
  return total;
}

static size_t moonbit_dialog_write_serialized_component(
  char* output,
  const char* text
) {
  size_t written = 0;
  for (const char* cursor = text; *cursor != '\0'; cursor++) {
    switch (*cursor) {
      case '\\':
        output[written++] = '\\';
        output[written++] = '\\';
        break;
      case '\n':
        output[written++] = '\\';
        output[written++] = 'n';
        break;
      case '\r':
        output[written++] = '\\';
        output[written++] = 'r';
        break;
      case '\t':
        output[written++] = '\\';
        output[written++] = 't';
        break;
      default:
        output[written++] = *cursor;
        break;
    }
  }
  return written;
}

static int moonbit_dialog_copy_utf8_paths_to_output(
  char* const* paths,
  int32_t path_count,
  moonbit_bytes_t output,
  int32_t output_len
) {
  moonbit_dialog_clear_output(output, output_len);
  if (output_len <= 0) {
    return 0;
  }

  size_t total = 1;
  for (int32_t i = 0; i < path_count; i++) {
    if (paths[i] == NULL) {
      continue;
    }
    total += moonbit_dialog_serialized_component_length(paths[i]);
    if (i + 1 < path_count) {
      total += 1;
    }
  }
  if ((size_t)output_len < total) {
    return 0;
  }

  size_t offset = 0;
  for (int32_t i = 0; i < path_count; i++) {
    if (paths[i] == NULL) {
      continue;
    }
    offset += moonbit_dialog_write_serialized_component(
      (char*)output + offset,
      paths[i]
    );
    if (i + 1 < path_count) {
      output[offset++] = '\n';
    }
  }
  output[offset] = '\0';
  return 1;
}

static int moonbit_dialog_copy_line_separated_paths_to_output(
  char* input,
  moonbit_bytes_t output,
  int32_t output_len
) {
  if (!moonbit_dialog_has_text(input)) {
    moonbit_dialog_clear_output(output, output_len);
    return 1;
  }

  int32_t count = 1;
  for (char* cursor = input; *cursor != '\0'; cursor++) {
    if (*cursor == '\n') {
      count++;
    }
  }

  char** paths = (char**)calloc((size_t)count, sizeof(char*));
  if (paths == NULL) {
    return 0;
  }

  int32_t index = 0;
  paths[index++] = input;
  for (char* cursor = input; *cursor != '\0'; cursor++) {
    if (*cursor == '\n') {
      *cursor = '\0';
      if (cursor > input && cursor[-1] == '\r') {
        cursor[-1] = '\0';
      }
      if (index < count) {
        paths[index++] = cursor + 1;
      }
    }
  }

  int ok = moonbit_dialog_copy_utf8_paths_to_output(paths, index, output, output_len);
  free(paths);
  return ok;
}

static char* moonbit_dialog_dup_text(const char* text) {
  size_t len = strlen(text);
  char* result = (char*)malloc(len + 1);
  if (result == NULL) {
    return NULL;
  }
  memcpy(result, text, len + 1);
  return result;
}

static char* moonbit_dialog_make_start_path(
  const char* directory,
  const char* file_name,
  int trailing_separator_for_directory
) {
  int has_directory = moonbit_dialog_has_text(directory);
  int has_file_name = moonbit_dialog_has_text(file_name);
  if (!has_directory && !has_file_name) {
    return NULL;
  }

  if (!has_directory) {
    return moonbit_dialog_dup_text(file_name);
  }

  size_t directory_len = strlen(directory);
  size_t file_name_len = has_file_name ? strlen(file_name) : 0;
  int needs_separator =
    has_file_name &&
    directory[directory_len - 1] != '/' &&
    directory[directory_len - 1] != '\\';
  int needs_trailing_separator =
    !has_file_name &&
    trailing_separator_for_directory &&
    directory[directory_len - 1] != '/' &&
    directory[directory_len - 1] != '\\';
  size_t total =
    directory_len + (size_t)needs_separator + (size_t)needs_trailing_separator +
    file_name_len + 1;
  char* result = (char*)malloc(total);
  if (result == NULL) {
    return NULL;
  }

  memcpy(result, directory, directory_len);
  size_t offset = directory_len;
  if (needs_separator || needs_trailing_separator) {
    result[offset++] = '/';
  }
  if (has_file_name) {
    memcpy(result + offset, file_name, file_name_len);
    offset += file_name_len;
  }
  result[offset] = '\0';
  return result;
}

static const char* moonbit_dialog_pick_label(
  const char* custom,
  const char* fallback
) {
  return moonbit_dialog_has_label(custom) ? custom : fallback;
}

typedef struct {
  char* name;
  char** patterns;
  int32_t pattern_count;
} moonbit_dialog_file_filter;

typedef struct {
  moonbit_dialog_file_filter* items;
  int32_t count;
} moonbit_dialog_file_filters;

static void moonbit_dialog_free_string_array(char** items, int32_t count);

static void moonbit_dialog_free_file_filter(
  moonbit_dialog_file_filter* filter
) {
  if (filter == NULL) {
    return;
  }

  free(filter->name);
  if (filter->patterns != NULL) {
    for (int32_t i = 0; i < filter->pattern_count; i++) {
      free(filter->patterns[i]);
    }
    free(filter->patterns);
  }

  filter->name = NULL;
  filter->patterns = NULL;
  filter->pattern_count = 0;
}

static void moonbit_dialog_free_file_filters(
  moonbit_dialog_file_filters* filters
) {
  if (filters == NULL) {
    return;
  }

  if (filters->items != NULL) {
    for (int32_t i = 0; i < filters->count; i++) {
      moonbit_dialog_free_file_filter(&filters->items[i]);
    }
    free(filters->items);
  }

  filters->items = NULL;
  filters->count = 0;
}

static char moonbit_dialog_unescape_filter_char(char ch) {
  switch (ch) {
    case 'n':
      return '\n';
    case 'r':
      return '\r';
    case 't':
      return '\t';
    default:
      return ch;
  }
}

static char* moonbit_dialog_parse_filter_token(
  const char** cursor,
  char* delimiter_out
) {
  const char* text = *cursor;
  size_t max_len = strlen(text);
  char* token = (char*)malloc(max_len + 1);
  if (token == NULL) {
    if (delimiter_out != NULL) {
      *delimiter_out = '\0';
    }
    return NULL;
  }

  size_t length = 0;
  while (*text != '\0' && *text != '\t' && *text != '\n') {
    if (*text == '\\' && text[1] != '\0') {
      text++;
      token[length++] = moonbit_dialog_unescape_filter_char(*text);
      text++;
      continue;
    }
    token[length++] = *text++;
  }

  token[length] = '\0';
  if (delimiter_out != NULL) {
    *delimiter_out = *text;
  }
  if (*text != '\0') {
    text++;
  }
  *cursor = text;
  return token;
}

static int moonbit_dialog_push_pattern(
  moonbit_dialog_file_filter* filter,
  char* pattern
) {
  char** next = (char**)realloc(
    filter->patterns,
    (size_t)(filter->pattern_count + 1) * sizeof(char*)
  );
  if (next == NULL) {
    free(pattern);
    return 0;
  }

  filter->patterns = next;
  filter->patterns[filter->pattern_count++] = pattern;
  return 1;
}

static int moonbit_dialog_push_file_filter(
  moonbit_dialog_file_filters* filters,
  moonbit_dialog_file_filter* filter
) {
  moonbit_dialog_file_filter* next = (moonbit_dialog_file_filter*)realloc(
    filters->items,
    (size_t)(filters->count + 1) * sizeof(moonbit_dialog_file_filter)
  );
  if (next == NULL) {
    return 0;
  }

  filters->items = next;
  filters->items[filters->count++] = *filter;
  filter->name = NULL;
  filter->patterns = NULL;
  filter->pattern_count = 0;
  return 1;
}

static int moonbit_dialog_parse_file_filters(
  const char* serialized_filters,
  moonbit_dialog_file_filters* filters_out
) {
  filters_out->items = NULL;
  filters_out->count = 0;
  if (!moonbit_dialog_has_text(serialized_filters)) {
    return 1;
  }

  const char* cursor = serialized_filters;
  while (*cursor != '\0') {
    moonbit_dialog_file_filter filter;
    memset(&filter, 0, sizeof(filter));

    char delimiter = '\0';
    filter.name = moonbit_dialog_parse_filter_token(&cursor, &delimiter);
    if (filter.name == NULL) {
      moonbit_dialog_free_file_filters(filters_out);
      return 0;
    }

    if (delimiter != '\t') {
      moonbit_dialog_free_file_filter(&filter);
      if (delimiter == '\0') {
        break;
      }
      continue;
    }

    while (1) {
      char pattern_delimiter = '\0';
      char* pattern = moonbit_dialog_parse_filter_token(
        &cursor,
        &pattern_delimiter
      );
      if (pattern == NULL) {
        moonbit_dialog_free_file_filter(&filter);
        moonbit_dialog_free_file_filters(filters_out);
        return 0;
      }

      if (moonbit_dialog_has_text(pattern)) {
        if (!moonbit_dialog_push_pattern(&filter, pattern)) {
          moonbit_dialog_free_file_filter(&filter);
          moonbit_dialog_free_file_filters(filters_out);
          return 0;
        }
      } else {
        free(pattern);
      }

      delimiter = pattern_delimiter;
      if (delimiter != '\t') {
        break;
      }
    }

    if (filter.pattern_count > 0) {
      if (!moonbit_dialog_push_file_filter(filters_out, &filter)) {
        moonbit_dialog_free_file_filter(&filter);
        moonbit_dialog_free_file_filters(filters_out);
        return 0;
      }
    }
    moonbit_dialog_free_file_filter(&filter);

    if (delimiter == '\0') {
      break;
    }
  }

  return 1;
}

static char* moonbit_dialog_join_filter_patterns(
  const moonbit_dialog_file_filter* filter,
  const char* separator
) {
  size_t separator_len = strlen(separator);
  size_t total = 1;
  int seen = 0;

  for (int32_t i = 0; i < filter->pattern_count; i++) {
    if (!moonbit_dialog_has_text(filter->patterns[i])) {
      continue;
    }
    total += strlen(filter->patterns[i]);
    if (seen) {
      total += separator_len;
    }
    seen = 1;
  }

  if (!seen) {
    return NULL;
  }

  char* result = (char*)malloc(total);
  if (result == NULL) {
    return NULL;
  }

  size_t offset = 0;
  seen = 0;
  for (int32_t i = 0; i < filter->pattern_count; i++) {
    const char* pattern = filter->patterns[i];
    if (!moonbit_dialog_has_text(pattern)) {
      continue;
    }
    if (seen) {
      memcpy(result + offset, separator, separator_len);
      offset += separator_len;
    }
    size_t pattern_len = strlen(pattern);
    memcpy(result + offset, pattern, pattern_len);
    offset += pattern_len;
    seen = 1;
  }
  result[offset] = '\0';
  return result;
}

static char* moonbit_dialog_join_all_filter_patterns(
  const moonbit_dialog_file_filters* filters,
  const char* separator
) {
  size_t separator_len = strlen(separator);
  size_t total = 1;
  int seen = 0;

  for (int32_t i = 0; i < filters->count; i++) {
    for (int32_t j = 0; j < filters->items[i].pattern_count; j++) {
      const char* pattern = filters->items[i].patterns[j];
      if (!moonbit_dialog_has_text(pattern)) {
        continue;
      }
      total += strlen(pattern);
      if (seen) {
        total += separator_len;
      }
      seen = 1;
    }
  }

  if (!seen) {
    return NULL;
  }

  char* result = (char*)malloc(total);
  if (result == NULL) {
    return NULL;
  }

  size_t offset = 0;
  seen = 0;
  for (int32_t i = 0; i < filters->count; i++) {
    for (int32_t j = 0; j < filters->items[i].pattern_count; j++) {
      const char* pattern = filters->items[i].patterns[j];
      if (!moonbit_dialog_has_text(pattern)) {
        continue;
      }
      if (seen) {
        memcpy(result + offset, separator, separator_len);
        offset += separator_len;
      }
      size_t pattern_len = strlen(pattern);
      memcpy(result + offset, pattern, pattern_len);
      offset += pattern_len;
      seen = 1;
    }
  }
  result[offset] = '\0';
  return result;
}

static char* moonbit_dialog_copy_default_extension(
  const char* extension_utf8,
  int with_dot
) {
  if (!moonbit_dialog_has_text(extension_utf8)) {
    return NULL;
  }

  while (*extension_utf8 == '.') {
    extension_utf8++;
  }
  if (*extension_utf8 == '\0') {
    return NULL;
  }

  size_t extension_len = strlen(extension_utf8);
  char* result = (char*)malloc(extension_len + (size_t)with_dot + 1);
  if (result == NULL) {
    return NULL;
  }

  size_t offset = 0;
  if (with_dot) {
    result[offset++] = '.';
  }
  memcpy(result + offset, extension_utf8, extension_len + 1);
  return result;
}

static int moonbit_dialog_path_has_extension(const char* path) {
  if (!moonbit_dialog_has_text(path)) {
    return 0;
  }

  const char* base = path;
  const char* slash = strrchr(path, '/');
  const char* backslash = strrchr(path, '\\');
  if (slash != NULL && slash + 1 > base) {
    base = slash + 1;
  }
  if (backslash != NULL && backslash + 1 > base) {
    base = backslash + 1;
  }

  int seen_non_dot = 0;
  for (const char* cursor = base; *cursor != '\0'; cursor++) {
    if (*cursor == '.') {
      if (seen_non_dot && cursor[1] != '\0') {
        return 1;
      }
      continue;
    }
    seen_non_dot = 1;
  }
  return 0;
}

static int moonbit_dialog_apply_default_extension(
  moonbit_bytes_t path_out,
  int32_t path_out_len,
  const char* default_extension_utf8
) {
  if (!moonbit_dialog_has_text((const char*)path_out)) {
    return 1;
  }
  if (moonbit_dialog_path_has_extension((const char*)path_out)) {
    return 1;
  }

  char* suffix = moonbit_dialog_copy_default_extension(
    default_extension_utf8,
    1
  );
  if (suffix == NULL) {
    return 1;
  }

  size_t path_len = strlen((const char*)path_out);
  size_t suffix_len = strlen(suffix);
  if (path_len + suffix_len >= (size_t)path_out_len) {
    free(suffix);
    return 0;
  }

  memcpy(path_out + path_len, suffix, suffix_len + 1);
  free(suffix);
  return 1;
}

static void moonbit_dialog_free_string_array(char** items, int32_t count) {
  if (items == NULL) {
    return;
  }
  for (int32_t i = 0; i < count; i++) {
    free(items[i]);
  }
  free(items);
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

#if defined(__APPLE__) || defined(__linux__)
extern char** environ;

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
  moonbit_bytes_t output,
  int32_t output_len
) {
  int pipe_fds[2] = {0, 0};
  moonbit_dialog_clear_output(output, output_len);

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
    while (filled + 1 < (size_t)output_len) {
      ssize_t count = read(
        pipe_fds[0],
        output + filled,
        (size_t)output_len - filled - 1
      );
      if (count <= 0) {
        break;
      }
      filled += (size_t)count;
    }
    output[filled] = '\0';
    moonbit_dialog_trim_newlines((char*)output);
  } else {
    char discard[64];
    while (read(pipe_fds[0], discard, sizeof(discard)) > 0) {
    }
  }

  close(pipe_fds[0]);
  return moonbit_dialog_wait_for_exit(pid);
}
#endif

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

static wchar_t* moonbit_dialog_build_windows_filter_wide(
  const moonbit_dialog_file_filters* filters
) {
  if (filters->count == 0) {
    return NULL;
  }

  wchar_t** names =
    (wchar_t**)calloc((size_t)filters->count, sizeof(wchar_t*));
  wchar_t** patterns =
    (wchar_t**)calloc((size_t)filters->count, sizeof(wchar_t*));
  if (names == NULL || patterns == NULL) {
    free(names);
    free(patterns);
    return NULL;
  }

  size_t total = 1;
  int ok = 1;
  for (int32_t i = 0; i < filters->count; i++) {
    char* pattern_text = moonbit_dialog_join_filter_patterns(
      &filters->items[i],
      ";"
    );
    const char* display_name = moonbit_dialog_has_text(filters->items[i].name)
      ? filters->items[i].name
      : pattern_text;
    if (display_name == NULL || pattern_text == NULL) {
      free(pattern_text);
      ok = 0;
      break;
    }

    names[i] = moonbit_dialog_utf8_to_wide(display_name);
    patterns[i] = moonbit_dialog_utf8_to_wide(pattern_text);
    free(pattern_text);
    if (names[i] == NULL || patterns[i] == NULL) {
      ok = 0;
      break;
    }

    total += wcslen(names[i]) + 1;
    total += wcslen(patterns[i]) + 1;
  }

  wchar_t* result = NULL;
  if (ok) {
    result = (wchar_t*)malloc(total * sizeof(wchar_t));
    if (result != NULL) {
      size_t offset = 0;
      for (int32_t i = 0; i < filters->count; i++) {
        size_t name_len = wcslen(names[i]);
        memcpy(result + offset, names[i], (name_len + 1) * sizeof(wchar_t));
        offset += name_len + 1;

        size_t pattern_len = wcslen(patterns[i]);
        memcpy(
          result + offset,
          patterns[i],
          (pattern_len + 1) * sizeof(wchar_t)
        );
        offset += pattern_len + 1;
      }
      result[offset] = L'\0';
    }
  }

  for (int32_t i = 0; i < filters->count; i++) {
    free(names[i]);
    free(patterns[i]);
  }
  free(names);
  free(patterns);
  return result;
}

static int moonbit_dialog_copy_wide_to_output(
  const wchar_t* text,
  moonbit_bytes_t output,
  int32_t output_len
) {
  moonbit_dialog_clear_output(output, output_len);
  if (output_len <= 0) {
    return 0;
  }

  int length = WideCharToMultiByte(CP_UTF8, 0, text, -1, NULL, 0, NULL, NULL);
  if (length <= 0 || length > output_len) {
    return 0;
  }

  return WideCharToMultiByte(
           CP_UTF8,
           0,
           text,
           -1,
           (char*)output,
           output_len,
           NULL,
           NULL
         ) != 0;
}

static char* moonbit_dialog_wide_to_utf8_owned(const wchar_t* text) {
  int length = WideCharToMultiByte(CP_UTF8, 0, text, -1, NULL, 0, NULL, NULL);
  if (length <= 0) {
    return NULL;
  }

  char* utf8 = (char*)malloc((size_t)length);
  if (utf8 == NULL) {
    return NULL;
  }

  if (
    WideCharToMultiByte(CP_UTF8, 0, text, -1, utf8, length, NULL, NULL) == 0
  ) {
    free(utf8);
    return NULL;
  }

  return utf8;
}

static wchar_t* moonbit_dialog_join_wide_path(
  const wchar_t* directory,
  const wchar_t* file_name
) {
  size_t directory_len = wcslen(directory);
  size_t file_len = wcslen(file_name);
  int needs_separator =
    directory_len > 0 &&
    directory[directory_len - 1] != L'/' &&
    directory[directory_len - 1] != L'\\';
  wchar_t* path = (wchar_t*)malloc(
    (directory_len + (size_t)needs_separator + file_len + 1) * sizeof(wchar_t)
  );
  if (path == NULL) {
    return NULL;
  }

  memcpy(path, directory, directory_len * sizeof(wchar_t));
  size_t offset = directory_len;
  if (needs_separator) {
    path[offset++] = L'\\';
  }
  memcpy(path + offset, file_name, (file_len + 1) * sizeof(wchar_t));
  return path;
}

static int moonbit_dialog_copy_wide_paths_to_output(
  wchar_t** paths,
  int32_t path_count,
  moonbit_bytes_t output,
  int32_t output_len
) {
  char** utf8_paths = (char**)calloc((size_t)path_count, sizeof(char*));
  if (utf8_paths == NULL) {
    return 0;
  }

  int ok = 1;
  for (int32_t i = 0; i < path_count; i++) {
    utf8_paths[i] = moonbit_dialog_wide_to_utf8_owned(paths[i]);
    if (utf8_paths[i] == NULL) {
      ok = 0;
      break;
    }
  }
  if (ok) {
    ok = moonbit_dialog_copy_utf8_paths_to_output(
      utf8_paths,
      path_count,
      output,
      output_len
    );
  }

  moonbit_dialog_free_string_array(utf8_paths, path_count);
  return ok;
}

static void moonbit_dialog_copy_wide_string(
  wchar_t* destination,
  size_t destination_len,
  const wchar_t* source
) {
  size_t i = 0;
  if (destination_len == 0) {
    return;
  }
  if (source != NULL) {
    while (i + 1 < destination_len && source[i] != L'\0') {
      destination[i] = source[i];
      i++;
    }
  }
  destination[i] = L'\0';
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

static int32_t moonbit_dialog_open_file_windows(
  const char* title_utf8,
  const char* directory_utf8,
  const char* filters_utf8,
  moonbit_bytes_t path_out,
  int32_t path_out_len
) {
  wchar_t* title = moonbit_dialog_utf8_to_wide(title_utf8);
  wchar_t* directory = NULL;
  wchar_t* filter_text = NULL;
  moonbit_dialog_file_filters filters;
  memset(&filters, 0, sizeof(filters));
  if (title == NULL) {
    DWORD error = GetLastError();
    return moonbit_dialog_encode_failure(
      DIALOG_BACKEND_WINDOWS_WIN32,
      error == 0 ? 11 : (int32_t)error
    );
  }
  if (moonbit_dialog_has_text(directory_utf8)) {
    directory = moonbit_dialog_utf8_to_wide(directory_utf8);
    if (directory == NULL) {
      DWORD error = GetLastError();
      free(title);
      return moonbit_dialog_encode_failure(
        DIALOG_BACKEND_WINDOWS_WIN32,
        error == 0 ? 12 : (int32_t)error
      );
    }
  }
  if (!moonbit_dialog_parse_file_filters(filters_utf8, &filters)) {
    free(directory);
    free(title);
    return moonbit_dialog_encode_failure(DIALOG_BACKEND_WINDOWS_WIN32, 14);
  }
  if (filters.count > 0) {
    filter_text = moonbit_dialog_build_windows_filter_wide(&filters);
    if (filter_text == NULL) {
      moonbit_dialog_free_file_filters(&filters);
      free(directory);
      free(title);
      return moonbit_dialog_encode_failure(DIALOG_BACKEND_WINDOWS_WIN32, 15);
    }
  }

  wchar_t file_buffer[32768];
  file_buffer[0] = L'\0';

  OPENFILENAMEW ofn;
  ZeroMemory(&ofn, sizeof(ofn));
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = NULL;
  ofn.lpstrFile = file_buffer;
  ofn.nMaxFile = (DWORD)(sizeof(file_buffer) / sizeof(file_buffer[0]));
  ofn.lpstrTitle = title;
  ofn.lpstrInitialDir = directory;
  ofn.lpstrFilter = filter_text;
  ofn.nFilterIndex = 1;
  ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

  BOOL result = GetOpenFileNameW(&ofn);
  DWORD detail = CommDlgExtendedError();
  free(filter_text);
  moonbit_dialog_free_file_filters(&filters);
  free(directory);
  free(title);

  if (!result) {
    if (detail == 0) {
      moonbit_dialog_clear_output(path_out, path_out_len);
      return moonbit_dialog_encode_success(
        DIALOG_BACKEND_WINDOWS_WIN32,
        DIALOG_RESPONSE_CANCEL
      );
    }
    return moonbit_dialog_encode_failure(
      DIALOG_BACKEND_WINDOWS_WIN32,
      (int32_t)detail
    );
  }

  if (!moonbit_dialog_copy_wide_to_output(file_buffer, path_out, path_out_len)) {
    return moonbit_dialog_encode_failure(DIALOG_BACKEND_WINDOWS_WIN32, 13);
  }

  return moonbit_dialog_encode_success(
    DIALOG_BACKEND_WINDOWS_WIN32,
    DIALOG_RESPONSE_OK
  );
}

static int32_t moonbit_dialog_open_files_windows(
  const char* title_utf8,
  const char* directory_utf8,
  const char* filters_utf8,
  moonbit_bytes_t path_out,
  int32_t path_out_len
) {
  wchar_t* title = moonbit_dialog_utf8_to_wide(title_utf8);
  wchar_t* directory = NULL;
  wchar_t* filter_text = NULL;
  moonbit_dialog_file_filters filters;
  memset(&filters, 0, sizeof(filters));
  if (title == NULL) {
    DWORD error = GetLastError();
    return moonbit_dialog_encode_failure(
      DIALOG_BACKEND_WINDOWS_WIN32,
      error == 0 ? 16 : (int32_t)error
    );
  }
  if (moonbit_dialog_has_text(directory_utf8)) {
    directory = moonbit_dialog_utf8_to_wide(directory_utf8);
    if (directory == NULL) {
      DWORD error = GetLastError();
      free(title);
      return moonbit_dialog_encode_failure(
        DIALOG_BACKEND_WINDOWS_WIN32,
        error == 0 ? 17 : (int32_t)error
      );
    }
  }
  if (!moonbit_dialog_parse_file_filters(filters_utf8, &filters)) {
    free(directory);
    free(title);
    return moonbit_dialog_encode_failure(DIALOG_BACKEND_WINDOWS_WIN32, 18);
  }
  if (filters.count > 0) {
    filter_text = moonbit_dialog_build_windows_filter_wide(&filters);
    if (filter_text == NULL) {
      moonbit_dialog_free_file_filters(&filters);
      free(directory);
      free(title);
      return moonbit_dialog_encode_failure(DIALOG_BACKEND_WINDOWS_WIN32, 19);
    }
  }

  wchar_t file_buffer[32768];
  file_buffer[0] = L'\0';

  OPENFILENAMEW ofn;
  ZeroMemory(&ofn, sizeof(ofn));
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = NULL;
  ofn.lpstrFile = file_buffer;
  ofn.nMaxFile = (DWORD)(sizeof(file_buffer) / sizeof(file_buffer[0]));
  ofn.lpstrTitle = title;
  ofn.lpstrInitialDir = directory;
  ofn.lpstrFilter = filter_text;
  ofn.nFilterIndex = 1;
  ofn.Flags =
    OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_ALLOWMULTISELECT;

  BOOL result = GetOpenFileNameW(&ofn);
  DWORD detail = CommDlgExtendedError();
  free(filter_text);
  moonbit_dialog_free_file_filters(&filters);
  free(directory);
  free(title);

  if (!result) {
    if (detail == 0) {
      moonbit_dialog_clear_output(path_out, path_out_len);
      return moonbit_dialog_encode_success(
        DIALOG_BACKEND_WINDOWS_WIN32,
        DIALOG_RESPONSE_CANCEL
      );
    }
    return moonbit_dialog_encode_failure(
      DIALOG_BACKEND_WINDOWS_WIN32,
      (int32_t)detail
    );
  }

  const wchar_t* first = file_buffer;
  const wchar_t* second = first + wcslen(first) + 1;
  if (*second == L'\0') {
    wchar_t* single_paths[1];
    single_paths[0] = (wchar_t*)first;
    if (!moonbit_dialog_copy_wide_paths_to_output(single_paths, 1, path_out, path_out_len)) {
      return moonbit_dialog_encode_failure(DIALOG_BACKEND_WINDOWS_WIN32, 20);
    }
  } else {
    int32_t count = 0;
    const wchar_t* cursor = second;
    while (*cursor != L'\0') {
      count++;
      cursor += wcslen(cursor) + 1;
    }

    wchar_t** paths = (wchar_t**)calloc((size_t)count, sizeof(wchar_t*));
    if (paths == NULL) {
      return moonbit_dialog_encode_failure(DIALOG_BACKEND_WINDOWS_WIN32, 21);
    }

    int ok = 1;
    cursor = second;
    for (int32_t i = 0; i < count; i++) {
      paths[i] = moonbit_dialog_join_wide_path(first, cursor);
      if (paths[i] == NULL) {
        ok = 0;
        break;
      }
      cursor += wcslen(cursor) + 1;
    }
    if (ok) {
      ok = moonbit_dialog_copy_wide_paths_to_output(
        paths,
        count,
        path_out,
        path_out_len
      );
    }
    for (int32_t i = 0; i < count; i++) {
      free(paths[i]);
    }
    free(paths);

    if (!ok) {
      return moonbit_dialog_encode_failure(DIALOG_BACKEND_WINDOWS_WIN32, 22);
    }
  }

  return moonbit_dialog_encode_success(
    DIALOG_BACKEND_WINDOWS_WIN32,
    DIALOG_RESPONSE_OK
  );
}

static int32_t moonbit_dialog_save_file_windows(
  const char* title_utf8,
  const char* directory_utf8,
  const char* file_name_utf8,
  const char* filters_utf8,
  const char* default_extension_utf8,
  moonbit_bytes_t path_out,
  int32_t path_out_len
) {
  wchar_t* title = moonbit_dialog_utf8_to_wide(title_utf8);
  wchar_t* directory = NULL;
  wchar_t* file_name = NULL;
  wchar_t* filter_text = NULL;
  wchar_t* default_extension = NULL;
  moonbit_dialog_file_filters filters;
  memset(&filters, 0, sizeof(filters));
  if (title == NULL) {
    DWORD error = GetLastError();
    return moonbit_dialog_encode_failure(
      DIALOG_BACKEND_WINDOWS_WIN32,
      error == 0 ? 21 : (int32_t)error
    );
  }
  if (moonbit_dialog_has_text(directory_utf8)) {
    directory = moonbit_dialog_utf8_to_wide(directory_utf8);
    if (directory == NULL) {
      DWORD error = GetLastError();
      free(title);
      return moonbit_dialog_encode_failure(
        DIALOG_BACKEND_WINDOWS_WIN32,
        error == 0 ? 22 : (int32_t)error
      );
    }
  }
  if (moonbit_dialog_has_text(file_name_utf8)) {
    file_name = moonbit_dialog_utf8_to_wide(file_name_utf8);
    if (file_name == NULL) {
      DWORD error = GetLastError();
      free(directory);
      free(title);
      return moonbit_dialog_encode_failure(
        DIALOG_BACKEND_WINDOWS_WIN32,
        error == 0 ? 23 : (int32_t)error
      );
    }
  }
  if (!moonbit_dialog_parse_file_filters(filters_utf8, &filters)) {
    free(file_name);
    free(directory);
    free(title);
    return moonbit_dialog_encode_failure(DIALOG_BACKEND_WINDOWS_WIN32, 24);
  }
  if (filters.count > 0) {
    filter_text = moonbit_dialog_build_windows_filter_wide(&filters);
    if (filter_text == NULL) {
      moonbit_dialog_free_file_filters(&filters);
      free(file_name);
      free(directory);
      free(title);
      return moonbit_dialog_encode_failure(DIALOG_BACKEND_WINDOWS_WIN32, 25);
    }
  }
  {
    char* extension_no_dot = moonbit_dialog_copy_default_extension(
      default_extension_utf8,
      0
    );
    if (extension_no_dot != NULL) {
      default_extension = moonbit_dialog_utf8_to_wide(extension_no_dot);
      free(extension_no_dot);
      if (default_extension == NULL) {
        free(filter_text);
        moonbit_dialog_free_file_filters(&filters);
        free(file_name);
        free(directory);
        free(title);
        return moonbit_dialog_encode_failure(DIALOG_BACKEND_WINDOWS_WIN32, 26);
      }
    }
  }

  wchar_t file_buffer[32768];
  moonbit_dialog_copy_wide_string(
    file_buffer,
    sizeof(file_buffer) / sizeof(file_buffer[0]),
    file_name
  );

  OPENFILENAMEW ofn;
  ZeroMemory(&ofn, sizeof(ofn));
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = NULL;
  ofn.lpstrFile = file_buffer;
  ofn.nMaxFile = (DWORD)(sizeof(file_buffer) / sizeof(file_buffer[0]));
  ofn.lpstrTitle = title;
  ofn.lpstrInitialDir = directory;
  ofn.lpstrFilter = filter_text;
  ofn.nFilterIndex = 1;
  ofn.lpstrDefExt = default_extension;
  ofn.Flags = OFN_EXPLORER | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;

  BOOL result = GetSaveFileNameW(&ofn);
  DWORD detail = CommDlgExtendedError();
  free(default_extension);
  free(filter_text);
  moonbit_dialog_free_file_filters(&filters);
  free(file_name);
  free(directory);
  free(title);

  if (!result) {
    if (detail == 0) {
      moonbit_dialog_clear_output(path_out, path_out_len);
      return moonbit_dialog_encode_success(
        DIALOG_BACKEND_WINDOWS_WIN32,
        DIALOG_RESPONSE_CANCEL
      );
    }
    return moonbit_dialog_encode_failure(
      DIALOG_BACKEND_WINDOWS_WIN32,
      (int32_t)detail
    );
  }

  if (!moonbit_dialog_copy_wide_to_output(file_buffer, path_out, path_out_len)) {
    return moonbit_dialog_encode_failure(DIALOG_BACKEND_WINDOWS_WIN32, 27);
  }
  if (
    !moonbit_dialog_apply_default_extension(
      path_out,
      path_out_len,
      default_extension_utf8
    )
  ) {
    return moonbit_dialog_encode_failure(DIALOG_BACKEND_WINDOWS_WIN32, 28);
  }

  return moonbit_dialog_encode_success(
    DIALOG_BACKEND_WINDOWS_WIN32,
    DIALOG_RESPONSE_OK
  );
}

static int CALLBACK moonbit_dialog_browse_callback(
  HWND hwnd,
  UINT msg,
  LPARAM lparam,
  LPARAM data
) {
  if (msg == BFFM_INITIALIZED && data != 0) {
    SendMessageW(hwnd, BFFM_SETSELECTIONW, TRUE, data);
  }
  (void)lparam;
  return 0;
}

static int32_t moonbit_dialog_select_folder_windows(
  const char* title_utf8,
  const char* directory_utf8,
  moonbit_bytes_t path_out,
  int32_t path_out_len
) {
  wchar_t* title = moonbit_dialog_utf8_to_wide(title_utf8);
  wchar_t* directory = NULL;
  if (title == NULL) {
    DWORD error = GetLastError();
    return moonbit_dialog_encode_failure(
      DIALOG_BACKEND_WINDOWS_WIN32,
      error == 0 ? 31 : (int32_t)error
    );
  }
  if (moonbit_dialog_has_text(directory_utf8)) {
    directory = moonbit_dialog_utf8_to_wide(directory_utf8);
    if (directory == NULL) {
      DWORD error = GetLastError();
      free(title);
      return moonbit_dialog_encode_failure(
        DIALOG_BACKEND_WINDOWS_WIN32,
        error == 0 ? 32 : (int32_t)error
      );
    }
  }

  wchar_t display_name[MAX_PATH];
  display_name[0] = L'\0';
  HRESULT ole_status = OleInitialize(NULL);

  BROWSEINFOW browse_info;
  ZeroMemory(&browse_info, sizeof(browse_info));
  browse_info.hwndOwner = NULL;
  browse_info.pszDisplayName = display_name;
  browse_info.lpszTitle = title;
  browse_info.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI;
  browse_info.lpfn = moonbit_dialog_browse_callback;
  browse_info.lParam = (LPARAM)directory;

  PIDLIST_ABSOLUTE item_id = SHBrowseForFolderW(&browse_info);
  free(directory);
  free(title);

  if (item_id == NULL) {
    if (SUCCEEDED(ole_status)) {
      OleUninitialize();
    }
    moonbit_dialog_clear_output(path_out, path_out_len);
    return moonbit_dialog_encode_success(
      DIALOG_BACKEND_WINDOWS_WIN32,
      DIALOG_RESPONSE_CANCEL
    );
  }

  wchar_t selected_path[32768];
  BOOL ok = SHGetPathFromIDListW(item_id, selected_path);
  CoTaskMemFree(item_id);
  if (SUCCEEDED(ole_status)) {
    OleUninitialize();
  }

  if (!ok) {
    return moonbit_dialog_encode_failure(DIALOG_BACKEND_WINDOWS_WIN32, 33);
  }
  if (
    !moonbit_dialog_copy_wide_to_output(
      selected_path,
      path_out,
      path_out_len
    )
  ) {
    return moonbit_dialog_encode_failure(DIALOG_BACKEND_WINDOWS_WIN32, 34);
  }

  return moonbit_dialog_encode_success(
    DIALOG_BACKEND_WINDOWS_WIN32,
    DIALOG_RESPONSE_OK
  );
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

static int moonbit_dialog_run_osascript(
  const char* const script_lines[],
  int script_line_count,
  const char* const args[],
  int arg_count,
  moonbit_bytes_t output,
  int32_t output_len
) {
  int total_args = 1 + script_line_count * 2 + 1 + arg_count + 1;
  const char** argv = (const char**)malloc((size_t)total_args * sizeof(char*));
  if (argv == NULL) {
    return -12;
  }

  int index = 0;
  argv[index++] = "osascript";
  for (int i = 0; i < script_line_count; i++) {
    argv[index++] = "-e";
    argv[index++] = script_lines[i];
  }
  argv[index++] = "--";
  for (int i = 0; i < arg_count; i++) {
    argv[index++] = args[i];
  }
  argv[index] = NULL;

  int status = moonbit_dialog_run_program_capture_stdout(argv, output, output_len);
  free(argv);
  return status;
}

static int moonbit_dialog_pattern_is_match_all(const char* pattern) {
  return strcmp(pattern, "*") == 0 || strcmp(pattern, "*.*") == 0;
}

static char* moonbit_dialog_copy_applescript_type(const char* pattern) {
  if (!moonbit_dialog_has_text(pattern)) {
    return NULL;
  }
  if (moonbit_dialog_pattern_is_match_all(pattern)) {
    return NULL;
  }

  const char* cursor = pattern;
  while (*cursor == '*') {
    cursor++;
  }
  while (*cursor == '.') {
    cursor++;
  }
  if (*cursor == '\0') {
    return NULL;
  }

  for (const char* it = cursor; *it != '\0'; it++) {
    if (
      *it == '*' || *it == '?' || *it == '|' || *it == ';' ||
      *it == ':' || *it == '/' || *it == '\\' || *it == ' '
    ) {
      return NULL;
    }
  }

  return moonbit_dialog_dup_text(cursor);
}

static int moonbit_dialog_collect_applescript_types(
  const moonbit_dialog_file_filters* filters,
  char*** types_out,
  int32_t* count_out
) {
  *types_out = NULL;
  *count_out = 0;

  for (int32_t i = 0; i < filters->count; i++) {
    for (int32_t j = 0; j < filters->items[i].pattern_count; j++) {
      const char* pattern = filters->items[i].patterns[j];
      if (moonbit_dialog_pattern_is_match_all(pattern)) {
        moonbit_dialog_free_string_array(*types_out, *count_out);
        *types_out = NULL;
        *count_out = 0;
        return 1;
      }

      char* type_text = moonbit_dialog_copy_applescript_type(pattern);
      if (type_text == NULL) {
        continue;
      }

      char** next = (char**)realloc(
        *types_out,
        (size_t)(*count_out + 1) * sizeof(char*)
      );
      if (next == NULL) {
        free(type_text);
        moonbit_dialog_free_string_array(*types_out, *count_out);
        *types_out = NULL;
        *count_out = 0;
        return 0;
      }

      *types_out = next;
      (*types_out)[(*count_out)++] = type_text;
    }
  }

  return 1;
}

static int32_t moonbit_dialog_open_file_macos(
  const char* title_utf8,
  const char* directory_utf8,
  const char* filters_utf8,
  moonbit_bytes_t path_out,
  int32_t path_out_len
) {
  moonbit_dialog_file_filters filters;
  memset(&filters, 0, sizeof(filters));
  char** type_args = NULL;
  int32_t type_count = 0;
  if (!moonbit_dialog_parse_file_filters(filters_utf8, &filters)) {
    return moonbit_dialog_encode_failure(
      DIALOG_BACKEND_MACOS_APPLE_SCRIPT,
      11
    );
  }
  if (!moonbit_dialog_collect_applescript_types(&filters, &type_args, &type_count)) {
    moonbit_dialog_free_file_filters(&filters);
    return moonbit_dialog_encode_failure(
      DIALOG_BACKEND_MACOS_APPLE_SCRIPT,
      12
    );
  }

  const char* script[] = {
    "on run argv",
    "set promptText to item 1 of argv",
    "set directoryPath to item 2 of argv",
    "set typeList to {}",
    "if (count of argv) > 2 then",
    "repeat with i from 3 to (count of argv)",
    "set end of typeList to item i of argv",
    "end repeat",
    "end if",
    "try",
    "if directoryPath is \"\" then",
    "if (count of typeList) is 0 then",
    "return POSIX path of (choose file with prompt promptText)",
    "else",
    "return POSIX path of (choose file with prompt promptText of type typeList)",
    "end if",
    "else",
    "if (count of typeList) is 0 then",
    "return POSIX path of (choose file with prompt promptText default location (POSIX file directoryPath))",
    "else",
    "return POSIX path of (choose file with prompt promptText default location (POSIX file directoryPath) of type typeList)",
    "end if",
    "end if",
    "on error number -128",
    "return \"" DIALOG_APPLESCRIPT_CANCELLED "\"",
    "end try",
    "end run"
  };
  int arg_count = 2 + (int)type_count;
  const char** args = (const char**)malloc((size_t)arg_count * sizeof(char*));
  if (args == NULL) {
    moonbit_dialog_free_string_array(type_args, type_count);
    moonbit_dialog_free_file_filters(&filters);
    return moonbit_dialog_encode_failure(
      DIALOG_BACKEND_MACOS_APPLE_SCRIPT,
      13
    );
  }
  args[0] = title_utf8;
  args[1] = directory_utf8;
  for (int32_t i = 0; i < type_count; i++) {
    args[2 + i] = type_args[i];
  }
  int status = moonbit_dialog_run_osascript(
    script,
    (int)(sizeof(script) / sizeof(script[0])),
    args,
    arg_count,
    path_out,
    path_out_len
  );
  free(args);
  moonbit_dialog_free_string_array(type_args, type_count);
  moonbit_dialog_free_file_filters(&filters);

  if (status == DIALOG_HELPER_MISSING) {
    return moonbit_dialog_encode_failure(
      DIALOG_BACKEND_MACOS_APPLE_SCRIPT,
      14
    );
  }
  if (status != 0) {
    return moonbit_dialog_encode_failure(
      DIALOG_BACKEND_MACOS_APPLE_SCRIPT,
      status
    );
  }
  if (strcmp((char*)path_out, DIALOG_APPLESCRIPT_CANCELLED) == 0) {
    moonbit_dialog_clear_output(path_out, path_out_len);
    return moonbit_dialog_encode_success(
      DIALOG_BACKEND_MACOS_APPLE_SCRIPT,
      DIALOG_RESPONSE_CANCEL
    );
  }

  return moonbit_dialog_encode_success(
    DIALOG_BACKEND_MACOS_APPLE_SCRIPT,
    DIALOG_RESPONSE_OK
  );
}

static int32_t moonbit_dialog_open_files_macos(
  const char* title_utf8,
  const char* directory_utf8,
  const char* filters_utf8,
  moonbit_bytes_t path_out,
  int32_t path_out_len
) {
  moonbit_dialog_file_filters filters;
  memset(&filters, 0, sizeof(filters));
  char** type_args = NULL;
  int32_t type_count = 0;
  if (!moonbit_dialog_parse_file_filters(filters_utf8, &filters)) {
    return moonbit_dialog_encode_failure(
      DIALOG_BACKEND_MACOS_APPLE_SCRIPT,
      31
    );
  }
  if (
    !moonbit_dialog_collect_applescript_types(&filters, &type_args, &type_count)
  ) {
    moonbit_dialog_free_file_filters(&filters);
    return moonbit_dialog_encode_failure(
      DIALOG_BACKEND_MACOS_APPLE_SCRIPT,
      32
    );
  }

  const char* script[] = {
    "on run argv",
    "set promptText to item 1 of argv",
    "set directoryPath to item 2 of argv",
    "set typeList to {}",
    "if (count of argv) > 2 then",
    "repeat with i from 3 to (count of argv)",
    "set end of typeList to item i of argv",
    "end repeat",
    "end if",
    "try",
    "if directoryPath is \"\" then",
    "if (count of typeList) is 0 then",
    "set chosenItems to choose file with prompt promptText multiple selections allowed true",
    "else",
    "set chosenItems to choose file with prompt promptText of type typeList multiple selections allowed true",
    "end if",
    "else",
    "if (count of typeList) is 0 then",
    "set chosenItems to choose file with prompt promptText default location (POSIX file directoryPath) multiple selections allowed true",
    "else",
    "set chosenItems to choose file with prompt promptText default location (POSIX file directoryPath) of type typeList multiple selections allowed true",
    "end if",
    "end if",
    "set pathList to {}",
    "repeat with chosenItem in chosenItems",
    "set end of pathList to POSIX path of chosenItem",
    "end repeat",
    "set AppleScript's text item delimiters to linefeed",
    "return pathList as text",
    "on error number -128",
    "return \"" DIALOG_APPLESCRIPT_CANCELLED "\"",
    "end try",
    "end run"
  };

  int arg_count = 2 + (int)type_count;
  const char** args = (const char**)malloc((size_t)arg_count * sizeof(char*));
  char* raw_output = (char*)malloc((size_t)path_out_len);
  if (args == NULL || raw_output == NULL) {
    free(args);
    free(raw_output);
    moonbit_dialog_free_string_array(type_args, type_count);
    moonbit_dialog_free_file_filters(&filters);
    return moonbit_dialog_encode_failure(
      DIALOG_BACKEND_MACOS_APPLE_SCRIPT,
      33
    );
  }
  args[0] = title_utf8;
  args[1] = directory_utf8;
  for (int32_t i = 0; i < type_count; i++) {
    args[2 + i] = type_args[i];
  }

  int status = moonbit_dialog_run_osascript(
    script,
    (int)(sizeof(script) / sizeof(script[0])),
    args,
    arg_count,
    (moonbit_bytes_t)raw_output,
    path_out_len
  );
  free(args);
  moonbit_dialog_free_string_array(type_args, type_count);
  moonbit_dialog_free_file_filters(&filters);

  if (status == DIALOG_HELPER_MISSING) {
    free(raw_output);
    return moonbit_dialog_encode_failure(
      DIALOG_BACKEND_MACOS_APPLE_SCRIPT,
      34
    );
  }
  if (status != 0) {
    free(raw_output);
    return moonbit_dialog_encode_failure(
      DIALOG_BACKEND_MACOS_APPLE_SCRIPT,
      status
    );
  }
  if (strcmp(raw_output, DIALOG_APPLESCRIPT_CANCELLED) == 0) {
    free(raw_output);
    moonbit_dialog_clear_output(path_out, path_out_len);
    return moonbit_dialog_encode_success(
      DIALOG_BACKEND_MACOS_APPLE_SCRIPT,
      DIALOG_RESPONSE_CANCEL
    );
  }
  if (
    !moonbit_dialog_copy_line_separated_paths_to_output(
      raw_output,
      path_out,
      path_out_len
    )
  ) {
    free(raw_output);
    return moonbit_dialog_encode_failure(
      DIALOG_BACKEND_MACOS_APPLE_SCRIPT,
      35
    );
  }
  free(raw_output);

  return moonbit_dialog_encode_success(
    DIALOG_BACKEND_MACOS_APPLE_SCRIPT,
    DIALOG_RESPONSE_OK
  );
}

static int32_t moonbit_dialog_save_file_macos(
  const char* title_utf8,
  const char* directory_utf8,
  const char* file_name_utf8,
  const char* filters_utf8,
  const char* default_extension_utf8,
  moonbit_bytes_t path_out,
  int32_t path_out_len
) {
  (void)filters_utf8;
  const char* script[] = {
    "on run argv",
    "set promptText to item 1 of argv",
    "set directoryPath to item 2 of argv",
    "set defaultName to item 3 of argv",
    "try",
    "if directoryPath is \"\" then",
    "if defaultName is \"\" then",
    "return POSIX path of (choose file name with prompt promptText)",
    "else",
    "return POSIX path of (choose file name with prompt promptText default name defaultName)",
    "end if",
    "else",
    "if defaultName is \"\" then",
    "return POSIX path of (choose file name with prompt promptText default location (POSIX file directoryPath))",
    "else",
    "return POSIX path of (choose file name with prompt promptText default location (POSIX file directoryPath) default name defaultName)",
    "end if",
    "end if",
    "on error number -128",
    "return \"" DIALOG_APPLESCRIPT_CANCELLED "\"",
    "end try",
    "end run"
  };
  const char* args[] = { title_utf8, directory_utf8, file_name_utf8 };
  int status = moonbit_dialog_run_osascript(
    script,
    (int)(sizeof(script) / sizeof(script[0])),
    args,
    (int)(sizeof(args) / sizeof(args[0])),
    path_out,
    path_out_len
  );

  if (status == DIALOG_HELPER_MISSING) {
    return moonbit_dialog_encode_failure(
      DIALOG_BACKEND_MACOS_APPLE_SCRIPT,
      21
    );
  }
  if (status != 0) {
    return moonbit_dialog_encode_failure(
      DIALOG_BACKEND_MACOS_APPLE_SCRIPT,
      status
    );
  }
  if (strcmp((char*)path_out, DIALOG_APPLESCRIPT_CANCELLED) == 0) {
    moonbit_dialog_clear_output(path_out, path_out_len);
    return moonbit_dialog_encode_success(
      DIALOG_BACKEND_MACOS_APPLE_SCRIPT,
      DIALOG_RESPONSE_CANCEL
    );
  }
  if (
    !moonbit_dialog_apply_default_extension(
      path_out,
      path_out_len,
      default_extension_utf8
    )
  ) {
    return moonbit_dialog_encode_failure(
      DIALOG_BACKEND_MACOS_APPLE_SCRIPT,
      22
    );
  }

  return moonbit_dialog_encode_success(
    DIALOG_BACKEND_MACOS_APPLE_SCRIPT,
    DIALOG_RESPONSE_OK
  );
}

static int32_t moonbit_dialog_select_folder_macos(
  const char* title_utf8,
  const char* directory_utf8,
  moonbit_bytes_t path_out,
  int32_t path_out_len
) {
  const char* script[] = {
    "on run argv",
    "set promptText to item 1 of argv",
    "set directoryPath to item 2 of argv",
    "try",
    "if directoryPath is \"\" then",
    "return POSIX path of (choose folder with prompt promptText)",
    "else",
    "return POSIX path of (choose folder with prompt promptText default location (POSIX file directoryPath))",
    "end if",
    "on error number -128",
    "return \"" DIALOG_APPLESCRIPT_CANCELLED "\"",
    "end try",
    "end run"
  };
  const char* args[] = { title_utf8, directory_utf8 };
  int status = moonbit_dialog_run_osascript(
    script,
    (int)(sizeof(script) / sizeof(script[0])),
    args,
    (int)(sizeof(args) / sizeof(args[0])),
    path_out,
    path_out_len
  );

  if (status == DIALOG_HELPER_MISSING) {
    return moonbit_dialog_encode_failure(
      DIALOG_BACKEND_MACOS_APPLE_SCRIPT,
      13
    );
  }
  if (status != 0) {
    return moonbit_dialog_encode_failure(
      DIALOG_BACKEND_MACOS_APPLE_SCRIPT,
      status
    );
  }
  if (strcmp((char*)path_out, DIALOG_APPLESCRIPT_CANCELLED) == 0) {
    moonbit_dialog_clear_output(path_out, path_out_len);
    return moonbit_dialog_encode_success(
      DIALOG_BACKEND_MACOS_APPLE_SCRIPT,
      DIALOG_RESPONSE_CANCEL
    );
  }

  return moonbit_dialog_encode_success(
    DIALOG_BACKEND_MACOS_APPLE_SCRIPT,
    DIALOG_RESPONSE_OK
  );
}
#endif

#if defined(__linux__)
#define DIALOG_XMESSAGE_BUTTON_BASE 20

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
          (moonbit_bytes_t)output,
          (int32_t)sizeof(output)
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

static char* moonbit_dialog_make_zenity_filter_arg(
  const moonbit_dialog_file_filter* filter
) {
  char* patterns = moonbit_dialog_join_filter_patterns(filter, " ");
  const char* name = moonbit_dialog_has_text(filter->name)
    ? filter->name
    : patterns;
  if (name == NULL || patterns == NULL) {
    free(patterns);
    return NULL;
  }

  size_t total =
    strlen("--file-filter=") + strlen(name) + strlen(" | ") +
    strlen(patterns) + 1;
  char* result = (char*)malloc(total);
  if (result != NULL) {
    snprintf(result, total, "--file-filter=%s | %s", name, patterns);
  }
  free(patterns);
  return result;
}

static int32_t moonbit_dialog_open_file_linux(
  const char* title_utf8,
  const char* directory_utf8,
  const char* filters_utf8,
  moonbit_bytes_t path_out,
  int32_t path_out_len
) {
  moonbit_dialog_file_filters filters;
  memset(&filters, 0, sizeof(filters));
  if (!moonbit_dialog_parse_file_filters(filters_utf8, &filters)) {
    return moonbit_dialog_encode_failure(DIALOG_BACKEND_LINUX_ZENITY, 21);
  }

  char* start_path = moonbit_dialog_make_start_path(directory_utf8, "", 1);
  char** zenity_filter_args = NULL;
  if (filters.count > 0) {
    zenity_filter_args = (char**)calloc((size_t)filters.count, sizeof(char*));
    if (zenity_filter_args == NULL) {
      free(start_path);
      moonbit_dialog_free_file_filters(&filters);
      return moonbit_dialog_encode_failure(DIALOG_BACKEND_LINUX_ZENITY, 22);
    }
    for (int32_t i = 0; i < filters.count; i++) {
      zenity_filter_args[i] = moonbit_dialog_make_zenity_filter_arg(
        &filters.items[i]
      );
      if (zenity_filter_args[i] == NULL) {
        moonbit_dialog_free_string_array(zenity_filter_args, filters.count);
        free(start_path);
        moonbit_dialog_free_file_filters(&filters);
        return moonbit_dialog_encode_failure(DIALOG_BACKEND_LINUX_ZENITY, 23);
      }
    }
  }

  int zenity_arg_count = 4 + (start_path != NULL ? 2 : 0) + filters.count + 1;
  const char** zenity_args = (const char**)malloc(
    (size_t)zenity_arg_count * sizeof(char*)
  );
  if (zenity_args == NULL) {
    moonbit_dialog_free_string_array(zenity_filter_args, filters.count);
    free(start_path);
    moonbit_dialog_free_file_filters(&filters);
    return moonbit_dialog_encode_failure(DIALOG_BACKEND_LINUX_ZENITY, 24);
  }
  int zenity_index = 0;
  zenity_args[zenity_index++] = "zenity";
  zenity_args[zenity_index++] = "--file-selection";
  zenity_args[zenity_index++] = "--title";
  zenity_args[zenity_index++] = title_utf8;
  if (start_path != NULL) {
    zenity_args[zenity_index++] = "--filename";
    zenity_args[zenity_index++] = start_path;
  }
  for (int32_t i = 0; i < filters.count; i++) {
    zenity_args[zenity_index++] = zenity_filter_args[i];
  }
  zenity_args[zenity_index] = NULL;

  int status = moonbit_dialog_run_program_capture_stdout(
    zenity_args,
    path_out,
    path_out_len
  );
  free(zenity_args);
  moonbit_dialog_free_string_array(zenity_filter_args, filters.count);
  if (status != DIALOG_HELPER_MISSING) {
    free(start_path);
    moonbit_dialog_free_file_filters(&filters);
    if (status == 0) {
      return moonbit_dialog_encode_success(
        DIALOG_BACKEND_LINUX_ZENITY,
        DIALOG_RESPONSE_OK
      );
    }
    if (status == 1 || status == 5) {
      return moonbit_dialog_encode_success(
        DIALOG_BACKEND_LINUX_ZENITY,
        DIALOG_RESPONSE_CANCEL
      );
    }
    return moonbit_dialog_encode_failure(DIALOG_BACKEND_LINUX_ZENITY, status);
  }

  char* kdialog_filter = moonbit_dialog_join_all_filter_patterns(
    &filters,
    " "
  );
  const char* kdialog_args[9];
  int kdialog_index = 0;
  kdialog_args[kdialog_index++] = "kdialog";
  kdialog_args[kdialog_index++] = "--getopenfilename";
  if (start_path != NULL) {
    kdialog_args[kdialog_index++] = start_path;
  }
  if (kdialog_filter != NULL) {
    kdialog_args[kdialog_index++] = kdialog_filter;
  }
  kdialog_args[kdialog_index++] = "--title";
  kdialog_args[kdialog_index++] = title_utf8;
  kdialog_args[kdialog_index] = NULL;

  status = moonbit_dialog_run_program_capture_stdout(
    kdialog_args,
    path_out,
    path_out_len
  );
  free(kdialog_filter);
  free(start_path);
  moonbit_dialog_free_file_filters(&filters);
  if (status == DIALOG_HELPER_MISSING) {
    return DIALOG_STATUS_BACKEND_UNAVAILABLE;
  }
  if (status == 0) {
    return moonbit_dialog_encode_success(
      DIALOG_BACKEND_LINUX_KDIALOG,
      DIALOG_RESPONSE_OK
    );
  }
  if (status == 1 || status == 255) {
    return moonbit_dialog_encode_success(
      DIALOG_BACKEND_LINUX_KDIALOG,
      DIALOG_RESPONSE_CANCEL
    );
  }
  return moonbit_dialog_encode_failure(DIALOG_BACKEND_LINUX_KDIALOG, status);
}

static int32_t moonbit_dialog_open_files_linux(
  const char* title_utf8,
  const char* directory_utf8,
  const char* filters_utf8,
  moonbit_bytes_t path_out,
  int32_t path_out_len
) {
  moonbit_dialog_file_filters filters;
  memset(&filters, 0, sizeof(filters));
  if (!moonbit_dialog_parse_file_filters(filters_utf8, &filters)) {
    return moonbit_dialog_encode_failure(DIALOG_BACKEND_LINUX_ZENITY, 41);
  }

  char* start_path = moonbit_dialog_make_start_path(directory_utf8, "", 1);
  char** zenity_filter_args = NULL;
  if (filters.count > 0) {
    zenity_filter_args = (char**)calloc((size_t)filters.count, sizeof(char*));
    if (zenity_filter_args == NULL) {
      free(start_path);
      moonbit_dialog_free_file_filters(&filters);
      return moonbit_dialog_encode_failure(DIALOG_BACKEND_LINUX_ZENITY, 42);
    }
    for (int32_t i = 0; i < filters.count; i++) {
      zenity_filter_args[i] = moonbit_dialog_make_zenity_filter_arg(
        &filters.items[i]
      );
      if (zenity_filter_args[i] == NULL) {
        moonbit_dialog_free_string_array(zenity_filter_args, filters.count);
        free(start_path);
        moonbit_dialog_free_file_filters(&filters);
        return moonbit_dialog_encode_failure(DIALOG_BACKEND_LINUX_ZENITY, 43);
      }
    }
  }

  int zenity_arg_count =
    6 + (start_path != NULL ? 2 : 0) + filters.count + 1;
  const char** zenity_args = (const char**)malloc(
    (size_t)zenity_arg_count * sizeof(char*)
  );
  if (zenity_args == NULL) {
    moonbit_dialog_free_string_array(zenity_filter_args, filters.count);
    free(start_path);
    moonbit_dialog_free_file_filters(&filters);
    return moonbit_dialog_encode_failure(DIALOG_BACKEND_LINUX_ZENITY, 44);
  }
  int zenity_index = 0;
  zenity_args[zenity_index++] = "zenity";
  zenity_args[zenity_index++] = "--file-selection";
  zenity_args[zenity_index++] = "--multiple";
  zenity_args[zenity_index++] = "--separator=\n";
  zenity_args[zenity_index++] = "--title";
  zenity_args[zenity_index++] = title_utf8;
  if (start_path != NULL) {
    zenity_args[zenity_index++] = "--filename";
    zenity_args[zenity_index++] = start_path;
  }
  for (int32_t i = 0; i < filters.count; i++) {
    zenity_args[zenity_index++] = zenity_filter_args[i];
  }
  zenity_args[zenity_index] = NULL;

  char* raw_output = (char*)malloc((size_t)path_out_len);
  if (raw_output == NULL) {
    free(zenity_args);
    moonbit_dialog_free_string_array(zenity_filter_args, filters.count);
    free(start_path);
    moonbit_dialog_free_file_filters(&filters);
    return moonbit_dialog_encode_failure(DIALOG_BACKEND_LINUX_ZENITY, 45);
  }

  int status = moonbit_dialog_run_program_capture_stdout(
    zenity_args,
    (moonbit_bytes_t)raw_output,
    path_out_len
  );
  free(zenity_args);
  moonbit_dialog_free_string_array(zenity_filter_args, filters.count);
  if (status != DIALOG_HELPER_MISSING) {
    free(start_path);
    moonbit_dialog_free_file_filters(&filters);
    if (status == 0) {
      if (
        !moonbit_dialog_copy_line_separated_paths_to_output(
          raw_output,
          path_out,
          path_out_len
        )
      ) {
        free(raw_output);
        return moonbit_dialog_encode_failure(DIALOG_BACKEND_LINUX_ZENITY, 46);
      }
      free(raw_output);
      return moonbit_dialog_encode_success(
        DIALOG_BACKEND_LINUX_ZENITY,
        DIALOG_RESPONSE_OK
      );
    }
    free(raw_output);
    if (status == 1 || status == 5) {
      return moonbit_dialog_encode_success(
        DIALOG_BACKEND_LINUX_ZENITY,
        DIALOG_RESPONSE_CANCEL
      );
    }
    return moonbit_dialog_encode_failure(DIALOG_BACKEND_LINUX_ZENITY, status);
  }
  free(raw_output);

  char* kdialog_filter = moonbit_dialog_join_all_filter_patterns(
    &filters,
    " "
  );
  const char* kdialog_args[10];
  int kdialog_index = 0;
  kdialog_args[kdialog_index++] = "kdialog";
  kdialog_args[kdialog_index++] = "--getopenfilename";
  if (start_path != NULL) {
    kdialog_args[kdialog_index++] = start_path;
  }
  if (kdialog_filter != NULL) {
    kdialog_args[kdialog_index++] = kdialog_filter;
  }
  kdialog_args[kdialog_index++] = "--multiple";
  kdialog_args[kdialog_index++] = "--separate-output";
  kdialog_args[kdialog_index++] = "--title";
  kdialog_args[kdialog_index++] = title_utf8;
  kdialog_args[kdialog_index] = NULL;

  raw_output = (char*)malloc((size_t)path_out_len);
  if (raw_output == NULL) {
    free(kdialog_filter);
    free(start_path);
    moonbit_dialog_free_file_filters(&filters);
    return moonbit_dialog_encode_failure(DIALOG_BACKEND_LINUX_KDIALOG, 47);
  }

  status = moonbit_dialog_run_program_capture_stdout(
    kdialog_args,
    (moonbit_bytes_t)raw_output,
    path_out_len
  );
  free(kdialog_filter);
  free(start_path);
  moonbit_dialog_free_file_filters(&filters);
  if (status == DIALOG_HELPER_MISSING) {
    free(raw_output);
    return DIALOG_STATUS_BACKEND_UNAVAILABLE;
  }
  if (status == 0) {
    if (
      !moonbit_dialog_copy_line_separated_paths_to_output(
        raw_output,
        path_out,
        path_out_len
      )
    ) {
      free(raw_output);
      return moonbit_dialog_encode_failure(DIALOG_BACKEND_LINUX_KDIALOG, 48);
    }
    free(raw_output);
    return moonbit_dialog_encode_success(
      DIALOG_BACKEND_LINUX_KDIALOG,
      DIALOG_RESPONSE_OK
    );
  }
  free(raw_output);
  if (status == 1 || status == 255) {
    return moonbit_dialog_encode_success(
      DIALOG_BACKEND_LINUX_KDIALOG,
      DIALOG_RESPONSE_CANCEL
    );
  }
  return moonbit_dialog_encode_failure(DIALOG_BACKEND_LINUX_KDIALOG, status);
}

static int32_t moonbit_dialog_save_file_linux(
  const char* title_utf8,
  const char* directory_utf8,
  const char* file_name_utf8,
  const char* filters_utf8,
  const char* default_extension_utf8,
  moonbit_bytes_t path_out,
  int32_t path_out_len
) {
  moonbit_dialog_file_filters filters;
  memset(&filters, 0, sizeof(filters));
  if (!moonbit_dialog_parse_file_filters(filters_utf8, &filters)) {
    return moonbit_dialog_encode_failure(DIALOG_BACKEND_LINUX_ZENITY, 31);
  }

  char* start_path = moonbit_dialog_make_start_path(
    directory_utf8,
    file_name_utf8,
    1
  );
  char** zenity_filter_args = NULL;
  if (filters.count > 0) {
    zenity_filter_args = (char**)calloc((size_t)filters.count, sizeof(char*));
    if (zenity_filter_args == NULL) {
      free(start_path);
      moonbit_dialog_free_file_filters(&filters);
      return moonbit_dialog_encode_failure(DIALOG_BACKEND_LINUX_ZENITY, 32);
    }
    for (int32_t i = 0; i < filters.count; i++) {
      zenity_filter_args[i] = moonbit_dialog_make_zenity_filter_arg(
        &filters.items[i]
      );
      if (zenity_filter_args[i] == NULL) {
        moonbit_dialog_free_string_array(zenity_filter_args, filters.count);
        free(start_path);
        moonbit_dialog_free_file_filters(&filters);
        return moonbit_dialog_encode_failure(DIALOG_BACKEND_LINUX_ZENITY, 33);
      }
    }
  }

  int zenity_arg_count = 6 + (start_path != NULL ? 2 : 0) + filters.count + 1;
  const char** zenity_args = (const char**)malloc(
    (size_t)zenity_arg_count * sizeof(char*)
  );
  if (zenity_args == NULL) {
    moonbit_dialog_free_string_array(zenity_filter_args, filters.count);
    free(start_path);
    moonbit_dialog_free_file_filters(&filters);
    return moonbit_dialog_encode_failure(DIALOG_BACKEND_LINUX_ZENITY, 34);
  }
  int zenity_index = 0;
  zenity_args[zenity_index++] = "zenity";
  zenity_args[zenity_index++] = "--file-selection";
  zenity_args[zenity_index++] = "--save";
  zenity_args[zenity_index++] = "--confirm-overwrite";
  zenity_args[zenity_index++] = "--title";
  zenity_args[zenity_index++] = title_utf8;
  if (start_path != NULL) {
    zenity_args[zenity_index++] = "--filename";
    zenity_args[zenity_index++] = start_path;
  }
  for (int32_t i = 0; i < filters.count; i++) {
    zenity_args[zenity_index++] = zenity_filter_args[i];
  }
  zenity_args[zenity_index] = NULL;

  int status = moonbit_dialog_run_program_capture_stdout(
    zenity_args,
    path_out,
    path_out_len
  );
  free(zenity_args);
  moonbit_dialog_free_string_array(zenity_filter_args, filters.count);
  if (status != DIALOG_HELPER_MISSING) {
    free(start_path);
    moonbit_dialog_free_file_filters(&filters);
    if (status == 0) {
      if (
        !moonbit_dialog_apply_default_extension(
          path_out,
          path_out_len,
          default_extension_utf8
        )
      ) {
        return moonbit_dialog_encode_failure(DIALOG_BACKEND_LINUX_ZENITY, 35);
      }
      return moonbit_dialog_encode_success(
        DIALOG_BACKEND_LINUX_ZENITY,
        DIALOG_RESPONSE_OK
      );
    }
    if (status == 1 || status == 5) {
      return moonbit_dialog_encode_success(
        DIALOG_BACKEND_LINUX_ZENITY,
        DIALOG_RESPONSE_CANCEL
      );
    }
    return moonbit_dialog_encode_failure(DIALOG_BACKEND_LINUX_ZENITY, status);
  }

  char* kdialog_filter = moonbit_dialog_join_all_filter_patterns(
    &filters,
    " "
  );
  const char* kdialog_args[9];
  int kdialog_index = 0;
  kdialog_args[kdialog_index++] = "kdialog";
  kdialog_args[kdialog_index++] = "--getsavefilename";
  if (start_path != NULL) {
    kdialog_args[kdialog_index++] = start_path;
  }
  if (kdialog_filter != NULL) {
    kdialog_args[kdialog_index++] = kdialog_filter;
  }
  kdialog_args[kdialog_index++] = "--title";
  kdialog_args[kdialog_index++] = title_utf8;
  kdialog_args[kdialog_index] = NULL;

  status = moonbit_dialog_run_program_capture_stdout(
    kdialog_args,
    path_out,
    path_out_len
  );
  free(kdialog_filter);
  free(start_path);
  moonbit_dialog_free_file_filters(&filters);
  if (status == DIALOG_HELPER_MISSING) {
    return DIALOG_STATUS_BACKEND_UNAVAILABLE;
  }
  if (status == 0) {
    if (
      !moonbit_dialog_apply_default_extension(
        path_out,
        path_out_len,
        default_extension_utf8
      )
    ) {
      return moonbit_dialog_encode_failure(DIALOG_BACKEND_LINUX_KDIALOG, 36);
    }
    return moonbit_dialog_encode_success(
      DIALOG_BACKEND_LINUX_KDIALOG,
      DIALOG_RESPONSE_OK
    );
  }
  if (status == 1 || status == 255) {
    return moonbit_dialog_encode_success(
      DIALOG_BACKEND_LINUX_KDIALOG,
      DIALOG_RESPONSE_CANCEL
    );
  }
  return moonbit_dialog_encode_failure(DIALOG_BACKEND_LINUX_KDIALOG, status);
}

static int32_t moonbit_dialog_select_folder_linux(
  const char* title_utf8,
  const char* directory_utf8,
  moonbit_bytes_t path_out,
  int32_t path_out_len
) {
  char* start_path = moonbit_dialog_make_start_path(directory_utf8, "", 1);
  const char* zenity_args[10];
  int zenity_index = 0;
  zenity_args[zenity_index++] = "zenity";
  zenity_args[zenity_index++] = "--file-selection";
  zenity_args[zenity_index++] = "--directory";
  zenity_args[zenity_index++] = "--title";
  zenity_args[zenity_index++] = title_utf8;
  if (start_path != NULL) {
    zenity_args[zenity_index++] = "--filename";
    zenity_args[zenity_index++] = start_path;
  }
  zenity_args[zenity_index] = NULL;

  int status = moonbit_dialog_run_program_capture_stdout(
    zenity_args,
    path_out,
    path_out_len
  );
  if (status != DIALOG_HELPER_MISSING) {
    free(start_path);
    if (status == 0) {
      return moonbit_dialog_encode_success(
        DIALOG_BACKEND_LINUX_ZENITY,
        DIALOG_RESPONSE_OK
      );
    }
    if (status == 1 || status == 5) {
      return moonbit_dialog_encode_success(
        DIALOG_BACKEND_LINUX_ZENITY,
        DIALOG_RESPONSE_CANCEL
      );
    }
    return moonbit_dialog_encode_failure(DIALOG_BACKEND_LINUX_ZENITY, status);
  }

  const char* kdialog_args[8];
  int kdialog_index = 0;
  kdialog_args[kdialog_index++] = "kdialog";
  kdialog_args[kdialog_index++] = "--getexistingdirectory";
  if (start_path != NULL) {
    kdialog_args[kdialog_index++] = start_path;
  }
  kdialog_args[kdialog_index++] = "--title";
  kdialog_args[kdialog_index++] = title_utf8;
  kdialog_args[kdialog_index] = NULL;

  status = moonbit_dialog_run_program_capture_stdout(
    kdialog_args,
    path_out,
    path_out_len
  );
  free(start_path);
  if (status == DIALOG_HELPER_MISSING) {
    return DIALOG_STATUS_BACKEND_UNAVAILABLE;
  }
  if (status == 0) {
    return moonbit_dialog_encode_success(
      DIALOG_BACKEND_LINUX_KDIALOG,
      DIALOG_RESPONSE_OK
    );
  }
  if (status == 1 || status == 255) {
    return moonbit_dialog_encode_success(
      DIALOG_BACKEND_LINUX_KDIALOG,
      DIALOG_RESPONSE_CANCEL
    );
  }
  return moonbit_dialog_encode_failure(DIALOG_BACKEND_LINUX_KDIALOG, status);
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

MOONBIT_FFI_EXPORT
int32_t moonbit_dialog_open_file(
  moonbit_bytes_t title,
  moonbit_bytes_t directory,
  moonbit_bytes_t filters,
  moonbit_bytes_t path_out,
  int32_t path_out_len
) {
#if defined(_WIN32)
  return moonbit_dialog_open_file_windows(
    (const char*)title,
    (const char*)directory,
    (const char*)filters,
    path_out,
    path_out_len
  );
#elif defined(__APPLE__)
  return moonbit_dialog_open_file_macos(
    (const char*)title,
    (const char*)directory,
    (const char*)filters,
    path_out,
    path_out_len
  );
#elif defined(__linux__)
  return moonbit_dialog_open_file_linux(
    (const char*)title,
    (const char*)directory,
    (const char*)filters,
    path_out,
    path_out_len
  );
#else
  (void)title;
  (void)directory;
  (void)filters;
  moonbit_dialog_clear_output(path_out, path_out_len);
  return DIALOG_STATUS_UNSUPPORTED_PLATFORM;
#endif
}

MOONBIT_FFI_EXPORT
int32_t moonbit_dialog_open_files(
  moonbit_bytes_t title,
  moonbit_bytes_t directory,
  moonbit_bytes_t filters,
  moonbit_bytes_t path_out,
  int32_t path_out_len
) {
#if defined(_WIN32)
  return moonbit_dialog_open_files_windows(
    (const char*)title,
    (const char*)directory,
    (const char*)filters,
    path_out,
    path_out_len
  );
#elif defined(__APPLE__)
  return moonbit_dialog_open_files_macos(
    (const char*)title,
    (const char*)directory,
    (const char*)filters,
    path_out,
    path_out_len
  );
#elif defined(__linux__)
  return moonbit_dialog_open_files_linux(
    (const char*)title,
    (const char*)directory,
    (const char*)filters,
    path_out,
    path_out_len
  );
#else
  (void)title;
  (void)directory;
  (void)filters;
  moonbit_dialog_clear_output(path_out, path_out_len);
  return DIALOG_STATUS_UNSUPPORTED_PLATFORM;
#endif
}

MOONBIT_FFI_EXPORT
int32_t moonbit_dialog_save_file(
  moonbit_bytes_t title,
  moonbit_bytes_t directory,
  moonbit_bytes_t file_name,
  moonbit_bytes_t filters,
  moonbit_bytes_t default_extension,
  moonbit_bytes_t path_out,
  int32_t path_out_len
) {
#if defined(_WIN32)
  return moonbit_dialog_save_file_windows(
    (const char*)title,
    (const char*)directory,
    (const char*)file_name,
    (const char*)filters,
    (const char*)default_extension,
    path_out,
    path_out_len
  );
#elif defined(__APPLE__)
  return moonbit_dialog_save_file_macos(
    (const char*)title,
    (const char*)directory,
    (const char*)file_name,
    (const char*)filters,
    (const char*)default_extension,
    path_out,
    path_out_len
  );
#elif defined(__linux__)
  return moonbit_dialog_save_file_linux(
    (const char*)title,
    (const char*)directory,
    (const char*)file_name,
    (const char*)filters,
    (const char*)default_extension,
    path_out,
    path_out_len
  );
#else
  (void)title;
  (void)directory;
  (void)file_name;
  (void)filters;
  (void)default_extension;
  moonbit_dialog_clear_output(path_out, path_out_len);
  return DIALOG_STATUS_UNSUPPORTED_PLATFORM;
#endif
}

MOONBIT_FFI_EXPORT
int32_t moonbit_dialog_select_folder(
  moonbit_bytes_t title,
  moonbit_bytes_t directory,
  moonbit_bytes_t path_out,
  int32_t path_out_len
) {
#if defined(_WIN32)
  return moonbit_dialog_select_folder_windows(
    (const char*)title,
    (const char*)directory,
    path_out,
    path_out_len
  );
#elif defined(__APPLE__)
  return moonbit_dialog_select_folder_macos(
    (const char*)title,
    (const char*)directory,
    path_out,
    path_out_len
  );
#elif defined(__linux__)
  return moonbit_dialog_select_folder_linux(
    (const char*)title,
    (const char*)directory,
    path_out,
    path_out_len
  );
#else
  (void)title;
  (void)directory;
  moonbit_dialog_clear_output(path_out, path_out_len);
  return DIALOG_STATUS_UNSUPPORTED_PLATFORM;
#endif
}
