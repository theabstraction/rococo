# Make file for VM module in the Rococo.Sexy library

VM_OBJS = sexy.vm.assembler.obj sexy.vm.core.obj sexy.vm.obj sexy.vm.disassembler.obj sexy.vm.main.obj sexy.vm.os.mac.obj sexy.vm.program.obj\
        sexy.vm.stdafx.obj sexy.vm.symbols.obj
VM_SRCS = $(VM_OBJS,.obj=.cpp) 

OBJ_DIR = ../../intermediate.mac/
VM_DIR = ../svm/svmcore/
LIB_DIR = ../../lib/

VM_SRC_AND_DIRS = $(addprefix $(VM_DIR),$(VM_SRCS))
VM_OBJ_AND_DIRS = $(addprefix $(OBJ_DIR),$(VM_OBJS))

CPP_COMPILER = g++
CPP_FLAGS = @g++.config.txt
LIBGEN = ar
LIBGEN_FLAGS = cr
	
$(LIB_DIR)sexy.vm.mac.lib: $(VM_OBJ_AND_DIRS)
	$(LIBGEN) $(LIBGEN_FLAGS) $(LIB_DIR)sexy.vm.mac.lib $(VM_OBJ_AND_DIRS)

$(OBJ_DIR)%.obj : $(addprefix $(VM_DIR),$(notdir %.cpp))
	$(CPP_COMPILER) $(CPP_FLAGS) -c $< -o $@

clean:
	rm -f $(VM_OBJ_AND_DIRS) $(LIB_DIR)sexy.vm.mac.lib

