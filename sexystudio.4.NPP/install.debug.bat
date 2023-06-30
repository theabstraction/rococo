@echo off

set notepad_root=%ProgramFiles%\Notepad++
set notepad_debug_dir=%notepad_root%\plugins\sexystudio.4.notepad++.debug
set boot_debug_dll=%~dp0..\bin\sexystudio.4.notepad++.debug.dll
set util_debug_dll=%~dp0..\bin\rococo.util.debug.dll
set windows_debug_dll=%~dp0..\bin\rococo.windows.debug.dll

if exist "%notepad_debug_dir%" (
	if exist "%notepad_debug_dir%\sexystudio.4.notepad++.debug.dll" del "%notepad_debug_dir%\sexystudio.4.notepad++.debug.dll"
) else (
    mkdir  "%notepad_debug_dir%" 
)

rem dir %boot_dll%
rem dir "%notepad_dir%"

if exist %boot_debug_dll% (
  copy "%boot_debug_dll%" "%notepad_debug_dir%\"
  copy "%util_debug_dll%" "%notepad_debug_dir%\"
  copy "%windows_debug_dll%" "%notepad_debug_dir%\"
  copy %~dp0..\bin\sexy.script.debug.dll "%notepad_debug_dir%\"
  copy %~dp0..\bin\sexy.util.debug.dll "%notepad_debug_dir%\"
  copy %~dp0..\bin\misc.utils.debug.dll "%notepad_debug_dir%\"
) else (
  echo "Could not find %boot_debug_dll%" 
)

