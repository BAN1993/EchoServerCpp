#!/bin/bash

DIR="$( cd "$( dirname "$0"  )" && pwd  )"

YELLOW=$(echo -e '\033[01;33m')
GREEN=$(echo -e '\033[00;32m')
RESTORE=$(echo -e '\033[0m')

echo -e "${YELLOW}\nEntering -> $DIR/libbase${GREEN}"
cd $DIR/libbase
make clean 1>/dev/null && make -j4

echo -e "${YELLOW}\nEntering -> $DIR/libsocket${GREEN}"
cd $DIR/libsocket
make clean 1>/dev/null && make -j4

echo -e "${YELLOW}\nEntering -> $DIR/libredis${GREEN}"
cd $DIR/libredis
make clean 1>/dev/null && make -j4

echo -e "${YELLOW}\nEntering -> $DIR/EchoServer${GREEN}"
cd $DIR/EchoServer
make clean 1>/dev/null && make -j4

echo -e "${RESTORE}"
