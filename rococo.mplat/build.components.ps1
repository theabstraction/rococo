$MPlatDirectory = $PSScriptRoot
$solutionDir = (get-item $MPlatDirectory).parent.FullName
$binDirectory = $solutionDir + '\bin'
$cpp_master = $binDirectory + '\Release\net6.0\rococo.cpp_master.exe'
$xml = $solutionDir + '\rococo.mplat\components.xml'
$solutionPath = $solutionDir + '\'
& $cpp_master $xml $solutionPath
