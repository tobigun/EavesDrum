#!/bin/sh

pip install --upgrade platformio

# install picotool
if [ ! -d picotool ]; then
    wget https://github.com/earlephilhower/pico-quick-toolchain/releases/download/4.1.0/x86_64-linux-gnu.picotool-c56c005.250530.tar.gz
    tar xvzf x86_64-linux-gnu.picotool-c56c005.250530.tar.gz
    rm x86_64-linux-gnu.picotool-c56c005.250530.tar.gz
fi

# install erlang for uf2tool
apt-get update
apt-get -y install rebar3

# Build uf2tool
if [ ! -d uf2tool-1.1.0 ]; then
    wget https://github.com/pguyot/uf2tool/archive/refs/tags/v1.1.0.tar.gz
    tar xvzf v1.1.0.tar.gz
    rm v1.1.0.tar.gz
    cd uf2tool-1.1.0
    rebar3 escriptize
    cd ..
fi

echo "Waiting for terminal connection ..."
tail -F /dev/null
