$testDirectory = $PSScriptRoot
$solutionDir = (get-item $testDirectory).parent.FullName
$binDirectory = $solutionDir + '\bin'
$exe = $binDirectory + '\rococo.wrappergen.exe'
$scripts = $solutionDir + '\content\scripts'
$nativeScripts = $scripts + '\native'
$args = @($nativeScripts, '-symbols:sxyobject\symbols.txt', '-ns:Rococo::WrapperTest', 'sxyobject\dog.sxy')  
Set-Location -Path $scripts
$targetFile = $testDirectory + '\dog_sxy.inl'
& $exe $args > $targetFile
