#!/bin/bash
find src/ proto/ -name "*.cpp" -or -name "*.h" -or -name "*.proto" | xargs -n1 clang-format-8 -i
