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

MHOST_PACKAGE_DIR = $(PACKAGE_DIR)mhost\MHost^\

$(MHOST_PACKAGE_DIR)mplat_sxh.sxy: $(INTEROP)mplat\mplat_sxh.sxy
	copy $(INTEROP)mplat\mplat_sxh.sxy  $(MHOST_PACKAGE_DIR)

$(MHOST_PACKAGE_DIR)mplat_config_sxh.sxy: $(INTEROP)mplat\mplat_config_sxh.sxy
	copy $(INTEROP)mplat\mplat_config_sxh.sxy  $(MHOST_PACKAGE_DIR)

$(MHOST_PACKAGE_DIR)mplat_gui_sxh.sxy: $(INTEROP)mplat\mplat_gui_sxh.sxy
	copy $(INTEROP)mplat\mplat_gui_sxh.sxy  $(MHOST_PACKAGE_DIR)
	
$(MHOST_PACKAGE_DIR)mplat_types.sxy: $(INTEROP)mplat\mplat_types.sxy
	copy $(INTEROP)mplat\mplat_types.sxy  $(MHOST_PACKAGE_DIR)
	
$(MHOST_PACKAGE_DIR)types.sxy: $(INTEROP)mplat\types.sxy
	copy $(INTEROP)mplat\types.sxy  $(MHOST_PACKAGE_DIR)
	
$(MHOST_PACKAGE_DIR)rococo.audio_sxh.sxy: $(INTEROP)audio\rococo.audio_sxh.sxy
	copy $(INTEROP)audio\rococo.audio_sxh.sxy  $(MHOST_PACKAGE_DIR)
	
$(MHOST_PACKAGE_DIR)audio_types.sxy: $(INTEROP)audio\audio_types.sxy
	copy $(INTEROP)audio\audio_types.sxy  $(MHOST_PACKAGE_DIR)
	

# In normal mode of operation, all we want to do is to ensure the inl file is up to date
all: $(MHOST) $(MHOST_PACKAGE_DIR)mplat_sxh.sxy $(MHOST_PACKAGE_DIR)mplat_types.sxy $(MHOST_PACKAGE_DIR)types.sxy $(MHOST_PACKAGE_DIR)rococo.audio_sxh.sxy $(MHOST_PACKAGE_DIR)audio_types.sxy 
    $(PACKAGER_PATH) $(PACKAGE_DIR)mhost $(CONTENT)packages\mhost_1000.sxyz

clean:
	