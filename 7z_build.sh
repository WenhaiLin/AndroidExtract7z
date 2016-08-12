#!/bin/sh
#rm -rf obj
rm -rf AndroidExtract7z

ndk-build NDK_DEBUG=0 NDK_PROJECT_PATH=.

mv libs AndroidExtract7z
#rm -rf obj