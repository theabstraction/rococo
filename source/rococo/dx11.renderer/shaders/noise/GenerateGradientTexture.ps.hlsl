// gradient indices for 3D noise  

#include <mplat.types.hlsl>

static float3 g[16] = 
{   
	{ 1, 1, 0},
	{-1, 1, 0},
	{ 1,-1, 0},  
	{-1,-1, 0},  
	{ 1, 0, 1},  
	{-1, 0, 1},  
	{ 1, 0,-1},  
	{-1, 0,-1},   
	{0,  1, 1}, 
	{0, -1, 1},  
	{0,  1,-1},  
	{0, -1,-1},  
	{1,  1, 0},  
	{0, -1, 1},  
	{-1, 1, 0},  
	{0, -1,-1} 
};  

// Generates an index look up table
float4 main(FillerPixelVertex p) : SV_TARGET
{  
    float x = p.position.x;
    return float4(g[x], 1.0f);
}   