#!/usr/bin/env bash
printf '\033[0;1;37mCleaning project...\033[0m'
rm -fr ./build/
rm -fr ./debug/
rm -fr ./.cache/
rm -fr ./bin/
printf '\033[0;1;37m done!\033[0m\n'
