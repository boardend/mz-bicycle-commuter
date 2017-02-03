#!/bin/sh
if ! [ -x "$(command -v minicom)" ]; then
  echo 'Error: Minicom is not installed.' >&2
  exit 1
fi
