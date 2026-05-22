call "%ProgramFiles%\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat"

msbuild ParallelProgramming.slnx ^
    -p:Configuration=Debug ^
    -p:Platform=x64 ^
    -maxCpuCount ^
    -nologo ^
    -v:minimal
