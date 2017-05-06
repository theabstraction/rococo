# Make file for maths native lib in the Rococo.Sexy library

COMMON_DIR = ../Common/
ROCOCO_HEADERS = ../../include/
OBJ_DIR = ../../intermediate.mac/
MATHS_DIR = ../SS/sexy.nativelib.maths/
REFL_DIR = ../SS/sexy.nativelib.reflection/
BIN_DIR = ../bin/mac/
LIB_DIR = ../../lib/

CPP_COMPILER = g++
CPP_FLAGS = -g -std=c++14
DYNLIB_FLAGS = -dynamiclib -undefined suppress -flat_namespace
	
$(BIN_DIR)sexy.nativelib.maths.mac.dylib: $(OBJ_DIR)sexy.nativelib.maths.obj
	$(CPP_COMPILER) $(DYNLIB_FLAGS) -g -o $(BIN_DIR)sexy.nativelib.maths.mac.dylib $(OBJ_DIR)sexy.nativelib.maths.obj
	
$(OBJ_DIR)sexy.nativelib.maths.obj : $(MATHS_DIR)sexy.nativelib.maths.cpp
	$(CPP_COMPILER) $(CPP_FLAGS) -I$(COMMON_DIR) -I$(ROCOCO_HEADERS) -c $< -o $@
	
clean:
	rm -f $(BIN_DIR)sexy.nativelib.maths.mac.dylib $(OBJ_DIR)sexy.nativelib.maths.obj
