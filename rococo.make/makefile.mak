# Rococo.make makefile.mak Copyright(c) 2022. All rights reserved, Mark Anthony Taylor - mark.anthony.taylor@gmail.com. 
# This file is part of the Rococo libraries. It is open source and free to use and vary.

DIR_INCLUDE = $(ROCOCO)include^\
DIR_LIB = $(ROCOCO)lib^\
DIR_BIN = $(ROCOCO)bin^\
DIR_SEXY = $(ROCOCO)sexy^\
DIR_SEXY_BIN = $(ROCOCO)sexy\Bin^\
DIR_MPLAT = $(ROCOCO)rococo.mplat^\
DIR_MPLAT_DYN = $(ROCOCO)rococo.mplat.dynamic^\
DIR_MHOST = $(ROCOCO)mhost^\
DIR_GUI_RETAINED = $(ROCOCO)rococo.gui.retained^\
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
MHOST_PACKAGE = $(ROCOCO)content\packages\mhost_1000.sxyz
MHOST_XC = $(DIR_MHOST)mhost.xc
DIR_EVENTS = $(ROCOCO)rococo.events^\
UTIL = $(ROCOCO)rococo.util^\
DEBUGFLAG_EDIT_AND_CONTINUE=-property:CPP_DEBUG_INFO_FORMAT=EditAndContinue
DEBUGFLAG_PROGRAM_DATABASE=-property:CPP_DEBUG_INFO_FORMAT=ProgramDatabase

EXECUTE_POWERSHELL = powershell.exe -NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass

!IFDEF _DEBUG

MSBUILD_CONFIG = -t:Build -p:Platform=x64 -p:Configuration=$(CONFIGURATION) $(DEBUGFLAG_EDIT_AND_CONTINUE)
BENNY_HILL = $(DIR_SEXY_BIN)sexy.bennyhill.debug.exe
CONFIGURATION = Debug
LCONFIGURATION = debug
LIB_UTIL = $(DIR_LIB)rococo.util.debug.lib
EVENTS_DLL = $(DIR_BIN)rococo.events.debug.dll
SEXY_CMD =  $(DIR_BIN)rococo.sexy.cmd.debug.exe
MHOST = $(DIR_BIN)rococo.mhost.debug.exe
SEXYSTUDIO = $(DIR_BIN)sexystudio.Debug.dll

!ELSE

MSBUILD_CONFIG = -t:Build -p:Platform=x64 -p:Configuration=$(CONFIGURATION) $(DEBUGFLAG_PROGRAM_DATABASE)
BENNY_HILL = $(DIR_SEXY_BIN)sexy.bennyhill.exe
CONFIGURATION = Release
LCONFIGURATION = release
LIB_UTIL = $(DIR_LIB)rococo.util.lib
EVENTS_DLL = $(DIR_BIN)rococo.events.dll
SEXY_CMD =  $(DIR_BIN)rococo.sexy.cmd.exe
MHOST = $(DIR_BIN)rococo.mhost.exe
SEXYSTUDIO = $(DIR_BIN)sexystudio.dll

!ENDIF

CPP_MASTER = $(DIR_BIN)tools\x64\$(CONFIGURATION)\net6.0\rococo.cpp_master.exe
NATIVE_SRC = $(DIR_SEXY)NativeSource^\
EVENTS = $(DIR_BIN)rococo.events.$(LCONFIGURATION).dll
MSBUILD_TERSE = -verbosity:minimal $(MSBUILD_CONFIG)
MSBUILD_VERBOSE = -verbosity:normal $(MSBUILD_CONFIG)
MSBUILD_PARALLEL = -maxcpucount:4
WITH_SOLUTION = -property:SolutionDir=$(ROCOCO)
CONTENT_SYS_TYPE = $(ROCOCO)content\scripts\native\Sys.Type.sxy
EVENTS_SYS_TYPE = $(DIR_EVENTS)content\scripts\native\Sys.Type.sxy

all: $(SEXY_CMD) $(CPP_MASTER)_build $(CONTENT_SYS_TYPE) $(EVENTS_SYS_TYPE) $(MPLAT_SXH_H) $(HV_SXH_H) $(MHOST_SXH_H) $(MPLAT_COMPONENTS_H) $(EVENTS) $(MHOST_PACKAGE) $(MHOST)_build $(SEXYSTUDIO)_build

clean: 
	del $(BENNY_HILL)
	del $(LIB_UTIL)
	del /Q $(DIR_LIB)*.*
	del /Q /S $(DIR_BIN)*.*
	del /Q /S $(DIR_SEXY)NativeSource\*.lib
	del /Q /S $(DIR_SEXY)NativeSource\*.pdb
	del /Q /S $(DIR_SEXY)NativeSource\*.dll

$(SEXY_CMD):
	msbuild $(DIR_SEXY)sexy.sln                                   $(MSBUILD_TERSE) $(MSBUILD_PARALLEL)
	msbuild $(ROCOCO)rococo.3rd-party.sln                         $(MSBUILD_TERSE) $(MSBUILD_PARALLEL)
	msbuild $(ROCOCO)rococo.maths\rococo.maths.vcxproj            $(MSBUILD_TERSE) $(MSBUILD_PARALLEL)
	msbuild $(ROCOCO)rococo.util.ex\rococo.util.ex.vcxproj        $(MSBUILD_TERSE) $(MSBUILD_PARALLEL)
	msbuild $(ROCOCO)rococo.packager\rococo.packager.vcxproj      $(MSBUILD_TERSE) $(MSBUILD_PARALLEL)
	msbuild $(ROCOCO)rococo.misc.utils\rococo.misc.utils.vcxproj  $(MSBUILD_TERSE) $(MSBUILD_PARALLEL)
	msbuild $(ROCOCO)rococo.windows\rococo.windows.vcxproj        $(MSBUILD_TERSE) $(MSBUILD_PARALLEL)
	msbuild $(ROCOCO)rococo.sexy.ide\rococo.sexy.ide.vcxproj      $(MSBUILD_TERSE) $(MSBUILD_PARALLEL)
	msbuild $(ROCOCO)rococo.sexy.cmd\rococo.sexy.cmd.vcxproj      $(MSBUILD_TERSE) $(MSBUILD_PARALLEL)

$(CONTENT_SYS_TYPE):
	copy $(NATIVE_SRC)*.sxy $(ROCOCO)content\scripts\native > NUL

$(EVENTS_SYS_TYPE):
	copy $(NATIVE_SRC)*.sxy $(ROCOCO)content\scripts\native > NUL

$(CPP_MASTER)_build: $(CPP_MASTER)
	$(ROCOCO)copy.natives.from.sexy.bat > NUL
	$(DIR_BIN)tools\x64\$(CONFIGURATION)\net6.0\rococo.cpp_master.exe $(ROCOCO)rococo.components.test\test.xml $(ROCOCO)

$(EVENTS): $(DIR_EVENTS)content\scripts\gen.events.hv.sxy $(DIR_EVENTS)content\scripts\gen.events.sxy
	$(SEXY_CMD) natives=$(NATIVE_SRC) installation=$(DIR_EVENTS)content\ root=$(DIR_EVENTS) run=!scripts/gen.events.hv.sxy 

$(MPLAT_SXH_H): $(MPLAT_SXH) $(MPLAT_XC)
	$(BENNY_HILL) $(DIR_MPLAT) mplat.sxh null

$(HV_SXH_H): $(HV_SXH) $(HV_XC)
	$(BENNY_HILL) $(DIR_HV) hv.sxh null

$(MHOST_SXH_H): $(MHOST_SXH) $(MHOST_XC)
	$(BENNY_HILL) $(DIR_MHOST) mhost.sxh null

$(MPLAT_COMPONENTS_H): $(MPLAT_COMPONENTS_XML) $(DIR_MPLAT)mplat.component.template.cpp $(DIR_MPLAT)mplat.component.template.h
	$(CPP_MASTER) $(MPLAT_COMPONENTS_XML) $(ROCOCO)

$(CPP_MASTER): $(ROCOCO)rococo.cpp_master\rococo.cpp_master.main.cs $(ROCOCO)rococo.cpp_master\rococo.cpp_master.component.cs
	msbuild $(ROCOCO)rococo.cpp_master\rococo.cpp_master.csproj $(MSBUILD_TERSE) $(MSBUILD_PARALLEL)

$(MHOST_PACKAGE): $(MPLAT_SXH) $(MPLAT_XC) $(MHOST_SXH) $(MHOST_XC)
	$(ROCOCO)packages\gen.mhost.package.bat

$(MHOST)_build:
	msbuild $(ROCOCO)rococo.sexy.mathsex\rococo.sexy.mathsex.vcxproj $(MSBUILD_TERSE) $(MSBUILD_PARALLEL) $(WITH_SOLUTION)
	msbuild $(ROCOCO)rococo.fonts\fonts.vcxproj                      $(MSBUILD_TERSE) $(MSBUILD_PARALLEL) $(WITH_SOLUTION)
	msbuild $(ROCOCO)dx11.renderer\dx11.renderer.vcxproj             $(MSBUILD_TERSE) $(MSBUILD_PARALLEL) $(WITH_SOLUTION)
	msbuild $(ROCOCO)rococo.file.browser\rococo.file.browser.vcxproj $(MSBUILD_TERSE) $(MSBUILD_PARALLEL) $(WITH_SOLUTION)
	msbuild $(DIR_GUI_RETAINED)rococo.gui.retained.vcxproj           $(MSBUILD_TERSE) $(MSBUILD_PARALLEL) $(WITH_SOLUTION)
	msbuild $(DIR_MPLAT)rococo.mplat.vcxproj                         $(MSBUILD_TERSE) $(MSBUILD_PARALLEL) $(WITH_SOLUTION)
	msbuild $(DIR_MPLAT_DYN)rococo.mplat.dynamic.vcxproj             $(MSBUILD_TERSE) $(MSBUILD_PARALLEL) $(WITH_SOLUTION)	
	msbuild $(DIR_MHOST)mhost.vcxproj                                $(MSBUILD_TERSE) $(MSBUILD_PARALLEL) $(WITH_SOLUTION)

$(SEXYSTUDIO)_build:
	msbuild $(ROCOCO)sexystudio/sexystudio.vcxproj                           $(MSBUILD_TERSE) $(MSBUILD_PARALLEL) $(WITH_SOLUTION)
	msbuild $(ROCOCO)sexystudio.app/sexystudio.app.vcxproj                   $(MSBUILD_TERSE) $(MSBUILD_PARALLEL) $(WITH_SOLUTION)
	msbuild $(ROCOCO)sexystudio.test/sexystudio.test.vcxproj                 $(MSBUILD_TERSE) $(MSBUILD_PARALLEL) $(WITH_SOLUTION)
	msbuild $(ROCOCO)sexystudio.4.NPP/vs.proj/sexystudio.4.Notepad++.vcxproj $(MSBUILD_TERSE) $(MSBUILD_PARALLEL) $(WITH_SOLUTION)
