# Make file for Utilities module in the Rococo library

UNIT_OBJS = rococo.base.obj rococo.allocators.obj rococo.flow.obj rococo.observer.obj rococo.os.mac.obj rococo.postbox.obj rococo.s-parser.helpers.obj rococo.sprites.obj rococo.strings.obj rococo.texture.builder.obj rococo.ide.stdout.obj

UNIT_SRCS = $(UNIT_OBJS,.obj=.cpp) 

OBJ_DIR = ../intermediate.mac/
UNIT_DIR = ../rococo.util/
LIB_DIR = ../lib/

UNIT_SRC_AND_DIRS = $(addprefix $(UNIT_DIR),$(UNIT_SRCS))
UNIT_OBJ_AND_DIRS = $(addprefix $(OBJ_DIR),$(UNIT_OBJS))

CPP_COMPILER = g++
CPP_FLAGS = @g++.config.txt
LIBGEN = ar
LIBGEN_FLAGS = cr

FLOW_OBJ =  $(OBJ_DIR)sexy.ide.flow.objk
DIALOGS_OBJ = $(OBJ_DIR)rococo.dialogs.osx.mobj
	
$(LIB_DIR)rococo.util.mac.lib: $(UNIT_OBJ_AND_DIRS) $(FLOW_OBJ) $(DIALOGS_OBJ)
	$(LIBGEN) $(LIBGEN_FLAGS) $(LIB_DIR)rococo.util.mac.lib $(UNIT_OBJ_AND_DIRS) $(FLOW_OBJ) $(DIALOGS_OBJ)

$(OBJ_DIR)%.obj : $(addprefix $(UNIT_DIR),$(notdir %.cpp))
	$(CPP_COMPILER) $(CPP_FLAGS) $< -o $@
	
$(FLOW_OBJ): ../rococo.sexy.ide/sexy.ide.flow.cpp
	$(CPP_COMPILER) $(CPP_FLAGS) ../rococo.sexy.ide/sexy.ide.flow.cpp -o $(FLOW_OBJ)

$(DIALOGS_OBJ): $(UNIT_DIR)rococo.dialogs.osx.mm
	$(CPP_COMPILER) $(CPP_FLAGS) $(UNIT_DIR)rococo.dialogs.osx.mm -o $(DIALOGS_OBJ)

clean:
	rm -f $(LIB_DIR)rococo.util.mac.lib $(UNIT_OBJ_AND_DIRS) $(FLOW_OBJ) $(DIALOGS_OBJ)

