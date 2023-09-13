#include <mplat.api.hlsl>

GuiPixelVertexOpaque main(GuiVertexOpaque v)
{
	GuiPixelVertexOpaque sv;
	
    sv.position = ConvertGuiScreenPixelCoordinatesToDX11Coordinates(v);
    sv.colour = GetGuiVertexColour(v);
    sv.base.xy = GetGuiTextureUV(v);
    sv.base.z = v.base.z;
    sv.sd = v.sd;
	return sv;
}