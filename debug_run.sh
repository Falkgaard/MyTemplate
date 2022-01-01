#!/usr/bin/env bash
set VK_LAYER_PATH /home/falk/Code/libs/Vulkan/1.2.198.0/x86_64/etc/vulkan/explicit_layer.d/ # temp fix (TODO: fix more cleanly elsewhere)
printf '\033[0;1;37mRunning debug build...\033[0m\n' ;
cd ./debug/ ;
./MyTemplate && printf '\033[0;1;37m... done!\033[0m\n'
#eof
