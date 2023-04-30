# Rococo.make makefile.mak Copyright(c) 2022. All rights reserved, Mark Anthony Taylor - mark.anthony.taylor@gmail.com. 
# This file is part of the Rococo libraries. It is open source and free to use and vary.

DIR_INCLUDE = $(ROCOCO)include^\
DIR_LIB = $(ROCOCO)lib^\
DIR_BIN = $(ROCOCO)bin^\
DIR_SEXY = $(ROCOCO)sexy^\
DIR_SEXY_BIN = $(ROCOCO)sexy\Bin^\
UTIL = $(ROCOCO)rococo.util^\

EXECUTE_POWERSHELL = powershell.exe -NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass

# DEBUGFLAG_SYMBOLS=-property:CPP_DEBUG_INFO_FORMAT=EditAndContinue
DEBUGFLAG_SYMBOLS=-property:CPP_DEBUG_INFO_FORMAT=ProgramDatabase

!IFDEF _DEBUG

CONFIGURATION = Debug
LCONFIGURATION = debug
LIB_UTIL = $(DIR_LIB)rococo.util.debug.lib
BENNY_HILL = $(DIR_SEXY_BIN)sexy.bennyhill.debug.exe

!ELSE

CONFIGURATION = Release
LCONFIGURATION = release
LIB_UTIL = $(DIR_LIB)rococo.util.lib
BENNY_HILL = $(DIR_SEXY_BIN)sexy.bennyhill.exe

!ENDIF

MSBUILD_CONFIG = -t:Build -p:Platform=x64 -p:Configuration=$(CONFIGURATION) $(DEBUGFLAG_SYMBOLS)
NATIVE_SRC = $(DIR_SEXY)NativeSource^\
EVENTS = $(DIR_BIN)rococo.events.$(LCONFIGURATION).dll
MSBUILD_CLEAN = -verbosity:minimal -t:Clean -p:Platform=x64 -p:Configuration=$(CONFIGURATION)
MSBUILD_TERSE = -verbosity:minimal $(MSBUILD_CONFIG)
MSBUILD_VERBOSE = -verbosity:normal $(MSBUILD_CONFIG)
MSBUILD_PARALLEL = -maxcpucount:8
WITH_SOLUTION = -property:SolutionDir=$(ROCOCO)sexy^\

all:
	msbuild $(UTIL)rococo.util.vcxproj                                                $(MSBUILD_TERSE) $(MSBUILD_PARALLEL)
	msbuild $(DIR_SEXY)Utilities\Utilities.vcxproj                                    $(MSBUILD_TERSE) $(MSBUILD_PARALLEL) $(WITH_SOLUTION)
	msbuild $(DIR_SEXY)SP\sexy.s-parser\sexy.s-parser.vcxproj                         $(MSBUILD_TERSE) $(MSBUILD_PARALLEL) $(WITH_SOLUTION)
	msbuild $(DIR_SEXY)SVM\svmcore\svmcore.vcxproj                                    $(MSBUILD_TERSE) $(MSBUILD_PARALLEL) $(WITH_SOLUTION)
	msbuild $(DIR_SEXY)s-parser.2/s-parser.2.vcxproj                                  $(MSBUILD_TERSE) $(MSBUILD_PARALLEL) $(WITH_SOLUTION)
	msbuild $(DIR_SEXY)sexy.bennyhill\sexy.bennyhill.vcxproj                          $(MSBUILD_TERSE) $(MSBUILD_PARALLEL) $(WITH_SOLUTION)
    msbuild $(DIR_SEXY)STC\stccore\stccore.vcxproj                                    $(MSBUILD_TERSE) $(MSBUILD_PARALLEL) $(WITH_SOLUTION)
	msbuild $(DIR_SEXY)SS\sexy.nativelib.maths\sexy.nativelib.maths.vcxproj           $(MSBUILD_TERSE) $(MSBUILD_PARALLEL) $(WITH_SOLUTION)
	msbuild $(DIR_SEXY)SS\sexy.nativelib.reflection\sexy.nativelib.reflection.vcxproj $(MSBUILD_TERSE) $(MSBUILD_PARALLEL) $(WITH_SOLUTION)
	msbuild $(DIR_SEXY)SS\sexy.nativelib.coroutines\sexy.nativelib.coroutines.vcxproj $(MSBUILD_TERSE) $(MSBUILD_PARALLEL) $(WITH_SOLUTION)
	msbuild $(DIR_SEXY)SS\sexy.script\sexy.script.vcxproj                             $(MSBUILD_TERSE) $(MSBUILD_PARALLEL) $(WITH_SOLUTION)
	msbuild $(DIR_SEXY)SS\sexy.script.test\sexy.script.test.vcxproj                   $(MSBUILD_TERSE) $(MSBUILD_PARALLEL) $(WITH_SOLUTION)
	msbuild $(DIR_SEXY)sexydotnethost\sexydotnethost.vcxproj                          $(MSBUILD_TERSE) $(MSBUILD_PARALLEL) $(WITH_SOLUTION)
	$(BENNY_HILL) $(DIR_SEXY)SS\sexy.nativelib.coroutines\ coroutines.sxh null

clean: 
	del $(BENNY_HILL)
	del /Q /S $(DIR_SEXY_BIN)*.*
	del /Q /S $(DIR_SEXY)NativeSource\*.lib
	del /Q /S $(DIR_SEXY)NativeSource\*.pdb
	del /Q /S $(DIR_SEXY)NativeSource\*.dll
	msbuild $(DIR_SEXY)Utilities\Utilities.vcxproj                                    $(MSBUILD_CLEAN) $(WITH_SOLUTION)
	msbuild $(DIR_SEXY)SP\sexy.s-parser\sexy.s-parser.vcxproj                         $(MSBUILD_CLEAN) $(WITH_SOLUTION)
	msbuild $(DIR_SEXY)SVM\svmcore\svmcore.vcxproj                                    $(MSBUILD_CLEAN) $(WITH_SOLUTION)
	msbuild $(DIR_SEXY)s-parser.2/s-parser.2.vcxproj                                  $(MSBUILD_CLEAN) $(WITH_SOLUTION)
	msbuild $(DIR_SEXY)sexy.bennyhill\sexy.bennyhill.vcxproj                          $(MSBUILD_CLEAN) $(WITH_SOLUTION)
	msbuild $(DIR_SEXY)STC\stccore\stccore.vcxproj                                    $(MSBUILD_CLEAN) $(WITH_SOLUTION)
	msbuild $(DIR_SEXY)SS\sexy.nativelib.maths\sexy.nativelib.maths.vcxproj           $(MSBUILD_CLEAN) $(WITH_SOLUTION)
	msbuild $(DIR_SEXY)SS\sexy.nativelib.reflection\sexy.nativelib.reflection.vcxproj $(MSBUILD_CLEAN) $(WITH_SOLUTION)
	msbuild $(DIR_SEXY)SS\sexy.nativelib.coroutines\sexy.nativelib.coroutines.vcxproj $(MSBUILD_CLEAN) $(WITH_SOLUTION)
	msbuild $(DIR_SEXY)SS\sexy.script\sexy.script.vcxproj                             $(MSBUILD_CLEAN) $(WITH_SOLUTION)
	msbuild $(DIR_SEXY)SS\sexy.script.test\sexy.script.test.vcxproj                   $(MSBUILD_CLEAN) $(WITH_SOLUTION)
	msbuild $(DIR_SEXY)sexydotnethost\sexydotnethost.vcxproj                          $(MSBUILD_CLEAN) $(WITH_SOLUTION)
