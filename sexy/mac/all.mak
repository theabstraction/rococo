# Make file for all modules in the Rococo.Sexy library. It first builds the rococo utils library

LIB_DIR = ../../lib/
BIN_DIR = ../bin/mac/
ROCOCO_DIR = ../../rococo.util/

$(BIN_DIR)sexy.script.test: $(LIB_DIR)rococo.util.mac.lib $(LIB_DIR)rococo.maths.mac.lib $(LIB_DIR)rococo.fonts.mac.lib $(LIB_DIR)rococo.zlib.mac.lib $(LIB_DIR)rococo.tiff.mac.lib $(LIB_DIR)rococo.jpg.mac.lib $(LIB_DIR)rococo.tiff.mac.lib $(LIB_DIR)sexy.utilities.mac.lib $(LIB_DIR)sexy.s-parser.mac.lib $(LIB_DIR)sexy.vm.mac.lib $(LIB_DIR)sexy.compiler.mac.lib $(LIB_DIR)sexy.script.mac.lib $(BIN_DIR)sexy.nativelib.maths.mac.dylib $(BIN_DIR)sexy.nativelib.reflection.mac.dylib $(BIN_DIR)sexy.vm.test $(BIN_DIR)sexy.compiler.test $(BIN_DIR)sexy.parser.test			
	make -f script.test.mak
	
$(LIB_DIR)sexy.utilities.mac.lib: utilities.mak
	make -f utilities.mak
	
$(LIB_DIR)rococo.tiff.mac.lib: ../../mac/tiff.mak
	make -C ../../mac/ -f tiff.mak

$(LIB_DIR)rococo.zlib.mac.lib: ../../mac/zlib.mak
	make -C ../../mac/ -f zlib.mak

$(LIB_DIR)rococo.maths.mac.lib: ../../mac/maths.mak
	make -C ../../mac/ -f maths.mak
	
$(LIB_DIR)rococo.jpg.mac.lib: ../../mac/jpg.mak
	make -C ../../mac/ -f jpg.mak

$(LIB_DIR)rococo.fonts.mac.lib: ../../mac/fonts.mak
	make -C ../../mac/ -f fonts.mak	
	
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
	
$(BIN_DIR)sexy.vm.test: vm.test.mak
	make -f vm.test.mak
	
$(BIN_DIR)sexy.compiler.test: compiler.test.mak
	make -f compiler.test.mak
	
$(BIN_DIR)sexy.parser.test: parser.test.mak
	make -f parser.test.mak
	
$(BIN_DIR)sexy.script.mac.lib: script.mak
	make -f script.mak

$(BIN_DIR)sexy.nativelib.reflection.mac.dylib: nativelibs.reflection.mak
	make -f nativelibs.reflection.mak
	
$(BIN_DIR)sexy.nativelib.maths.mac.dylib: nativelibs.maths.mak
	make -f nativelibs.maths.mak
	
$(BIN_DIR)sexy.nativelib.coroutines.mac.dylib: nativelibs.coroutines.mak
	make -f nativelibs.coroutines.mak
	
clean:
	make -f vm.mak        clean
	make -f utilities.mak clean
	make -f s-parser.mak  clean
	make -f compiler.mak  clean
	make -f script.mak    clean
	make -f nativelibs.reflection.mak clean
	make -f nativelibs.coroutines.mak clean
	make -f nativelibs.maths.mak      clean
	make -f vm.test.mak            clean
	make -f compiler.test.mak      clean
	make -f parser.test.mak        clean
	make -f script.test.mak        clean
	make -C ../../mac -f utils.mak clean
	make -C ../../mac -f tiff.mak  clean
	make -C ../../mac -f jpg.mak   clean
	make -C ../../mac -f zlib.mak   clean
	make -C ../../mac -f fonts.mak clean
	make -C ../../mac -f maths.mak clean
	
test:
	cd $(BIN_DIR);./sexy.parser.test;./sexy.vm.test;./sexy.compiler.test;./sexy.script.test;

