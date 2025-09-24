Import("env")

from datetime import datetime, UTC
import json
import shutil
import subprocess
import sys
import textwrap

def create_version_header():
    result = subprocess.run(['git', 'rev-parse', '--short', 'HEAD'], stdout=subprocess.PIPE, text=True)
    git_commit_hash = result.stdout.strip()

    with open('webui/package.json') as package_json_file:
        package_json = json.load(package_json_file)
    
    # get current datetime with UTC timezone but 'Z' instead of "+00:00" (e.g. 2025-05-02T07:56:10Z)
    buildTime = datetime.now(UTC)
    buildTimeString = buildTime.replace(tzinfo=None).isoformat(timespec="seconds") + 'Z'

    contents = textwrap.dedent(f"""\
    // Copyright (c) 2025 Tobias Gunkel
    // SPDX-License-Identifier: GPL-3.0-or-later

    #pragma once
    
    class Version {{
    public:
        static const char* getPackageVersion() {{ return "{package_json['version']}"; }}
        static const char* getGitCommitHash() {{ return "{git_commit_hash}"; }}
        static const char* getBuildTime() {{ return "{buildTimeString}"; }}
    }};
    """)
    
    headerFile = 'src/version.h'
    print("Updating {} with version/timestamp...".format(headerFile))
    with open(headerFile, 'w+') as file:
        file.write(contents)

def copy_config():
    print("Copy config/config.yaml to data directory...")
    shutil.copy('config/config.yaml', 'data/config.yaml')
    shutil.copy('config/config.jsonc', 'data/config.jsonc')

def before_littlefs(source, target, env):
    print("Building UI...")
    if env.Execute("npm run --prefix webui build"):
        print("UI build failed: npm returned an error", file=sys.stderr)
        Exit(1)
    copy_config()

create_version_header()
env.AddPreAction("$BUILD_DIR/littlefs.bin", before_littlefs)
