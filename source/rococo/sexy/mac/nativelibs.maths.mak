# Make file for maths native lib in the Rococo.Sexy library

OBJ_DIR = ../../intermediate.mac/
MATHS_DIR = ../SS/sexy.nativelib.maths/
REFL_DIR = ../SS/sexy.nativelib.reflection/
BIN_DIR = ../bin/mac/
LIB_DIR = ../../lib/

CPP_COMPILER = g++
CPP_FLAGS = @g++.config.txt
DYNLIB_FLAGS = -dynamiclib -flat_namespace -stdlib=libc++ -mmacosx-version-min=10.8
	
$(BIN_DIR)sexy.nativelib.maths.mac.dylib: $(OBJ_DIR)sexy.nativelib.maths.obj $(LIB_DIR)rococo.util.mac.lib $(LIB_DIR)sexy.vm.mac.lib $(LIB_DIR)sexy.utilities.mac.lib $(LIB_DIR)sexy.compiler.mac.lib $(LIB_DIR)sexy.s-parser.mac.lib $(LIB_DIR)sexy.script.mac.lib
	$(CPP_COMPILER) $(DYNLIB_FLAGS) -g -o $(BIN_DIR)sexy.nativelib.maths.mac.dylib $(OBJ_DIR)sexy.nativelib.maths.obj $(LIB_DIR)rococo.util.mac.lib $(LIB_DIR)sexy.vm.mac.lib $(LIB_DIR)sexy.utilities.mac.lib $(LIB_DIR)sexy.compiler.mac.lib $(LIB_DIR)sexy.s-parser.mac.lib $(LIB_DIR)sexy.script.mac.lib
	
$(OBJ_DIR)sexy.nativelib.maths.obj : $(MATHS_DIR)sexy.nativelib.maths.cpp
	$(CPP_COMPILER) $(CPP_FLAGS) -c $< -o $@
	
clean:
	rm -f $(BIN_DIR)sexy.nativelib.maths.mac.dylib $(OBJ_DIR)sexy.nativelib.maths.obj
