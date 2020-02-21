
cd %~dp0
mkdir .build
cd .build
cmake -G "Visual Studio 16 2019" "%~dp0"
