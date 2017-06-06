# Make file for the font module in the Rococo library

UNIT_OBJS = fonts.obj

UNIT_SRCS = $(UNIT_OBJS,.obj=.cpp) 

ROCOCO_HEADERS = ../include/
OBJ_DIR = ../intermediate.mac/
UNIT_DIR = ../rococo.fonts/
LIB_DIR = ../lib/
SEXY_DIR = ../sexy/Common/

UNIT_SRC_AND_DIRS = $(addprefix $(UNIT_DIR),$(UNIT_SRCS))
UNIT_OBJ_AND_DIRS = $(addprefix $(OBJ_DIR),$(UNIT_OBJS))

CPP_COMPILER = g++
CPP_FLAGS = -g -c -std=c++11
LIBGEN = ar
LIBGEN_FLAGS = cr
	
$(LIB_DIR)rococo.fonts.mac.lib: $(UNIT_OBJ_AND_DIRS)
	$(LIBGEN) $(LIBGEN_FLAGS) $(LIB_DIR)rococo.fonts.mac.lib $(UNIT_OBJ_AND_DIRS)

$(OBJ_DIR)%.obj : $(addprefix $(UNIT_DIR),$(notdir %.cpp))
	$(CPP_COMPILER) $(CPP_FLAGS) -I$(ROCOCO_HEADERS) -I$(SEXY_DIR) $< -o $@
	
clean:
	rm -f $(LIB_DIR)rococo.fonts.mac.lib $(UNIT_OBJ_AND_DIRS)

