#!/bin/bash
sudo socat PTY,link=/dev/ttyACM50,mode=666,raw,echo=0 PTY,link=/dev/ttyACM51,mode=666,raw,echo=0
