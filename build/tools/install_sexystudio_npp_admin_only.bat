@echo off

set notepad_root=%ProgramFiles%\Notepad++
set notepad_dir=%notepad_root%\plugins\sexystudio.4.npp
set boot_dll=%~dp0..\..\SexyStudioBin\sexystudio.4.npp.dll
set studio_bin=%~dp0..\..\SexyStudioBin\sexystudio.dll
for %%i in ("%studio_bin%") do SET "studio_bin_short=%%~fi"

if exist "%notepad_dir%" (
	if exist "%notepad_dir%\sexystudio.4.npp.dll" del "%notepad_dir%\sexystudio.4.npp.dll"
) else (
    mkdir  "%notepad_dir%" 
)

rem dir %boot_dll%
rem dir "%notepad_dir%"

echo DebugBinPath=%studio_bin_short% > %~dp0\\npp.config.txt

if exist %boot_dll% (
  copy "%boot_dll%" "%notepad_dir%\"
  copy "%~dp0\\npp.config.txt" "%notepad_dir%\"
) else (
  echo "Could not find %boot_dll%" 
)

