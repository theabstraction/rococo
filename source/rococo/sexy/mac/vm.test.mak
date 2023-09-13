# Make file for VM Test module in the Rococo.Sexy library

OBJ_DIR = ../../intermediate.mac/
VM_DIR = ../svm/svmtest/
BIN_DIR = ../bin/mac/
LIB_DIR = ../../lib/

CPP_COMPILER = g++
CPP_FLAGS = @g++.config.txt
	
$(BIN_DIR)sexy.vm.test: $(LIB_DIR)sexy.vm.mac.lib $(LIB_DIR)sexy.utilities.mac.lib $(OBJ_DIR)sexy.vm.test.obj
	$(CPP_COMPILER) -g -o $(BIN_DIR)sexy.vm.test $(LIB_DIR)sexy.vm.mac.lib $(LIB_DIR)sexy.utilities.mac.lib $(OBJ_DIR)sexy.vm.test.obj  $(LIB_DIR)rococo.util.mac.lib

$(OBJ_DIR)%.obj : $(addprefix $(VM_DIR),$(notdir %.cpp))
	$(CPP_COMPILER) $(CPP_FLAGS) -c $< -o $@
	
clean:
	rm -f $(BIN_DIR)sexy.vm.test $(OBJ_DIR)sexy.vm.test.obj
