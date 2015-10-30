struct Pos
{
	float x;
	float y;
	float saturation;
	float fontBlend;
};

struct WorldVertex
{
	Pos pos : POSITION;
	float4 colour : COLOR;
	float4 tx1: TEXCOORD0;
};

struct Tx
{
	float2 uv;
	float saturation;
	float fontBlend;
};

struct ScreenVertex
{
	float4 position : SV_POSITION;
	float4 colour : COLOR;
	Tx tx: TEXCOORD0;
};

struct GuiScale
{
	float OOScreenWidth;
	float OOScreenHeight;
	float OOFontWidth;
	float OOFontHeight;
};

GuiScale guiScale;

ScreenVertex main(WorldVertex v)
{
	ScreenVertex sv;

	sv.position.x = 2.0f * v.pos.x * guiScale.OOScreenWidth - 1.0f;
	sv.position.y = -2.0f * v.pos.y * guiScale.OOScreenHeight + 1.0f;
	sv.position.z = 0.0f;
	sv.position.w = 1.0f;
	sv.colour = v.colour;
	sv.tx.uv = v.pos.fontBlend * v.tx1.xy * guiScale.OOFontWidth + (1.0f - v.pos.fontBlend) * v.tx1.xy;
	sv.tx.saturation = v.pos.saturation;
	sv.tx.fontBlend = v.pos.fontBlend;

	return sv;
}