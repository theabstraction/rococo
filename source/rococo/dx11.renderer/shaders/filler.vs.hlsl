struct FillerVertex
{
	float2 position : POSITION;
};

struct FillerPixelVertex
{
	float4 position : SV_POSITION0;
};


FillerPixelVertex main(FillerVertex vertexInput )
{
	FillerPixelVertex pixelOutput;
	pixelOutput.position.xy = vertexInput.position.xy;
	pixelOutput.position.z = 0.0f;
	pixelOutput.position.w = 1.0f;
	return pixelOutput;
}