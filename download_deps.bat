@echo off
echo Downloading miniaudio.h...
powershell -Command "Invoke-WebRequest -Uri 'https://raw.githubusercontent.com/mackron/miniaudio/master/miniaudio.h' -OutFile 'third_party\miniaudio.h'"
echo Done!
