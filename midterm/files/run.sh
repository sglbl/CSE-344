#!/bin/bash

./client -s serverFifo -o files/matrix1.csv &
./client -s serverFifo -o files/matrix2.csv &
./client -s serverFifo -o files/matrix3.csv &
./client -s serverFifo -o files/matrix1.csv &
./client -s serverFifo -o files/matrix2.csv &
./client -s serverFifo -o files/matrix3.csv


