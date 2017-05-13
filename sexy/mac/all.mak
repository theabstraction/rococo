# Make file for all modules in the Rococo.Sexy library


LIB_DIR = ../../lib/
BIN_DIR = ../bin/mac/
ROCOCO_DIR = ../../rococo.util/

$(BIN_DIR)sexy.vm.test: $(LIB_DIR)sexy.utilities.mac.lib $(LIB_DIR)rococo.util.mac.lib  $(LIB_DIR)sexy.s-parser.mac.lib $(LIB_DIR)sexy.vm.mac.lib $(LIB_DIR)sexy.compiler.mac.lib $(LIB_DIR)sexy.script.mac.lib
	make -f vm.test.mak
	
$(LIB_DIR)sexy.utilities.mac.lib: utilities.mak
	make -f utilities.mak
	
$(LIB_DIR)sexy.s-parser.mac.lib: s-parser.mak
	make -f s-parser.mak
	
$(LIB_DIR)sexy.compiler.mac.lib: compiler.mak
	make -f compiler.mak
	
$(LIB_DIR)sexy.vm.mac.lib: vm.mak
	make -f vm.mak
	
$(LIB_DIR)rococo.util.mac.lib: ../../mac/utils.mak
	make -C ../../mac -f utils.mak
	
$(LIB_DIR)sexy.script.mac.lib: script.mak
	make -f script.mak
	
clean:
	make -f vm.mak        clean
	make -f utilities.mak clean
	make -f s-parser.mak  clean
	make -f compiler.mak  clean
	make -f script.mak    clean
	make -f vm.test.mak   clean
	make -C ../../mac -f utils.mak clean