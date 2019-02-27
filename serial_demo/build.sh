#!/bin/bash

gcc serial.c -g -std=c99 -lpthread -o serial_demo
if [[ $? -ne 0 ]]; then
    exit 1
fi

sudo ./serial_demo
