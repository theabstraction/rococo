#include <mplat.api.hlsl>

GuiPixelVertexOpaque main(GuiVertexOpaque v)
{
    GuiPixelVertexOpaque sv;
    sv.position = ConvertGuiScreenPixelCoordinatesToDX11Coordinates(v);
    sv.colour = GetGuiVertexColour(v);
    sv.base = v.base;
    sv.sd = v.sd;
	return sv;
}