# Rococo.make makefile.mak Copyright(c) 2022. All rights reserved, Mark Anthony Taylor - mark.anthony.taylor@gmail.com. 
# This file is part of the Rococo libraries. It is open source and free to use and vary.

SXH_FILE_FULL_PATH=$(SOURCE_ROOT)$(SXH_FILE)
SXH_INL=$(SXH_FILE_FULL_PATH).inl
SXL_H=$(SXH_FILE_FULL_PATH).h
SXH_XC_FULL_PATH = $(SOURCE_ROOT)$(XC_FILE)

$(SXH_INL): $(SXH_FILE_FULL_PATH) $(SXH_XC_FULL_PATH)
	$(BENNY_HILL) $(SOURCE_ROOT) $(CONTENT_ROOT) $(SXH_FILE)

all: $(SXH_INL)
	


