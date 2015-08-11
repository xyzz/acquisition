#!/bin/bash
set -ev
sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
sudo add-apt-repository -y ppa:beineri/opt-qt542
sudo add-apt-repository -y 'deb http://llvm.org/apt/precise/ llvm-toolchain-precise-3.6 main'
wget -O - http://llvm.org/apt/llvm-snapshot.gpg.key | sudo apt-key add -
sudo apt-get update -qq
if [ "$CXX" = "g++" ]; then
	sudo apt-get install -qq g++-4.8
fi
if [ "$CXX" = "clang++" ]; then
	sudo apt-get install clang-3.6
fi
sudo apt-get install qt54base qt54webkit valgrind
