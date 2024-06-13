# Rococo.make makefile.mak Copyright(c) 2022. All rights reserved, Mark Anthony Taylor - mark.anthony.taylor@gmail.com. 
# This file is part of the Rococo libraries. It is open source and free to use and vary.

# When we call the mak file from the batch file, the batch file sets the working directory to match the container for the makefile

!IFNDEF CONFIG
!  ERROR build\tools\ship.sexystudio.mak error: CONFIG was not defined for package.mak. It should  be passed via command line either as CONFIG=debug or CONFIG=release
!ENDIF

ROCOCO = $(abspath ..\..\)
BIN64 = $(ROCOCO)gen\bin\win64^\
BIN = $(BIN64)$(lowercase $(CONFIG))^\

TARGET = $(ROCOCO)SexyStudioBin^\

$(TARGET)sexystudio.dll: $(BIN)sexystudio.dll
	-shutdown.npp.bat
	-copy $(BIN)sexystudio.dll  $(TARGET)
	
$(TARGET)sexystudio.LS.exe: $(BIN)sexystudio.LS.exe
	-copy $(BIN)sexystudio.LS.exe  $(TARGET)
	
$(TARGET)sexystudio.app.exe: $(BIN)sexystudio.app.exe
	-shutdown.npp.bat
	-copy $(BIN)sexystudio.app.exe  $(TARGET)
	
$(TARGET)sexystudio.4.npp.dll: $(BIN)sexystudio.4.npp.dll
	-shutdown.npp.bat
	-copy $(BIN)sexystudio.4.npp.dll  $(TARGET)
	
$(TARGET)rococo.util.dll: $(BIN)rococo.util.dll
	-shutdown.npp.bat
	-copy $(BIN)rococo.util.dll  $(TARGET)
	
$(TARGET)rococo.windows.dll: $(BIN)rococo.windows.dll
	-shutdown.npp.bat
	-copy $(BIN)rococo.windows.dll  $(TARGET)
		
$(TARGET)sexy.script.dll: $(BIN)sexy.script.dll
	-shutdown.npp.bat
	-copy $(BIN)sexy.script.dll $(TARGET)
			
$(TARGET)sexy.util.dll: $(BIN)sexy.util.dll
	-shutdown.npp.bat
	-copy $(BIN)sexy.util.dll  $(TARGET)
	
$(TARGET)lib-tiff.dll: $(BIN)lib-tiff.dll
	-shutdown.npp.bat
	-copy $(BIN)lib-tiff.dll  $(TARGET)
		
$(TARGET)lib-jpg.dll: $(BIN)lib-jpg.dll
	-shutdown.npp.bat
	-copy $(BIN)lib-jpg.dll $(TARGET)
			
$(TARGET)lib-zip.dll: $(BIN)lib-zip.dll
	-shutdown.npp.bat
	-copy $(BIN)lib-zip.dll  $(TARGET)

$(TARGET)rococo.sexml.dll: $(BIN)rococo.sexml.dll
	-shutdown.npp.bat
	-copy $(BIN)rococo.sexml.dll  $(TARGET)

$(TARGET)rococo.sex.inference.dll: $(BIN)rococo.sex.inference.dll
	-shutdown.npp.bat
	-copy $(BIN)rococo.sex.inference.dll  $(TARGET)

$(TARGET)rococo.graphics.dll: $(BIN)rococo.graphics.dll
	-shutdown.npp.bat
	-copy $(BIN)rococo.graphics.dll  $(TARGET)
	
# In normal mode of operation, all we want to do is to ensure the inl file is up to date
all: $(TARGET)sexystudio.dll $(TARGET)sexystudio.LS.exe $(TARGET)sexystudio.app.exe $(TARGET)rococo.util.dll $(TARGET)rococo.windows.dll $(TARGET)sexystudio.4.npp.dll $(TARGET)sexy.script.dll $(TARGET)sexy.util.dll $(TARGET)lib-tiff.dll $(TARGET)lib-jpg.dll $(TARGET)lib-zip.dll $(TARGET)rococo.sexml.dll  $(TARGET)rococo.sex.inference.dll $(TARGET)rococo.graphics.dll

clean:
	