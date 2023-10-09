// gradient indices for 3D noise  

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
float3 main(float p : POSITION) : SV_TARGET 
{  
	return g[p * 16];
}   