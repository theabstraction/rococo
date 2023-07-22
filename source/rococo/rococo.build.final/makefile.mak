$(BIN)native.hashes.sxy: ..\..\source\rococo\sexy\NativeSource\native.hashes.sxy
	copy ..\..\source\rococo\sexy\NativeSource\native.hashes.sxy $(BIN)

all: $(BIN)native.hashes.sxy

install:
	@..\..\build\tools\package.bat $(CONFIG)
	@..\..\build\tools\ship.sexystudio.bat $(CONFIG)

clean:
