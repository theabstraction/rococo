@echo off
cd %~dp0
nmake -nologo -f package.mak all CONFIG=%1
