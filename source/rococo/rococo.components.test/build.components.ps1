$testDirectory = $PSScriptRoot
$solutionDir = (get-item $testDirectory).parent.FullName
$binDirectory = $solutionDir + '\bin'
$cpp_master = $binDirectory + '\Debug\net6.0\rococo.cpp_master.exe'
$xml = $solutionDir + '\rococo.components.test\test.xml'
$solutionPath = $solutionDir + '\'
& $cpp_master $xml $solutionPath
