@echo off

set notepad_root=%ProgramFiles%\Notepad++
set notepad_dir=%notepad_root%\plugins\sexystudio.4.npp
set boot_dll=%~dp0..\..\SexyStudioBin\sexystudio.4.npp.dll


if exist "%notepad_dir%" (
	if exist "%notepad_dir%\sexystudio.4.npp.dll" del "%notepad_dir%\sexystudio.4.npp.dll"
) else (
    mkdir  "%notepad_dir%" 
)

rem dir %boot_dll%
rem dir "%notepad_dir%"

if exist %boot_dll% (
  copy "%boot_dll%" "%notepad_dir%\"
  copy "%~dp0\\npp.config.txt" "%notepad_dir%\"
) else (
  echo "Could not find %boot_dll%" 
)

