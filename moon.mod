name = "justjavac/dialog"

version = "0.1.4"

readme = "README.mbt.md"

repository = "https://github.com/justjavac/moonbit-dialog"

license = "MIT"

keywords = [ "dialog", "gui", "native", "message-box" ]

description = "Native-only message dialogs for MoonBit on Windows, macOS, and Linux."

preferred_target = "native"

options(
  source: "src",
  supported_targets: "+native",
  "--moonbit-unstable-prebuild": "native_link_config.mjs",
)
