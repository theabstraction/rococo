# Make file for jpg module in the Rococo.Sexy library

.SUFFIXES:

JPG_OBJS = jcapimin.obj jcapistd.obj jccoefct.obj jccolor.obj jcdctmgr.obj jchuff.obj jcinit.obj jcmainct.obj jcmarker.obj\
 jcmaster.obj jcomapi.obj jcparam.obj jcphuff.obj jcprepct.obj jcsample.obj jctrans.obj jdapimin.obj jdapistd.obj jdatafromem.obj\
 jdcoefct.obj jdcolor.obj jddctmgr.obj jdhuff.obj jdinput.obj jdmainct.obj jdmarker.obj jdmaster.obj jdmerge.obj jdphuff.obj\
 jdpostct.obj jdsample.obj jdtrans.obj jerror.obj jfdctflt.obj jfdctfst.obj jfdctint.obj jidctflt.obj jidctfst.obj jidctint.obj\
 jidctred.obj jmemmgr.obj jmemnobs.obj jquant1.obj jquant2.obj jutils.obj rdbmp.obj rdcolmap.obj rdgif.obj rdppm.obj rdrle.obj\
 rdtarga.obj transupp.obj wrbmp.obj wrgif.obj wrppm.obj wrrle.obj wrtarga.obj

JPG_SRCS = $(JPG_OBJS,.obj=.c) 

ROCOCO_HEADERS = ../include/
OBJ_DIR = ../intermediate.mac/
JPG_DIR = ../libjpg/jpeg-6b/
LIB_DIR = ../lib/

JPG_SRC_AND_DIRS = $(addprefix $(JPG_DIR),$(JPG_SRCS))
JPG_OBJ_AND_DIRS = $(addprefix $(OBJ_DIR),$(JPG_OBJS))

CPP_COMPILER = g++
C_COMPILER = gcc
C_FLAGS = @gcc.config.txt
CPP_FLAGS = @g++.config.txt
LIBGEN = ar
LIBGEN_FLAGS = cr
	
$(LIB_DIR)rococo.jpg.mac.lib: $(JPG_OBJ_AND_DIRS) $(OBJ_DIR)rococo.jpg.readimage.objp  $(OBJ_DIR)rococo.jpg.writeimage.objp
	$(LIBGEN) $(LIBGEN_FLAGS) $(LIB_DIR)rococo.jpg.mac.lib $(JPG_OBJ_AND_DIRS) $(OBJ_DIR)rococo.jpg.readimage.objp  $(OBJ_DIR)rococo.jpg.writeimage.objp

$(OBJ_DIR)%.obj : $(addprefix $(JPG_DIR),$(notdir %.c))
	$(C_COMPILER) $(C_FLAGS) -I$(ROCOCO_HEADERS) $< -o $@
	
	
$(OBJ_DIR)rococo.jpg.readimage.objp : ../libjpg/readimage.cpp
	$(CPP_COMPILER) $(CPP_FLAGS) -I$(JPG_DIR) $< -o $@
		
$(OBJ_DIR)rococo.jpg.writeimage.objp : ../libjpg/writeimage.cpp
	$(CPP_COMPILER) $(CPP_FLAGS) -I$(JPG_DIR) $< -o $@

clean:
	rm -f $(LIB_DIR)rococo.jpg.mac.lib $(JPG_OBJ_AND_DIRS) $(OBJ_DIR)rococo.jpg.readimage.objp $(OBJ_DIR)rococo.jpg.writeimage.objp
	