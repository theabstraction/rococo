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
BIN64 = $(ROCOCO)gen\bin\win64^\
BIN = $(BIN64)$(lowercase $(CONFIG))^\
PACKAGER_EXE_FILENAME = rococo.packager.exe
PACKAGER_PATH = $(BIN)$(PACKAGER_EXE_FILENAME)

!IF EXISTS($(PACKAGER_PATH))
!ELSE
!  ERROR build\tools\package.mak error: '$(PACKAGER_PATH)' does not appear to exist. Config is '$(CONFIG)'. Is that correct?
!ENDIF

MHOST_PACKAGE_DIR = $(PACKAGE_DIR)mhost\MHost^\

$(MHOST_PACKAGE_DIR)mplat_sxh.sxy: $(SCRIPTS)mplat_sxh.sxy
	copy $(SCRIPTS)mplat_sxh.sxy  $(MHOST_PACKAGE_DIR)
	
$(MHOST_PACKAGE_DIR)mplat_types.sxy: $(SCRIPTS)mplat_types.sxy
	copy $(SCRIPTS)mplat_types.sxy  $(MHOST_PACKAGE_DIR)
	
$(MHOST_PACKAGE_DIR)types.sxy: $(SCRIPTS)types.sxy
	copy $(SCRIPTS)types.sxy  $(MHOST_PACKAGE_DIR)
	
$(MHOST_PACKAGE_DIR)rococo.audio_sxh.sxy: $(SCRIPTS)rococo.audio_sxh.sxy
	copy $(SCRIPTS)rococo.audio_sxh.sxy  $(MHOST_PACKAGE_DIR)
	
$(MHOST_PACKAGE_DIR)audio_types.sxy: $(SCRIPTS)audio_types.sxy
	copy $(SCRIPTS)audio_types.sxy  $(MHOST_PACKAGE_DIR)
	

# In normal mode of operation, all we want to do is to ensure the inl file is up to date
all: $(MHOST) $(MHOST_PACKAGE_DIR)mplat_sxh.sxy $(MHOST_PACKAGE_DIR)mplat_types.sxy $(MHOST_PACKAGE_DIR)types.sxy $(MHOST_PACKAGE_DIR)rococo.audio_sxh.sxy $(MHOST_PACKAGE_DIR)audio_types.sxy 
    $(PACKAGER_PATH) $(PACKAGE_DIR)mhost $(CONTENT)packages\mhost_1000.sxyz
