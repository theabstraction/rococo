rebuild:
	echo Hi
	..\..\build\tools\package.bat $(CONFIG)
	@..\..\build\tools\ship.sexystudio.bat $(CONFIG)

$(BIN)native.hashes.sxy: ..\..\source\rococo\sexy\NativeSource\native.hashes.sxy
	copy ..\..\source\rococo\sexy\NativeSource\native.hashes.sxy $(BIN)

all:
	..\..\build\tools\package.bat $(CONFIG)
	@..\..\build\tools\ship.sexystudio.bat $(CONFIG)

clean:

ship_studio:
	@..\..\build\tools\ship.sexystudio.bat $(CONFIG)