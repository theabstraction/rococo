rebuild:
	@xcopy ..\..\packages\mhost\MHost\mhost_sxh.sxy ..\..\content\scripts\interop\rococo\mhost\mhost_sxh.sxy 	 /s 	/i   /y    /d    /q
	@..\..\build\tools\package.bat $(CONFIG)
	

$(BIN)native.hashes.sxy: ..\..\source\rococo\sexy\NativeSource\native.hashes.sxy
	copy ..\..\source\rococo\sexy\NativeSource\native.hashes.sxy $(BIN)

all:
	@xcopy ..\..\packages\mhost\MHost\mhost_sxh.sxy ..\..\content\scripts\interop\rococo\mhost\mhost_sxh.sxy 	 /s 	/i   /y    /d    /q
	@..\..\build\tools\package.bat $(CONFIG)

clean:
	
