# Rococo.make makefile.mak Copyright(c) 2022. All rights reserved, Mark Anthony Taylor - mark.anthony.taylor@gmail.com. 
# This file is part of the Rococo libraries. It is open source and free to use and vary.

DIR_INCLUDE = $(ROCOCO)include^\
DIR_LIB = $(ROCOCO)lib^\
DIR_BIN = $(ROCOCO)bin^\
DIR_SEXY = $(ROCOCO)sexy^\
DIR_MPLAT = $(ROCOCO)rococo.mplat^\
DIR_MHOST = $(ROCOCO)mhost^\
MPLAT_SXH_H = $(DIR_MPLAT)mplat.sxh.h
MPLAT_SXH = $(DIR_MPLAT)mplat.sxh
MPLAT_XC = $(DIR_MPLAT)config.xc
DIR_HV = $(ROCOCO)hyperverse^\
HV_SXH = $(DIR_HV)hv.sxh
HV_SXH_H = $(DIR_HV)hv.sxh.h
HV_XC = $(DIR_HV)config.xc
MPLAT_COMPONENTS_H = $(DIR_MPLAT)mplat.components.h
MPLAT_COMPONENTS_XML = $(DIR_MPLAT)components.xml
MHOST_SXH = $(DIR_MHOST)mhost.sxh
MHOST_SXH_H = $(DIR_MHOST)mhost.sxh.h
MHOST_XC = $(DIR_MHOST)mhost.xc

EXECUTE_POWERSHELL = powershell.exe -NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass

!IFDEF _DEBUG

DIR_BIN_SEXY = $(ROCOCO)sexy\Bin\x64Debug^\
BENNY_HILL = $(DIR_BIN_SEXY)sexy.bennyhill.exe
COMPILE_PREREQUISITES = $(ROCOCO)build.prerequisites.debug.bat
LIB_UTIL = $(DIR_LIB)rococo.util.debug.lib
CPP_MASTER = $(DIR_BIN)Debug\net6.0\rococo.cpp_master.exe

!ELSE

DIR_BIN_SEXY = $(ROCOCO)sexy\Bin\x64Release^\
BENNY_HILL = $(DIR_BIN_SEXY)sexy.bennyhill.exe
COMPILE_PREREQUISITES = $(ROCOCO)build.prerequisites.release.bat
LIB_UTIL = $(DIR_LIB)rococo.util.lib
CPP_MASTER = $(DIR_BIN)Release\net6.0\rococo.cpp_master.exe

!ENDIF

all: $(BENNY_HILL) $(MPLAT_SXH_H) $(HV_SXH_H) $(MHOST_SXH_H) $(MPLAT_COMPONENTS_H)

$(LIB_UTIL): $(ROCOCO)rococo.util/rococo.base.cpp
	CALL $(COMPILE_PREREQUISITES)

$(BENNY_HILL): $(LIB_UTIL)
	CALL $(COMPILE_PREREQUISITES)

$(MPLAT_SXH_H): $(MPLAT_SXH) $(MPLAT_XC)
	$(BENNY_HILL) $(DIR_MPLAT) mplat.sxh null

$(HV_SXH_H): $(HV_SXH) $(HV_XC)
	$(BENNY_HILL) $(DIR_HV) hv.sxh null

$(MHOST_SXH_H): $(MHOST_SXH) $(MHOST_XC)
	$(BENNY_HILL) $(DIR_MHOST) mhost.sxh null

$(MPLAT_COMPONENTS_H): $(MPLAT_COMPONENTS_XML)
	$(CPP_MASTER) $(MPLAT_COMPONENTS_XML) $(ROCOCO)


