all:
	@..\..\build\tools\package.bat $(CONFIG)
	@..\..\build\tools\ship.sexystudio.bat $(CONFIG)
clean:
