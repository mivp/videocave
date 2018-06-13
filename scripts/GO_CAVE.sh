#!/bin/sh

mpirun --hostfile hostfile_1_4 -np 5 videocave -d 5 -m 5 &
mpirun --hostfile hostfile_5_4 -np 5 videocave -d 3 -m 5 &
mpirun --hostfile hostfile_9_4 -np 5 videocave -d 4 -m 5 &
mpirun --hostfile hostfile_13_4 -np 5 videocave -d 2 -m 5 &
mpirun --hostfile hostfile_17_4 -np 5 videocave -d 1 -m 8 &
