#!/bin/zsh
set -xe

brew install cmake pkg-config jack xquartz qjackctl

mkdir -p build
pushd build

# Install clthreads
[[ -d clthreads ]] || git clone https://github.com/fugalh/clthreads.git
pushd clthreads/source
make
sudo make install
popd

# Install clxclient
[[ -d clxclient ]] || git clone https://github.com/fugalh/clxclient.git
pushd clxclient/source
make
sudo make install
popd

# Build aeolus
cmake ..
make
sudo make install

# You'll need to provide stops, e.g.  https://kokkinizita.linuxaudio.org/linuxaudio/downloads/stops-0.4.0.tar.bz2
# Refer to the README
