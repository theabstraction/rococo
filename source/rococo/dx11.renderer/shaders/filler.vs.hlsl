#include <mplat.types.hlsl>

struct FillerVertex
{
	float2 position : POSITION;
};

FillerPixelVertex main(FillerVertex vertexInput )
{
	FillerPixelVertex pixelOutput;
	pixelOutput.position.xy = vertexInput.position.xy;
	pixelOutput.position.z = 0.0f;
	pixelOutput.position.w = 1.0f;
	return pixelOutput;
}