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
DIR_EVENTS = $(ROCOCO)rococo.events^\
UTIL = $(ROCOCO)rococo.util^\

EXECUTE_POWERSHELL = powershell.exe -NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass

!IFDEF _DEBUG

CONFIGURATION = Debug
LCONFIGURATION = debug
LIB_UTIL = $(DIR_LIB)rococo.util.debug.lib
EVENTS_DLL = $(DIR_BIN)rococo.events.debug.dll
SEXY_CMD =  $(DIR_BIN)rococo.sexy.cmd.debug.exe

!ELSE

CONFIGURATION = Release
LCONFIGURATION = release
LIB_UTIL = $(DIR_LIB)rococo.util.lib
EVENTS_DLL = $(DIR_BIN)rococo.events.dll
SEXY_CMD =  $(DIR_BIN)rococo.sexy.cmd.exe

!ENDIF

COMPILE_PREREQUISITES = $(ROCOCO)build.prerequisites.$(LCONFIGURATION).bat
DIR_BIN_SEXY = $(ROCOCO)sexy\Bin\x64$(CONFIGURATION)^\
BENNY_HILL = $(DIR_BIN_SEXY)sexy.bennyhill.exe
CPP_MASTER = $(DIR_BIN)tools\x64\$(CONFIGURATION)\net6.0\rococo.cpp_master.exe
NATIVE_SRC = $(DIR_SEXY)NativeSource^\

all: $(BENNY_HILL) $(MPLAT_SXH_H) $(HV_SXH_H) $(MHOST_SXH_H) $(CPP_MASTER) $(MPLAT_COMPONENTS_H)
	$(ROCOCO)copy.natives.from.sexy.bat > NUL
	copy $(NATIVE_SRC)*.sxy $(DIR_EVENTS)content\scripts\native > NUL
	copy $(NATIVE_SRC)*.sxy $(ROCOCO)content\scripts\native > NUL
	$(SEXY_CMD) natives=$(NATIVE_SRC) installation=$(DIR_EVENTS)content\ root=$(DIR_EVENTS) run=!scripts/gen.events.hv.sxy 

clean: 
	del $(BENNY_HILL)
	del $(LIB_UTIL)

$(LIB_UTIL): $(ROCOCO)rococo.util/rococo.base.cpp $(UTIL)rococo.base.cpp $(UTIL)rococo.throw.cr_sex.cpp
	msbuild $(ROCOCO)rococo.util/rococo.util.vcxproj -p:Configuration=$(CONFIGURATION) -t:Build -p:Platform=x64 -m -verbosity:minimal

$(BENNY_HILL): $(LIB_UTIL)
	$(COMPILE_PREREQUISITES)

$(MPLAT_SXH_H): $(MPLAT_SXH) $(MPLAT_XC)
	$(BENNY_HILL) $(DIR_MPLAT) mplat.sxh null

$(HV_SXH_H): $(HV_SXH) $(HV_XC)
	$(BENNY_HILL) $(DIR_HV) hv.sxh null

$(MHOST_SXH_H): $(MHOST_SXH) $(MHOST_XC)
	$(BENNY_HILL) $(DIR_MHOST) mhost.sxh null

$(MPLAT_COMPONENTS_H): $(MPLAT_COMPONENTS_XML) $(DIR_MPLAT)mplat.component.template.cpp $(DIR_MPLAT)mplat.component.template.h
	$(CPP_MASTER) $(MPLAT_COMPONENTS_XML) $(ROCOCO)

$(CPP_MASTER): $(ROCOCO)rococo.cpp_master\rococo.cpp_master.main.cs $(ROCOCO)rococo.cpp_master\rococo.cpp_master.component.cs
	msbuild $(ROCOCO)rococo.cpp_master\rococo.cpp_master.csproj -p:Configuration=$(CONFIGURATION) -t:Build -p:Platform=x64 -m -verbosity:minimal


	