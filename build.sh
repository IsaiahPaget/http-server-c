#!/bin/sh

set -e # Exit early if any commands fail

if [ "$1" = "-v" ]; then
  echo "🔍 Running with Valgrind..."
	clang ./src/main.c -std=c99 -Wall -Werror -g
  exec valgrind --leak-check=full --track-origins=yes ./a.out
elif [ "$1" = "-s" ]; then
  echo "🧪 Running with AddressSanitizer..."
  clang ./src/main.c -std=c99 -Wall -Werror -g -fsanitize=address,undefined
  exec ./a.out
else
  clang ./src/main.c -std=c99 -Wall -Werror -g
  exec ./a.out
fi
