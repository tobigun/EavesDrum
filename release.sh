#!/bin/sh

# Usage:
# ./release minor | major | patch

cd webui

# Workaround for npm version bug if folder is not the git root dir
# See https://stackoverflow.com/questions/75965870/npm-version-command-not-creating-git-tag-when-the-npm-app-is-in-a-sub-folder
mkdir .git
npm version $@ -m "set version to %s"
rmdir .git
