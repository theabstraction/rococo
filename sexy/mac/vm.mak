# Make file for VM module in the Rococo.Sexy library

VM_OBJS = sexy.vm.assembler.obj sexy.vm.core.obj sexy.vm.obj sexy.vm.disassembler.obj sexy.vm.main.obj sexy.vm.os.mac.obj sexy.vm.program.obj\
        sexy.vm.stdafx.obj sexy.vm.symbols.obj
VM_SRCS = $(OBJS,.obj=.cpp) 

COMMON_DIR = ../Common/
ROCOCO_HEADERS = ../../include/
OBJ_DIR = ../../intermediate.mac/
VM_DIR = ../svm/svmcore/
LIB_DIR = ../../lib/

VM_SRC_AND_DIRS = $(addprefix $(VM_DIR),$(VM_SRCS))
VM_OBJ_AND_DIRS = $(addprefix $(OBJ_DIR),$(VM_OBJS))

CPP_COMPILER = g++
CPP_FLAGS = -Wall -g -std=c++14
LIBGEN = ar
LIBGEN_FLAGS = cr
	
$(LIB_DIR)sexy.vm.mac.lib: $(VM_OBJ_AND_DIRS)
	$(LIBGEN) $(LIBGEN_FLAGS) $(LIB_DIR)sexy.vm.mac.lib $(VM_OBJ_AND_DIRS)

$(OBJ_DIR)%.obj : $(addprefix $(VM_DIR),$(notdir %.cpp))
	$(CPP_COMPILER) $(CPP_FLAGS)    -I$(COMMON_DIR)   -I$(ROCOCO_HEADERS)   -c $<   -o $@
	