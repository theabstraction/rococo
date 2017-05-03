# Make file for VM Test module in the Rococo.Sexy library

COMMON_DIR = ../Common/
ROCOCO_HEADERS = ../../include/
OBJ_DIR = ../../intermediate.mac/
S_DIR = ../SP/sexy.s-parser.test/
BIN_DIR = ../bin/mac/
LIB_DIR = ../../lib/

CPP_COMPILER = g++
CPP_FLAGS = -g -std=c++14
	
$(BIN_DIR)sexy.parser.test: $(LIB_DIR)sexy.s-parser.mac.lib $(LIB_DIR)sexy.utilities.mac.lib $(OBJ_DIR)sexy.s-parser.test.obj
	$(CPP_COMPILER) -g -o $(BIN_DIR)sexy.parser.test $(LIB_DIR)sexy.s-parser.mac.lib $(LIB_DIR)sexy.utilities.mac.lib $(OBJ_DIR)sexy.s-parser.test.obj

$(OBJ_DIR)%.obj : $(addprefix $(S_DIR),$(notdir %.cpp))
	$(CPP_COMPILER) $(CPP_FLAGS) -I$(COMMON_DIR) -I$(ROCOCO_HEADERS) -c $< -o $@
	
clean:
	rm -f $(BIN_DIR)sexy.parser.test $(OBJ_DIR)sexy.s-parser.test.obj
