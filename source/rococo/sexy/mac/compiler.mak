# Make file for compiler module in the Rococo.Sexy library

COMPILER_OBJS = sexy.codebuilder.obj sexy.compiler.obj sexy.compiler.os.obj
COMPILER_SRCS = $(COMPILER_OBJS,.obj=.cpp) 

OBJ_DIR = ../../intermediate.mac/
COMPILER_DIR = ../STC/stccore/
LIB_DIR = ../../lib/

COMPILER_SRC_AND_DIRS = $(addprefix $(COMPILER_DIR),$(COMPILER_SRCS))
COMPILER_OBJ_AND_DIRS = $(addprefix $(OBJ_DIR),$(COMPILER_OBJS))

CPP_COMPILER = g++
CPP_FLAGS = @g++.config.txt
LIBGEN = ar
LIBGEN_FLAGS = cr
	
$(LIB_DIR)sexy.compiler.mac.lib: $(COMPILER_OBJ_AND_DIRS)
	$(LIBGEN) $(LIBGEN_FLAGS) $(LIB_DIR)sexy.compiler.mac.lib $(COMPILER_OBJ_AND_DIRS)

$(OBJ_DIR)%.obj : $(addprefix $(COMPILER_DIR),$(notdir %.cpp))
	$(CPP_COMPILER) $(CPP_FLAGS) -c $< -o $@

clean:
	rm -f $(LIB_DIR)sexy.compiler.mac.lib $(COMPILER_OBJ_AND_DIRS)