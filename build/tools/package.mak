# Rococo.make makefile.mak Copyright(c) 2022. All rights reserved, Mark Anthony Taylor - mark.anthony.taylor@gmail.com. 
# This file is part of the Rococo libraries. It is open source and free to use and vary.

# When we call the mak file from the batch file, the batch file sets the working directory to match the container for the makefile

!IFNDEF CONFIG
!  ERROR build\tools\package.mak error: CONFIG was not defined for package.mak. It should  be passed via command line either as CONFIG=debug or CONFIG=release
!ENDIF

ROCOCO = $(abspath ..\..\)
CONTENT = $(ROCOCO)content^\
MHOST=$(CONTENT)packages\mhost_1000.sxyz
PACKAGE_DIR=$(ROCOCO)packages^\
SCRIPTS=$(CONTENT)scripts^\
INTEROP=$(SCRIPTS)interop\rococo^\
BIN64 = $(ROCOCO)gen\bin\win64^\
BIN = $(BIN64)$(lowercase $(CONFIG))^\
PACKAGER_EXE_FILENAME = rococo.packager.exe
PACKAGER_PATH = $(BIN)$(PACKAGER_EXE_FILENAME)

!IF EXISTS($(PACKAGER_PATH))
!ELSE
!  ERROR build\tools\package.mak error: '$(PACKAGER_PATH)' does not appear to exist. Config is '$(CONFIG)'. Is that correct?
!ENDIF

# In normal mode of operation, all we want to do is to ensure the inl file is up to date
all:
	@xcopy $(INTEROP)mplat\*.sxy        $(PACKAGE_DIR)mhost\MHost\			    				/i   /y    /d    /q
	@xcopy $(INTEROP)components\*.sxy   $(PACKAGE_DIR)mhost\MHost\Components\Interop\   		/i   /y    /d    /q
	@$(PACKAGER_PATH) $(PACKAGE_DIR)mhost $(CONTENT)packages\mhost_1000.sxyz
	@echo "The package.mak said 'MHOST Packaging complete'. What next?"