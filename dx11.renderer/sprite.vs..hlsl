struct Pos
{
   float x;
   float y;
   float unusedz;
   float unusedw;
};

struct SpriteVertex
{
   Pos pos : POSITION;
   float4 unusedColour : COLOR;
   float4 tx: TEXCOORD0;
};

struct Tx
{
   float2 uv;
   float arrayIndex;
};

struct ScreenVertex
{
   float4 position : SV_POSITION;
   Tx tx : TEXCOORD0;
};

struct GuiScale
{
   float OOScreenWidth;
   float OOScreenHeight;
   float OOTextureArrayWidth;
   float OOTextureArrayHeight;
};

GuiScale guiScale;

ScreenVertex main(SpriteVertex v)
{
   ScreenVertex sv;

   sv.position.x = 2.0f * v.pos.x * guiScale.OOScreenWidth - 1.0f;
   sv.position.y = -2.0f * v.pos.y * guiScale.OOScreenHeight + 1.0f;
   sv.position.z = 0.0f;
   sv.position.w = 1.0f;
   sv.tx.uv = v.tx.xy * guiScale.OOTextureArrayWidth;
   sv.tx.arrayIndex = v.tx.z;

   return sv;
}