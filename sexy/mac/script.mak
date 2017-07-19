# Make file for VM module in the Rococo.Sexy library

SCRIPT_OBJS = sexy.script.obj sexy.script.expression.parser.obj sexy.script.helpers.obj sexy.script.os.mac.obj
SCRIPT_SRCS = $(SCRIPT_OBJS,.obj=.cpp) 

OBJ_DIR = ../../intermediate.mac/
SCRIOT_DIR = ../SS/sexy.script/
COMPILER_DIR = ../STC/stccore/
LIB_DIR = ../../lib/

SCRIPT_SRC_AND_DIRS = $(addprefix $(SCRIOT_DIR),$(SCRIPT_SRCS))
SCRIPT_OBJ_AND_DIRS = $(addprefix $(OBJ_DIR),$(SCRIPT_OBJS))

CPP_COMPILER = g++
CPP_FLAGS = @g++.config.txt
LIBGEN = ar
LIBGEN_FLAGS = cr
	
$(LIB_DIR)sexy.script.mac.lib: $(SCRIPT_OBJ_AND_DIRS)
	$(LIBGEN) $(LIBGEN_FLAGS) $(LIB_DIR)sexy.script.mac.lib $(SCRIPT_OBJ_AND_DIRS)

$(OBJ_DIR)%.obj : $(addprefix $(SCRIOT_DIR),$(notdir %.cpp))
	$(CPP_COMPILER) $(CPP_FLAGS) -I$(COMPILER_DIR) -c $< -o $@

clean:
	rm -f $(SCRIPT_OBJ_AND_DIRS) $(LIB_DIR)sexy.script.mac.lib
