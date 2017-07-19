# Make file for Utilities module in the Rococo library

UNIT_OBJS = adler32.obj compress.obj crc32.obj deflate.obj infback.obj inffast.obj inflate.obj inftrees.obj trees.obj uncompr.obj zutil.obj

UNIT_SRCS = $(UNIT_OBJS,.obj=.c) 

OBJ_DIR = ../intermediate.mac/
UNIT_DIR = ../zlib/
LIB_DIR = ../lib/

UNIT_SRC_AND_DIRS = $(addprefix $(UNIT_DIR),$(UNIT_SRCS))
UNIT_OBJ_AND_DIRS = $(addprefix $(OBJ_DIR),$(UNIT_OBJS))

C_COMPILER = gcc
C_FLAGS = @gcc.config.txt
LIBGEN = ar
LIBGEN_FLAGS = cr

TARGET = $(LIB_DIR)rococo.zlib.mac.lib
	
$(TARGET): $(UNIT_OBJ_AND_DIRS)
	$(LIBGEN) $(LIBGEN_FLAGS) $(TARGET) $(UNIT_OBJ_AND_DIRS)

$(OBJ_DIR)%.obj : $(addprefix $(UNIT_DIR),$(notdir %.c))
	$(C_COMPILER) $(C_FLAGS) $< -o $@
	
clean:
	rm -f $(TARGET) $(UNIT_OBJ_AND_DIRS)

