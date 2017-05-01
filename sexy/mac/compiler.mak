# Make file for VM module in the Rococo.Sexy library

COMPILER_OBJS = sexy.codebuilder.obj sexy.compiler.obj sexy.compiler.os.obj sexy.compiler.stdafx.obj
COMPILER_SRCS = $(COMPILER_OBJS,.obj=.cpp) 

COMMON_DIR = ../Common/
ROCOCO_HEADERS = ../../include/
OBJ_DIR = ../../intermediate.mac/
COMPILER_DIR = ../STC/stccore/
LIB_DIR = ../../lib/

COMPILER_SRC_AND_DIRS = $(addprefix $(COMPILER_DIR),$(COMPILER_SRCS))
COMPILER_OBJ_AND_DIRS = $(addprefix $(OBJ_DIR),$(COMPILER_OBJS))

CPP_COMPILER = g++
CPP_FLAGS = -g -std=c++14
LIBGEN = ar
LIBGEN_FLAGS = cr
	
$(LIB_DIR)sexy.compiler.mac.lib: $(COMPILER_OBJ_AND_DIRS)
	$(LIBGEN) $(LIBGEN_FLAGS) $(LIB_DIR)sexy.compiler.mac.lib $(COMPILER_OBJ_AND_DIRS)

$(OBJ_DIR)%.obj : $(addprefix $(COMPILER_DIR),$(notdir %.cpp))
	$(CPP_COMPILER) $(CPP_FLAGS) -I$(COMMON_DIR) -I$(ROCOCO_HEADERS) -c $< -o $@

clean:
	rm -f $(LIB_DIR)sexy.compiler.mac.lib $(COMPILER_OBJ_AND_DIRS)