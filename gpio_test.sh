#!/bin/bash

cd /sys/class/gpio
echo $1 > export
cd gpio$1
echo "low" > direction

while true; do
    echo "1" > value
    sleep 1
    echo "0" > value
    sleep 1
done
