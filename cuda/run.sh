#!/bin/sh
make


./angle 1000000 32
echo ---------------
./angle 1000000 64
echo ---------------
./angle 1000000 128
echo ---------------
./angle 1000000 256
echo ---------------
./angle 1000000 512
echo ---------------
./angle 5000000 32
echo ---------------
./angle 5000000 64
echo ---------------
./angle 5000000 128
echo ---------------
./angle 5000000 256
echo ---------------
./angle 5000000 512
echo ---------------
./angle 10000000 32
echo ---------------
./angle 10000000 64
echo ---------------
./angle 10000000 128
echo ---------------
./angle 10000000 256
echo ---------------
./angle 10000000 512
