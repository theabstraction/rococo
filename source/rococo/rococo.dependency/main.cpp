int main(int argc, char* argv[])
{
	// this project exists simply to hack visual studio into compiling this before rococo.mplat is compiled.
	// Compiling this file forces the DLLs, libs and executables it depends upon to compile first. 
	// I had trouble making rococo.mplat build order correct, because as a lib, sharpmake appears to ignore its dependencies on DLLs.
	// The reason the lib is dependent on the DLL, is that the DLL uses code-generation to create interface headers that mplat depends on.
	// There appears to be no mechanism in Sharpmap.ProjectDependency to make a project depend on code-generation from another, so this hack has to do for now.
	// Better solutions warmly received
	return 0;
}