#!/bin/sh
find . \( -path ./gazpacho/\*.py \
          -o -path ./bin/gazpacho \
          -o -name setup.py \) \
  -a -name \*|xargs pyflakes
