#!/usr/bin/env bash
printf '\033[0;1;37mRunning release build...\033[0m\n' ;
cd ./release/ && ./MyTemplate
if [ "$?" -ne "0" ]; then
	printf '\033[0;1;37m... error!\033[0m\n'
	exit 1
else
	printf '\033[0;1;37m... done!\033[0m\n'
	exit 0
fi
#eof
