@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul
if errorlevel 1 exit /b %errorlevel%
cl.exe /nologo /std:c++17 /EHsc /W4 /O2 ^
  /I"C:\Users\k\Repository\External\PresentMon_2-5-1\IntelPresentMon\ControlLib" ^
  /Fo"%~dp0unity-drs-audit.obj" ^
  /Fe"%~dp0unity-drs-audit.exe" ^
  "%~dp0unity-drs-audit.cpp"
exit /b %errorlevel%
