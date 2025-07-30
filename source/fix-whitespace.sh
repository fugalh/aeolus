#!/bin/bash

# Convert tabs to 8 spaces and remove trailing whitespace in all .cc and .h files
find . -name "*.cc" -o -name "*.h" | xargs sed -i '' -e 's/\t/        /g' -e 's/[[:space:]]*$//'