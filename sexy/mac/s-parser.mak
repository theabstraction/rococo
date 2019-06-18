# Make file for s-parser module in the Rococo.Sexy library

S_OBJS = sexy.s-parser.obj sexy.s-parser.s-block.obj
S_SRCS = $(S_OBJS,.obj=.cpp) 

OBJ_DIR = ../../intermediate.mac/
S_DIR = ../SP/sexy.s-parser/
LIB_DIR = ../../lib/

S_SRC_AND_DIRS = $(addprefix $(S_DIR),$(S_SRCS))
S_OBJ_AND_DIRS = $(addprefix $(OBJ_DIR),$(S_OBJS))

CPP_COMPILER = g++
CPP_FLAGS = @g++.config.txt
LIBGEN = ar
LIBGEN_FLAGS = cr
	
$(LIB_DIR)sexy.s-parser.mac.lib: $(S_OBJ_AND_DIRS)
	$(LIBGEN) $(LIBGEN_FLAGS) $(LIB_DIR)sexy.s-parser.mac.lib $(S_OBJ_AND_DIRS)

$(OBJ_DIR)%.obj : $(addprefix $(S_DIR),$(notdir %.cpp))
	$(CPP_COMPILER) $(CPP_FLAGS) -c $< -o $@

clean:
	rm -f $(LIB_DIR)sexy.s-parser.mac.lib $(S_OBJ_AND_DIRS)