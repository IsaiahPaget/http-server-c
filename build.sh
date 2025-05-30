#!/bin/sh

set -e # Exit early if any commands fail

clang ./src/main.c -std=c99 -Wall -Werror


