#!/bin/bash

rm ~/.cache/deepin/org.deepin.dde.lock/org.deepin.dde.lock.log

cmake --build build/ --config Releae --target dde-lock -- -j15
sudo cp build/dde-lock /usr/bin -f
sudo killall dde-lock
dde-lock -t
