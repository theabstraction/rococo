all:
	@..\..\build\tools\package.bat $(CONFIG)
	@..\..\build\tools\ship.sexystudio.bat $(CONFIG)
	copy ..\..\source\rococo\sexy\NativeSource\native.hashes.sxy $(BIN)
clean:
