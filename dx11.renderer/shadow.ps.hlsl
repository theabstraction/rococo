struct PixelVertex
{
	float4 position : SV_POSITION;
};

cbuffer depthRenderData
{
	float4x4 worldToCamera;
	float4x4 worldToScreen;
	float4 eye;
	float4 direction;
	float4 right;
	float4 up;
	float nearPlane;
	float farPlane;
	float fov; // radians
	float time; // Used for animation, ranges from 0 to ~ 59.99f
	float4 randoms; // 0.0 - 1.0 x4
};

float4 main(PixelVertex p) : SV_TARGET
{ 
	return float4(p.position.z, 0, 0, 1.0f);
}