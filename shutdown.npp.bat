:retry

tasklist /fi "IMAGENAME eq Notepad++.exe" | findstr /C:"notepad++" 1>nul && ( 
  Echo.Killing task notepad++
  taskkill /im notepad++.exe
  goto :retry
)

exit /b 0