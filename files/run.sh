#!/bin/bash

./client -s ../../../serverFifo -o matrix1.csv &
./client -s ../../../serverFifo -o matrix2.csv &
./client -s ../../../serverFifo -o matrix3.csv &
./client -s ../../../serverFifo -o matrix1.csv &
./client -s ../../../serverFifo -o matrix2.csv &
./client -s ../../../serverFifo -o matrix3.csv


