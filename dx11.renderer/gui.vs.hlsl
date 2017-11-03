struct BaseVertexData
{
	float2 uv;
	float fontBlend; // 0 -> normal triangle, 1 -> modulate with font texture
};

struct SpriteVertexData
{
	float saturation; // 1.0 -> use colour, 0.0 -> use bitmap texture
	float spriteIndex; // Indexes the texture in the sprite array.
	float matIndex; // Indexes the texture in the material array
	float spriteToMatLerpFactor; // 0 -> use textureIndex, 1 -> use matIndex, lerping in between
};

struct GuiVertex
{
	float2 pos : POSITION;
	float3 base : TEXCOORD0;
	float4 sd: TEXCOORD1;
	float4 colour: COLOR;
};

struct PixelVertex
{
	float4 position : SV_POSITION;
	float3 base : TEXCOORD0;
	float4 sd : TEXCOORD1;
	float4 colour : COLOR;
};

struct GuiScale
{
	float OOScreenWidth;
	float OOScreenHeight;
	float OOFontWidth;
	float OOFontHeight;
};

GuiScale guiScale;

PixelVertex main(GuiVertex v)
{
	PixelVertex sv;

	sv.position.x = 2.0f * v.pos.x * guiScale.OOScreenWidth - 1.0f;
	sv.position.y = -2.0f * v.pos.y * guiScale.OOScreenHeight + 1.0f;
	sv.position.z = 0;
	sv.position.w = 1.0f;
	sv.colour = v.colour;
	sv.base = v.base;
	sv.sd = v.sd;
	sv.base.xy = lerp(sv.base.xy,  v.base.xy * guiScale.OOFontWidth, v.base.z);

	return sv;
}