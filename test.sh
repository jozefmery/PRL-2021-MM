#!/bin/bash

# mat1=$(head -n1 mat1)
# mat2=$(head -n1 mat2)
 
# cpus=$((mat1*mat2))
 
# mpic++ --prefix /usr/local/share/OpenMPI -o mm mm.cpp -std=c++17
# mpirun --prefix /usr/local/share/OpenMPI -np $cpus mm
# rm -f mm

#  TODO remove
mpic++ --prefix /usr/local/share/OpenMPI -o mm mm.cpp -std=c++17
mpirun --prefix /usr/local/share/OpenMPI --oversubscribe -np 12 mm
# rm -f mm