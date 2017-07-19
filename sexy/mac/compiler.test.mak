# Make file for Compiler Test module in the Rococo.Sexy library

OBJ_DIR = ../../intermediate.mac/
STC_DIR = ../STC/stctest/
STC_HDR = ../STC/stccore/
BIN_DIR = ../bin/mac/
LIB_DIR = ../../lib/

CPP_COMPILER = g++
CPP_FLAGS = @g++.config.txt
	
$(BIN_DIR)sexy.compiler.test: $(LIB_DIR)sexy.compiler.mac.lib $(LIB_DIR)sexy.utilities.mac.lib $(LIB_DIR)sexy.vm.mac.lib $(OBJ_DIR)sexy.compiler.test.obj
	$(CPP_COMPILER) -g -o $(BIN_DIR)sexy.compiler.test $(LIB_DIR)rococo.util.mac.lib $(LIB_DIR)sexy.compiler.mac.lib $(LIB_DIR)sexy.utilities.mac.lib $(OBJ_DIR)sexy.compiler.test.obj $(LIB_DIR)sexy.vm.mac.lib 

$(OBJ_DIR)%.obj : $(addprefix $(STC_DIR),$(notdir %.cpp))
	$(CPP_COMPILER) $(CPP_FLAGS) -I$(STC_HDR) -c $< -o $@
	
clean:
	rm -f $(BIN_DIR)sexy.compiler.test $(OBJ_DIR)sexy.compiler.test.obj
