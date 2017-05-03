# Make file for Utilities module in the Rococo.Sexy library

UTILS_OBJS = sexy.compiler.obj sexy.namespaces.obj sexy.natives.helpers.obj sexy.os.mac.obj sexy.parsering.obj sexy.s-parser.util.obj sexy.strings.2.obj\
        sexy.strings.obj sexy.util.obj sexy.variants.obj sexy.strings.builder.obj
UTILS_SRCS = $(UTILS_OBJS,.obj=.cpp) 

COMMON_DIR = ../Common/
ROCOCO_HEADERS = ../../include/
OBJ_DIR = ../../intermediate.mac/
UTILS_DIR = ../Utilities/
LIB_DIR = ../../lib/
STC_LIB = ../STC/stccore/

UTILS_SRC_AND_DIRS = $(addprefix $(UTILS_DIR),$(UTILS_SRCS))
UTILS_OBJ_AND_DIRS = $(addprefix $(OBJ_DIR),$(UTILS_OBJS))

CPP_COMPILER = g++
CPP_FLAGS = -g -std=c++14
LIBGEN = ar
LIBGEN_FLAGS = cr
	
$(LIB_DIR)sexy.utilities.mac.lib: $(UTILS_OBJ_AND_DIRS)
	$(LIBGEN) $(LIBGEN_FLAGS) $(LIB_DIR)sexy.utilities.mac.lib $(UTILS_OBJ_AND_DIRS)

$(OBJ_DIR)%.obj : $(addprefix $(UTILS_DIR),$(notdir %.cpp))
	$(CPP_COMPILER) $(CPP_FLAGS) -I$(COMMON_DIR) -I$(STC_LIB) -I$(ROCOCO_HEADERS) -c $< -o $@

clean:
	rm -f $(UTILS_OBJ_AND_DIRS) $(LIB_DIR)sexy.utilities.mac.lib

