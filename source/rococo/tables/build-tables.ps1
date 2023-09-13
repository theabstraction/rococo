param(
    [Parameter(Mandatory)]
	[String]$solution, # example: C:\work\rococo\
    [String]$configuration, # example: Debug
    [String]$platform # example: x64
)



$tableDirectory = $solution + 'tables\'
$binDirectory = $solution + 'bin'
$carpenterApp = $binDirectory + '\' + $configuration + '\netcoreapp3.1\rococo.carpenter.app.exe'

Write-Output 'Running rococo.carpenter.app'

& $carpenterApp

Write-Output 'Running benny hill'

$sexyDirectory = $solution + 'sexy'
$sexyBinDirectory = $sexyDirectory + '\Bin\' + $platform + $configuration
$bennyHill = $sexyBinDirectory + '\sexy.bennyhill.exe'
$args = @($tableDirectory, 'rococo.carpenter.test.sxh', 'rococo.carpenter.test.sxh.h') 
& $bennyHill $args

