#! /bin/bash
rm -rf out
rm -rf bin && rm -rf tests/bin
mkdir out && cd out
cmake ..
make -j8