@echo off

bin\x64\Debug\mpiexec.exe -np 4 bin\x64\Debug\MatrixMultiply.exe A.txt B.txt result.txt
