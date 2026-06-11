#!/usr/bin/env node

import fs from "node:fs";

function main() {
  const payload = JSON.parse(fs.readFileSync(0, "utf8"));
  const env = payload.env ?? {};
  const isWindows = process.platform === "win32" || env.OS === "Windows_NT";

  process.stdout.write(JSON.stringify({
    link_configs: isWindows
      ? [
          {
            package: "justjavac/dialog",
            link_libs: ["comdlg32", "ole32"],
          },
        ]
      : [],
  }));
}

main();
