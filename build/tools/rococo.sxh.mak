# Rococo.make makefile.mak Copyright(c) 2022. All rights reserved, Mark Anthony Taylor - mark.anthony.taylor@gmail.com. 
# This file is part of the Rococo libraries. It is open source and free to use and vary.

SXH_FILE_FULL_PATH=$(SOURCE_ROOT)$(SXH_FILE)
SXH_INL=$(SXH_FILE_FULL_PATH).inl
SXL_H=$(SXH_FILE_FULL_PATH).h
SXH_XC_FULL_PATH = $(SOURCE_ROOT)$(XC_FILE)

# We want to generate an INL file, and it depends on the sxh and associated config.xc file. To create it we run benny hill on the sxh file.
$(SXH_INL): $(SXH_FILE_FULL_PATH) $(SXH_XC_FULL_PATH)
	$(BENNY_HILL) $(SOURCE_ROOT) $(CONTENT_ROOT) $(SXH_FILE)

# In normal mode of operation, all we want to do is to ensure the inl file is up to date
all: $(SXH_INL)
	
