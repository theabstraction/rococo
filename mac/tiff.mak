# Make file for Utilities module in the Rococo.Sexy library

TIFF_OBJS = tif_aux.obj tif_close.obj tif_codec.obj tif_compress.obj tif_dir.obj tif_dirinfo.obj tif_dirread.obj tif_dumpmode.obj\
 tif_error.obj tif_extension.obj tif_fax3.obj tif_fax3sm.obj tif_flush.obj tif_getimage.obj tif_jbig.obj tif_jpeg.obj tif_jpeg_12.obj\
 tif_luv.obj tif_lzw.obj tif_next.obj tif_ojpeg.obj tif_open.obj tif_packbits.obj tif_pixarlog.obj tif_predict.obj tif_print.obj\
 tif_read.obj tif_strip.obj tif_swab.obj tif_thunder.obj tif_tile.obj tif_version.obj tif_warning.obj tif_write.obj tif_zip.obj

TIFF_SRCS = $(TIFF_OBJS,.obj=.c) 

ROCOCO_HEADERS = ../include/
OBJ_DIR = ../intermediate.mac/
TIFF_DIR = ../libtiff/libtiff/
LIB_DIR = ../lib/

TIFF_SRC_AND_DIRS = $(addprefix $(TIFF_DIR),$(TIFF_SRCS))
TIFF_OBJ_AND_DIRS = $(addprefix $(OBJ_DIR),$(TIFF_OBJS))

CPP_COMPILER = g++
C_COMPILER = gcc
C_FLAGS = -g -c
CPP_FLAGS = -g -c -std=c++11
LIBGEN = ar
LIBGEN_FLAGS = cr
	
$(LIB_DIR)rococo.tiff.mac.lib: $(TIFF_OBJ_AND_DIRS) $(OBJ_DIR)bloke.tiff.objp
	$(LIBGEN) $(LIBGEN_FLAGS) $(LIB_DIR)rococo.tiff.mac.lib $(TIFF_OBJ_AND_DIRS)

$(OBJ_DIR)%.obj : $(addprefix $(TIFF_DIR),$(notdir %.c))
	$(C_COMPILER) $(C_FLAGS) -I$(ROCOCO_HEADERS) $< -o $@
	
$(OBJ_DIR)bloke.tiff.objp : ../libtiff/bloke.tiff.cpp
	$(CPP_COMPILER) $(CPP_FLAGS) -I$(ROCOCO_HEADERS) -I$(TIFF_DIR) $< -o $@

clean:
	rm -f $(LIB_DIR)rococo.tiff.mac.lib $(TIFF_OBJ_AND_DIRS) $(OBJ_DIR)bloke.tiff.objp