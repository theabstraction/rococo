# Make file for reflection native lib in the Rococo.Sexy library

OBJ_DIR = ../../intermediate.mac/
MATHS_DIR = ../SS/sexy.nativelib.maths/
REFL_DIR = ../SS/sexy.nativelib.coroutines/
BIN_DIR = ../bin/mac/
LIB_DIR = ../../lib/

CPP_COMPILER = g++
CPP_FLAGS = @g++.config.txt -I../STC/stccore/
DYNLIB_FLAGS = -dynamiclib -stdlib=libc++ -mmacosx-version-min=10.8 -flat_namespace
	
$(BIN_DIR)sexy.nativelib.coroutines.mac.dylib: $(OBJ_DIR)sexy.nativelib.coroutine.obj  $(LIB_DIR)rococo.util.mac.lib $(LIB_DIR)sexy.vm.mac.lib $(LIB_DIR)sexy.utilities.mac.lib $(LIB_DIR)sexy.compiler.mac.lib $(LIB_DIR)sexy.s-parser.mac.lib $(LIB_DIR)sexy.script.mac.lib
	$(CPP_COMPILER) $(DYNLIB_FLAGS) -g -o $(BIN_DIR)sexy.nativelib.coroutines.mac.dylib $(OBJ_DIR)sexy.nativelib.coroutine.obj $(LIB_DIR)rococo.util.mac.lib $(LIB_DIR)sexy.vm.mac.lib $(LIB_DIR)sexy.utilities.mac.lib $(LIB_DIR)sexy.compiler.mac.lib $(LIB_DIR)sexy.s-parser.mac.lib $(LIB_DIR)sexy.script.mac.lib

$(OBJ_DIR)sexy.nativelib.coroutine.obj : $(REFL_DIR)sexy.nativelib.coroutine.cpp
	$(CPP_COMPILER) $(CPP_FLAGS) -c $< -o $@
	
clean:
	rm -f $(BIN_DIR)sexy.nativelib.coroutines.mac.dylib $(OBJ_DIR)sexy.nativelib.coroutine.obj
