# Make file for maths module in the Rococo library

UNIT_OBJS = rococo.collisions.obj rococo.quad.tree.obj rococo.random.obj rococo.maths.obj

UNIT_SRCS = $(UNIT_OBJS,.obj=.cpp) 

OBJ_DIR = ../intermediate.mac/
UNIT_DIR = ../rococo.maths/
LIB_DIR = ../lib/

UNIT_OBJ_AND_DIRS = $(addprefix $(OBJ_DIR),$(UNIT_OBJS))

CPP_COMPILER = g++
CPP_FLAGS = @g++.config.txt
LIBGEN = ar
LIBGEN_FLAGS = cr

TARGET = $(LIB_DIR)rococo.maths.mac.lib
	
$(TARGET): $(UNIT_OBJ_AND_DIRS)
	$(LIBGEN) $(LIBGEN_FLAGS) $(TARGET) $(UNIT_OBJ_AND_DIRS)

$(OBJ_DIR)%.obj : $(addprefix $(UNIT_DIR),$(notdir %.cpp))
	$(CPP_COMPILER) $(CPP_FLAGS)  $< -o $@
	
clean:
	rm -f $(TARGET) $(UNIT_OBJ_AND_DIRS)

