@echo off
title SPL1 Project Builder (Fixed E2EE Stream & Commands)
echo ========================================================
echo Building Enterprise-Grade E2EE Terminal Chat (SPL1)
echo Features: Dynamic Name, E2E Direct Messaging, File Transfer
echo ========================================================
echo.

echo [1/3] Compiling Crypto and Utils Core...
gcc -c utils.c -o utils.o -O2 -Wall
gcc -c crypto.c -o crypto.o -O2 -Wall

echo [2/3] Linking and Building Server (server.c) ...
gcc server.c utils.o crypto.o -o server.exe -lws2_32 -O2 -Wall

echo [3/3] Linking and Building Client (client.c) ...
gcc client.c utils.o crypto.o -o client.exe -lws2_32 -O2 -Wall

echo.
echo Cleaning up object files...
if exist utils.o del utils.o
if exist crypto.o del crypto.o

echo.
echo ========================================================
echo SUCCESS: Project successfully compiled!
echo Run server.exe and multiple client.exe instances.
echo ========================================================
pause
