@echo off

set COMPILER_PATH="C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat"

call %COMPILER_PATH% x64

cl.exe /EHsc /openmp task1.cpp /Fe:task1.exe

if %errorlevel% equ 0 (
    echo good
    task1.exe
) else (
    echo no good
)
