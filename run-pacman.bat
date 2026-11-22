@echo off
clang pacman.c -o pacman.exe && conhost.exe "%~dp0pacman.exe"