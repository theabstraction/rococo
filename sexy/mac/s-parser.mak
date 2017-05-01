# Make file for Utilities module in the Rococo.Sexy library

S_OBJS = sexy.s-parser.obj
S_SRCS = $(S_OBJS,.obj=.cpp) 

COMMON_DIR = ../Common/
ROCOCO_HEADERS = ../../include/
OBJ_DIR = ../../intermediate.mac/
S_DIR = ../SP/sexy.s-parser/
LIB_DIR = ../../lib/

S_SRC_AND_DIRS = $(addprefix $(S_DIR),$(S_SRCS))
S_OBJ_AND_DIRS = $(addprefix $(OBJ_DIR),$(S_OBJS))

CPP_COMPILER = g++
CPP_FLAGS = -g -std=c++14
LIBGEN = ar
LIBGEN_FLAGS = cr
	
$(LIB_DIR)sexy.s-parser.mac.lib: $(S_OBJ_AND_DIRS)
	$(LIBGEN) $(LIBGEN_FLAGS) $(LIB_DIR)sexy.s-parser.mac.lib $(S_OBJ_AND_DIRS)

$(OBJ_DIR)%.obj : $(addprefix $(S_DIR),$(notdir %.cpp))
	$(CPP_COMPILER) $(CPP_FLAGS) -I$(COMMON_DIR) -I$(ROCOCO_HEADERS) -c $< -o $@

clean:
	rm -f $(LIB_DIR)sexy.s-parser.mac.lib $(S_OBJ_AND_DIRS)