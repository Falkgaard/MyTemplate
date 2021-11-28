echo "Building debug build..."
cmake -S . -Bdebug -DCMAKE_BUILD_TYPE=debug && cd ./debug/ && make
echo "... done!"
