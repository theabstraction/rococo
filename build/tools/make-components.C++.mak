# When we call the mak file from the batch file, the batch file sets the working directory to match the container for the makefile
ROCOCO = $(abspath ..\..\)
DIR_MPLAT = $(ROCOCO)source\rococo\rococo.mplat^\
MPLAT_COMPONENTS_H = $(DIR_MPLAT)mplat.components.h
MPLAT_COMPONENTS_XML = $(DIR_MPLAT)components.xml
TEMPLATE_CPP = $(DIR_MPLAT)mplat.component.template.cpp
TEMPLATE_H = $(DIR_MPLAT)mplat.component.template.h

$(MPLAT_COMPONENTS_H): $(MPLAT_COMPONENTS_XML) $(TEMPLATE_CPP) $(TEMPLATE_H)
	$(CPP_MASTER) $(MPLAT_COMPONENTS_XML) $(ROCOCO)
	
all: $(MPLAT_COMPONENTS_H)

