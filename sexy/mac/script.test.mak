# Make file for Script test module in the Rococo.Sexy library

COMMON_DIR = ../Common/
ROCOCO_HEADERS = ../../include/
OBJ_DIR = ../../intermediate.mac/
SCRIOT_DIR = ../SS/sexy.script.test/
COMPILER_DIR = ../STC/stccore/
LIB_DIR = ../../lib/
STC_HDR = ../STC/stccore/
BIN_DIR = ../bin/mac/

CPP_COMPILER = g++
CPP_FLAGS = -g -std=c++14
	
$(BIN_DIR)sexy.script.test: $(LIB_DIR)sexy.vm.mac.lib $(LIB_DIR)sexy.utilities.mac.lib $(LIB_DIR)sexy.compiler.mac.lib $(LIB_DIR)sexy.s-parser.mac.lib $(LIB_DIR)sexy.script.mac.lib $(OBJ_DIR)sexy.script.test.obj
	$(CPP_COMPILER) -g -o $(BIN_DIR)sexy.script.test $(LIB_DIR)sexy.vm.mac.lib $(LIB_DIR)sexy.utilities.mac.lib $(LIB_DIR)sexy.s-parser.mac.lib $(LIB_DIR)sexy.compiler.mac.lib $(LIB_DIR)sexy.script.mac.lib $(OBJ_DIR)sexy.script.test.obj

$(OBJ_DIR)%.obj : $(addprefix $(SCRIOT_DIR),$(notdir %.cpp))
	$(CPP_COMPILER) $(CPP_FLAGS) -I$(COMMON_DIR) -I$(STC_HDR) -I$(ROCOCO_HEADERS) -c $< -o $@
	
clean:
	rm -f $(BIN_DIR)sexy.script.test $(OBJ_DIR)sexy.script.test.obj

