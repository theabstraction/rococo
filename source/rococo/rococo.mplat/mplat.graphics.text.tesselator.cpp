#include <rococo.mplat.h>
#include <vector>
#include <rococo.fonts.h>
#include <rococo.strings.h>

using namespace Rococo;
using namespace Rococo::Graphics;
using namespace Rococo::Strings;

namespace ANON
{
	const float fontWidth = 1024.0f;
	const float ooFontWidth = 1.0f / 1024.0f;

	// Used for 2D bitmap text on 3D surfaces
	struct TextTesselator: public ITextTesselatorSupervisor, public Fonts::IGlyphRenderer
	{
		Platform& platform;

		Quad formatQuad;
		Vec3 tangent{ 1,0,0 };
		Vec3 vertical{ 0,1,0 };
		Vec3 normal{ 0,0,1 };
		std::vector<QuadVertices> quads;
		float scale = 1;
		int align = 0;
		const int32 virtualPixelsPerQuad = 1024;

		Fonts::IFont& font;

		int fontIndex = 0;

		float32 deltaZ = 0;

		TextTesselator(Platform& _platform):
			platform(_platform), 
			font(_platform.graphics.renderer.GuiResources().FontMetrics())
		{
		}

		void AddTextJob(const Vec2i& pos, Fonts::IDrawTextJob& job, const GuiRect* clipRect)
		{
			char stackBuffer[128];
			Fonts::IGlyphRenderPipeline* pipeline = Fonts::CreateGlyphRenderPipeline(stackBuffer, sizeof(stackBuffer), *this);

			GuiRectf qrect(-10000.0f, -10000.0f, 10000.0f, 10000.0f);

			if (clipRect != nullptr)
			{
				qrect.left = (float)clipRect->left;
				qrect.right = (float)clipRect->right;
				qrect.top = (float)clipRect->top;
				qrect.bottom = (float)clipRect->bottom;
			}
			RouteDrawTextBasic(pos, job, font, *pipeline, qrect);
		}

		void DrawGlyph(cr_vec2 uvTopLeft, cr_vec2 uvBottomRight, cr_vec2 posTopLeft, cr_vec2 posBottomRight, Fonts::FontColour fcolour) override
		{
			QuadVertices q = { 0 };
			q.normals.a = normal;
			q.colours.a = *((RGBAb*)&fcolour);
			q.colours.b.alpha = 255;

			float u0 = ooFontWidth * (uvTopLeft.x + 0.5f);
			float v1 = ooFontWidth * (uvTopLeft.y + 0.5f);
			float u1 = ooFontWidth * (uvBottomRight.x - 0.5f);
			float v0 = ooFontWidth * (uvBottomRight.y - 0.5f);
			q.uv = { u0, v1, u1, v0 };
			
			float32 t0 = posTopLeft.x;
			float32 t1 = posBottomRight.x;
			float32 s1 = virtualPixelsPerQuad - posTopLeft.y;
			float32 s0 = posBottomRight.y;
			
			q.positions.a = formatQuad.d + (t0 * tangent + s1 * vertical) * scale;
			q.positions.b = formatQuad.d + (t1 * tangent + s1 * vertical) * scale;
			q.positions.c = formatQuad.d + (t1 * tangent + s0 * vertical) * scale;
			q.positions.d = formatQuad.d + (t0 * tangent + s0 * vertical) * scale;

			deltaZ += 0.00001f;

			q.positions.a.z = q.positions.b.z = q.positions.c.z = q.positions.d.z = deltaZ;

			quads.push_back(q);
		}

		void AddBlankQuad(const Quad& positions, RGBAb paperColour) override
		{
			QuadVertices q = { 0 };
			q.positions = positions;
			Vec3 normal = Normalize(Cross(positions.b - positions.a, positions.b - positions.c));
			q.normals.a = normal;
			q.colours.a = paperColour;
			q.colours.b.alpha = 0;
			q.uv = { 0,0,0,0 };
			quads.push_back(q);
		}

		void AddLeftAlignedText(RGBAb colour, const fstring& text) override
		{
			deltaZ = 0;

			Graphics::StackSpaceGraphics ssg;
			GuiRect rect{ 0, 0, virtualPixelsPerQuad, virtualPixelsPerQuad };
			int fontHeight = 16;
			auto& job = Graphics::CreateLeftAlignedText(ssg, rect, 200, 120, fontHeight, fontIndex, text, colour);
			AddTextJob({ 0,0 }, job, nullptr);
		}

		void Clear() override
		{
			quads.clear();
			deltaZ = 0;
		}

		void Free() override
		{
			delete this;
		}

		void SetFormatQuad(const Quad& q) override
		{
			formatQuad = q;
			tangent = q.b - q.a;
			vertical = q.a - q.d;
			scale = 1.0f / virtualPixelsPerQuad;
			normal = Normalize(Cross(q.b - q.a, q.b - q.c));
		} 

		void SetUVScale(float scaleFactor) override
		{
			scale = scaleFactor;
		}

		void SaveMesh(const fstring& meshName) override
		{
			platform.graphics.meshes.Clear();
			platform.graphics.meshes.Begin(meshName);

			MaterialVertexData mat = { 0 };

			VertexTriangle topRight, bottomLeft;
			for (const auto& q : quads)
			{
				mat.colour = q.colours.a;
				mat.gloss = q.colours.b.alpha; // Gloss is factor in font to solid colour lerp

				topRight.a.position = q.positions.a;
				topRight.b.position = q.positions.b;
				topRight.c.position = q.positions.c;

				topRight.a.normal = topRight.b.normal = topRight.c.normal = q.normals.a;

				bottomLeft.a.position = q.positions.c;
				bottomLeft.b.position = q.positions.d;
				bottomLeft.c.position = q.positions.a;

				bottomLeft.a.normal = bottomLeft.b.normal = bottomLeft.c.normal = q.normals.a;

				topRight.a.uv = { q.uv.left, q.uv.top };
				topRight.b.uv = { q.uv.right, q.uv.top };
				topRight.c.uv = { q.uv.right, q.uv.bottom };

				bottomLeft.a.uv = { q.uv.right, q.uv.bottom };
				bottomLeft.b.uv = { q.uv.left, q.uv.bottom };
				bottomLeft.c.uv = { q.uv.left, q.uv.top };

				topRight.a.material = topRight.b.material = topRight.c.material = mat;
				bottomLeft.a.material = bottomLeft.b.material = bottomLeft.c.material = mat;

				platform.graphics.meshes.AddTriangleEx(topRight);
				platform.graphics.meshes.AddTriangleEx(bottomLeft);
			}

			platform.graphics.meshes.End(false, false);

			quads.clear();
		}

		boolean32 TrySetFontIndex(int32 index) override
		{
			if (index >= 0 && index < font.NumberOfGlyphSets())
			{
				fontIndex = index;
				return true;
			}
			return false;
		}

		boolean32 TrySetFont(const fstring& argname, float dotSize) override
		{
			int32 bestIndex = -1;
			float bestDelta = 1e38f;

			for (int i = 0; i < font.NumberOfGlyphSets(); ++i)
			{
				auto& glyphSet = font[i];
				
				bool candidate = false;

				if (argname.length == 0) candidate = true;
				else
				{
					cstr name = glyphSet.Name();
					if (Eq(name, argname)) candidate = true;
				}
			
				if (candidate)
				{
					float delta = fabsf(glyphSet.FontHeight() - dotSize);
					if (delta < bestDelta)
					{
						bestIndex = i;
						bestDelta = delta;
					}
				}
			}

			if (bestIndex >= 0)
			{
				fontIndex = bestIndex;
				return true;
			}

			return false;
		}
	};
}

namespace Rococo
{
	namespace Graphics
	{
		ITextTesselatorSupervisor* CreateTextTesselator(Platform& platform)
		{
			return new ANON::TextTesselator(platform);
		}
	}
}