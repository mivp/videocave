#!/bin/sh

mpirun --hostfile hostfile_localhost -np 5 videocave -d 0 -m 9
