# Rococo.make makefile.mak Copyright(c) 2022. All rights reserved, Mark Anthony Taylor - mark.anthony.taylor@gmail.com. 
# This file is part of the Rococo libraries. It is open source and free to use and vary.

# NMAKE /NOLOGO /F "..\tools\rococo.sxh.mak" CONFIG=%1 SOURCE_ROOT=%2 INTEROP_SUBDIR=%3 SXH_FILE=%4 XC_FILE=%5

!IFNDEF CONFIG
!  ERROR tools\rococo.sxh.mak error: CONFIG was not defined for rococo.sxh.mak. It should  be passed via the command line
!ENDIF

!IFNDEF SOURCE_ROOT
!  ERROR tools\rococo.sxh.mak error: SOURCE_ROOT was not defined for rococo.sxh.mak. It should be passed via the command line
!ENDIF

!IFNDEF INTEROP_SUBDIR
!  ERROR tools\rococo.sxh.mak error: INTEROP_SUBDIR was not defined for rococo.sxh.mak. It should be passed via the command line
!ENDIF

!IFNDEF SXH_FILE
!  ERROR tools\rococo.sxh.mak error: SXH_FILE was not defined for rococo.sxh.mak. It should be passed via the command line
!ENDIF

!IFNDEF SXH_FILE
!  ERROR tools\rococo.sxh.mak error: SXH_FILE was not defined for rococo.sxh.mak. It should be passed via the command line
!ENDIF

!IFNDEF XC_FILE
!  ERROR tools\rococo.sxh.mak error: XC_FILE was not defined for rococo.sxh.mak. It should be passed via the command line
!ENDIF

SXH_FILE_FULL_PATH=$(SOURCE_ROOT)\$(SXH_FILE)
SXH_INL=$(SXH_FILE_FULL_PATH).inl
SXL_H=$(SXH_FILE_FULL_PATH).h
SXH_XC_FULL_PATH = $(SOURCE_ROOT)\$(XC_FILE)
ROCOCO_DIR=.^\
CONTENT_DIR=$(ROCOCO_DIR)content^\
INTEROP_DIR=$(CONTENT_DIR)scripts\interop^\
BIN_DIR=$(ROCOCO_DIR)gen\bin\win64\$(CONFIG)^\
BENNY_HILL=$(BIN_DIR)sexy.bennyhill.exe

!IF EXISTS($(SXH_FILE_FULL_PATH))
!ELSE
!   ERROR tools\rococo.sxh.mak error:  $(SXH_FILE_FULL_PATH) does not appear to exist
!ENDIF

!IF EXISTS($(SXH_XC_FULL_PATH))
!ELSE
!   ERROR tools\rococo.sxh.mak error:  $(SXH_XC_FULL_PATH) does not appear to exist
!ENDIF

# We want to generate an INL file, and it depends on the sxh and associated config.xc file. To create it we run benny hill on the sxh file.
$(SXH_INL): $(SXH_FILE_FULL_PATH) $(SXH_XC_FULL_PATH)
	$(BENNY_HILL) $(SOURCE_ROOT)\ $(INTEROP_DIR)$(INTEROP_SUBDIR)\ $(SXH_FILE) $(SXH_XC_FULL_PATH)
	@echo Built $(SXH_INL) which depended on $(SXH_FILE_FULL_PATH) and $(SXH_XC_FULL_PATH). Rococo root is: $(ROCOCO_DIR). 
