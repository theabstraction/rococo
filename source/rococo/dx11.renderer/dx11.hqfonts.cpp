#include "dx11.renderer.h"
#include <rococo.fonts.hq.h>
#include <rococo.imaging.h>
#include <vector>
#include <array>

using namespace Rococo;
using namespace Rococo::Strings;
using namespace Rococo::Graphics;
using namespace Rococo::Graphics::Fonts;

namespace ANON
{
	using namespace Rococo::DX11;

	struct HQTextBuilder : IHQTextBuilder
	{
		Fonts::IArrayFont& font;
		const Fonts::ArrayFontMetrics& metrics;
		RGBAb colour{ 255,255,255,255 };
		Vec2 span;
		GuiRectf nextRect;
		GuiRectf clipRect;
		bool evaluateSpanOnly = false;
		IDX11FontRenderer* renderer = nullptr;

		HQTextBuilder(Fonts::IArrayFont& _font, const GuiRectf& _clipRect) :
			font(_font),
			metrics(_font.Metrics()),
			span{ (float)metrics.imgWidth, (float)metrics.imgHeight },
			nextRect{ 0, 0, span.x, span.y },
			clipRect(_clipRect)
		{
		}

		const Fonts::ArrayFontMetrics& Metrics() const override
		{
			return metrics;
		}

		void SetColour(RGBAb _colour) override
		{
			colour = _colour;
		}

		void SetCursor(Vec2 pos) override
		{
			Vec2 floorPos{ floorf(pos.x), floorf(pos.y) };
			nextRect = { floorPos.x, floorPos.y - span.y, span.x, floorPos.y };
		}

		void Write(char c, GuiRectf* outputBounds) override
		{
			Write((char32_t)(unsigned char)c, outputBounds);
		}

		void Write(wchar_t c, GuiRectf* outputBounds) override
		{
			Write((char32_t)c, outputBounds);
		}

		static void ClipOutputRectAgainstClipRect(const GuiRectf& clipRect, GuiRectf& Rprimed, const GuiRectf& R, GuiRectf& tPrimed)
		{
			Rprimed = R;
			tPrimed = { 0,0,1.0f,1.0f };

			Rprimed.left = R.left;
			if (Rprimed.left < clipRect.left)
			{
				Rprimed.left = clipRect.left;
				tPrimed.left = (Rprimed.left - R.left) / Width(R);
			}
			
			if (Rprimed.top < clipRect.top)
			{
				Rprimed.top = clipRect.top;
			}
			else
			{
				tPrimed.top = (Rprimed.top - R.top) / Height(R);
			}

			if (Rprimed.right > clipRect.right)
			{
				Rprimed.right = clipRect.right;
				tPrimed.right = (Rprimed.right - R.left) / Width(R);
			}
			
			if (Rprimed.bottom > clipRect.bottom)
			{
				Rprimed.bottom = clipRect.bottom;
			}
			else
			{
				tPrimed.bottom = (Rprimed.bottom - R.top) / Height(R);
			}
		}

		void Write(char32_t c, GuiRectf* outputBounds) override
		{
			const Fonts::ArrayGlyph& g = font[c];

			float i = (float)g.Index;

			auto& R = nextRect;

			R.right = R.left + span.x;

			bool isFullyClipped = R.right <= clipRect.left || R.left >= clipRect.right || R.top >= clipRect.bottom || R.bottom <= clipRect.top;
			
			if (!evaluateSpanOnly && colour.alpha > 0 && !isFullyClipped)
			{
				GuiRectf Rprimed;
				GuiRectf Tprimed;
				ClipOutputRectAgainstClipRect(clipRect, Rprimed, R, Tprimed);

				SpriteVertexData ithTexture{ 0, i, 0, 0 };

				GuiQuad q
				{
					/*
					struct GuiVertex
					{
						Vec2 pos;
						BaseVertexData vd; // 3 floats
						SpriteVertexData sd; // 4 floats
						RGBAb colour;
					};
					*/

					{ TopLeft(Rprimed),     {TopLeft(Tprimed),     0}, ithTexture, colour},
					{ TopRight(Rprimed),    {TopRight(Tprimed),    0}, ithTexture, colour},
					{ BottomLeft(Rprimed),  {BottomLeft(Tprimed),  0}, ithTexture, colour},
					{ BottomRight(Rprimed), {BottomRight(Tprimed), 0}, ithTexture, colour},
				};

				GuiTriangle A{ q.topLeft, q.topRight, q.bottomRight };
				GuiTriangle B{ q.bottomRight, q.bottomLeft, q.topLeft };
				renderer->AddTriangle(&A.a);
				renderer->AddTriangle(&B.a);
			}

			if (outputBounds != nullptr)
			{
				R.right = R.left + g.B + g.C;
				*outputBounds = R;
			}

			R.left += g.A + g.B + g.C;
		}
	}; // struct HQBuilder
}// ANON

namespace Rococo::DX11
{
	struct OSFont
	{
		Fonts::IArrayFontSupervisor* arrayFont;
		Fonts::FontSpec spec;
		HString fontFamily;
		IDX11BitmapArray* array;
	};

	bool operator == (const Fonts::FontSpec& a, const Fonts::FontSpec& b)
	{
		return
			Eq(a.fontName, b.fontName) &&
			a.height == b.height &&
			a.italic == b.italic &&
			a.weight == b.weight;
	}

	DX11::OSFont DuplicateScaled(DX11::OSFont& font, float zoomLevel, ID3D11Device& device, ID3D11DeviceContext& activeDC)
	{
		struct ANON : IEventCallback<const GlyphDesc>, IArrayFontSet
		{
			std::vector<GlyphDesc> newGlyphs;

			void OnEvent(const GlyphDesc& src) override
			{
				newGlyphs.push_back(src);
			}

			void Populate(IFontGlyphBuilder& builder)
			{
				for (auto& g : newGlyphs)
				{
					if (g.charCode >= 32 && g.charCode < 128)
					{
						builder.AddGlyph((unsigned char) g.charCode);
					}
				}
			}
		} glyphBuilder;

		font.arrayFont->ForEachGlyph(glyphBuilder);

		FontSpec newSpec = font.spec;
		newSpec.height = (int)(font.spec.height * zoomLevel);
		AutoFree<IArrayFontSupervisor> newFont = CreateOSFont(glyphBuilder, newSpec);

		AutoFree<IDX11BitmapArray> new_array2D = CreateDX11BitmapArray(device, activeDC);
		OSFont osFont{ newFont, font.spec, font.spec.fontName, new_array2D };
		
		struct : IEventCallback<const Fonts::GlyphDesc>
		{
			OSFont* font;
			size_t index = 0;
			void OnEvent(const Fonts::GlyphDesc& gd) override
			{
				struct : Imaging::IImagePopulator<GRAYSCALE>
				{
					size_t index;
					OSFont* font;
					void OnImage(const GRAYSCALE* pixels, int32 width, int32 height) override
					{
						font->array->WriteSubImage(index, pixels, { width, height });
					}
				} addImageToTextureArray;
				addImageToTextureArray.font = font;
				addImageToTextureArray.index = index++;
				font->arrayFont->GenerateImage(gd.charCode, addImageToTextureArray);
			}
		} addGlyphToTextureArray;

		addGlyphToTextureArray.font = &osFont;

		new_array2D->ResetWidth(newFont->Metrics().imgWidth, newFont->Metrics().imgHeight);
		new_array2D->Resize(newFont->NumberOfGlyphs());
		newFont->ForEachGlyph(addGlyphToTextureArray);

		return DX11::OSFont{ newFont.Detach(), font.spec, font.spec.fontName, new_array2D.Detach() };
	}

	struct DX11HQFontFonts: IDX11HQFontResource
	{
		ID3D11Device& device;
		std::vector<OSFont> osFonts;
		IDX11FontRenderer& renderer;
		ID3D11DeviceContext* activeDC = nullptr;

		std::array<ID_FONT, 10> sysFonts =
		{
			ID_FONT::Invalid(),
			ID_FONT::Invalid(),
			ID_FONT::Invalid(),
			ID_FONT::Invalid(),
			ID_FONT::Invalid(),
			ID_FONT::Invalid(),
			ID_FONT::Invalid(),
			ID_FONT::Invalid(),
			ID_FONT::Invalid(),
			ID_FONT::Invalid()
		};

		enum { ID_FONT_OSFONT_OFFSET = 400 };

		DX11HQFontFonts(IDX11FontRenderer& _renderer, ID3D11Device& _device, ID3D11DeviceContext& dc) : renderer(_renderer), device(_device), activeDC(&dc)
		{
		}

		ID_FONT NormalizeFontId(ID_FONT id) const
		{
			if (id.value < 0)
			{
				auto defautlSysId = sysFonts[0];
				return defautlSysId;
			}

			if (id.value >= 0 && id.value <= (int)sysFonts.size())
			{
				return sysFonts[id.value];
			}
			else
			{
				return id;
			}
		}

		ID_FONT CreateSysFont(Fonts::IArrayFontSet& glyphs, const Fonts::FontSpec& spec)
		{
			if (spec.sysFontId < 0 || spec.sysFontId >= (int)sysFonts.size())
			{
				Throw(0, "Cannot create system font. sysFontId >= %llu", sysFonts.size());
			}

			if (sysFonts[spec.sysFontId])
			{
				Throw(0, "SysFont #d already defined", spec.sysFontId);
			}

			FontSpec osSpec = spec;
			osSpec.sysFontId = -1;

			auto id = sysFonts[spec.sysFontId] = CreateOSFont(glyphs, osSpec);

			return id;
		}

		ID_FONT CreateOSFont(Fonts::IArrayFontSet& glyphs, const Fonts::FontSpec& spec) override
		{
			if (spec.sysFontId >= 0)
			{
				return CreateSysFont(glyphs, spec);
			}

			int i = 0;
			for (auto& osFont : osFonts)
			{
				if (osFont.spec == spec)
				{
					return ID_FONT{ i + ID_FONT_OSFONT_OFFSET };
				}

				i++;
			}

			Fonts::IArrayFontSupervisor* new_Font = Fonts::CreateOSFont(glyphs, spec);
			IDX11BitmapArray* new_array2D = CreateDX11BitmapArray(device, *activeDC);
			OSFont osFont{ new_Font, spec, spec.fontName, new_array2D };
			osFont.spec.fontName = osFont.fontFamily;
			osFonts.push_back(osFont); // osFonts manages lifetime, so we can release our references

			struct : IEventCallback<const Fonts::GlyphDesc>
			{
				OSFont* font;
				size_t index = 0;
				void OnEvent(const Fonts::GlyphDesc& gd) override
				{
					struct : Imaging::IImagePopulator<GRAYSCALE>
					{
						size_t index;
						OSFont* font;
						void OnImage(const GRAYSCALE* pixels, int32 width, int32 height) override
						{
							font->array->WriteSubImage(index, pixels, { width, height });
						}
					} addImageToTextureArray;
					addImageToTextureArray.font = font;
					addImageToTextureArray.index = index++;
					font->arrayFont->GenerateImage(gd.charCode, addImageToTextureArray);
				}
			} addGlyphToTextureArray;

			addGlyphToTextureArray.font = &osFont;

			auto& font = *osFont.arrayFont;
			osFont.array->ResetWidth(font.Metrics().imgWidth, font.Metrics().imgHeight);
			osFont.array->Resize(font.NumberOfGlyphs());
			osFont.arrayFont->ForEachGlyph(addGlyphToTextureArray);

			return ID_FONT{ i + ID_FONT_OSFONT_OFFSET };
		}

		Vec2i EvalSpan(ID_FONT rawId, const fstring& text) const override
		{
			ID_FONT id = NormalizeFontId(rawId);
			int32 index = id.value - ID_FONT_OSFONT_OFFSET;
			if (index < 0 || index >= (int32)osFonts.size())
			{
				return { 0,0 }; // See interface definition for comments
			}

			auto& osFont = osFonts[index];
			auto& font = *osFont.arrayFont;

			auto& metrics = font.Metrics();
			UNUSED(metrics);

			GuiRectf clipRect{ -1e37f, -1e37f, 1e37f, 1e37f };
			ANON::HQTextBuilder builder(font, clipRect);

			builder.evaluateSpanOnly = true;
			builder.renderer = nullptr;

			struct : IHQTextJob
			{
				Vec2 span { 0, 0 };
				cstr text = nullptr;
				Vec2 startPos{ 0,0 };
				GuiRectf lastRect = { 0,0,0,0 };

				void Render(IHQTextBuilder& builder) override
				{
					builder.SetCursor(startPos);

					for (const char* p = text; *p != 0; p++)
					{
						builder.Write(*p, &lastRect);
						span.x = max(lastRect.right, span.x);
						span.y = max(-lastRect.top, span.y);
					}
				}
			} job;

			job.text = text;

			job.Render(builder);

			return Quantize({ job.span });
		}

		virtual ~DX11HQFontFonts()
		{
			for (auto& osFont : osFonts)
			{
				osFont.arrayFont->Free();
				osFont.array->Free();
			}
		}

		void Free() override
		{
			delete this;
		}

		const Fonts::ArrayFontMetrics& GetFontMetrics(ID_FONT rawId) const override
		{
			ID_FONT idFont = NormalizeFontId(rawId);

			int32 index = idFont.value - ID_FONT_OSFONT_OFFSET;
			if (index < 0 || index >= (int32)osFonts.size())
			{
				Throw(0, "DX11Renderer.GetFontMetrics - ID_FONT parameter did not specify a HQ font");
			}

			return osFonts[index].arrayFont->Metrics();
		}

		void RenderHQText(ID_FONT rawId, IHQTextJob& job, IGuiRenderContext::EMode mode, ID3D11DeviceContext& dc, const GuiRect& clipRect) override
		{
			ID_FONT id = NormalizeFontId(rawId);

			int32 index = id.value - ID_FONT_OSFONT_OFFSET;
			if (index < 0 || index >= (int32)osFonts.size())
			{
				Throw(0, "DX11Renderer.RenderHQTest - ID_FONT parameter did not specify a HQ font");
			}

			auto& osFont = osFonts[index];
			auto& font = *osFont.arrayFont;

			ANON::HQTextBuilder builder(font, Dequantize(clipRect));

			builder.evaluateSpanOnly = mode == IGuiRenderContext::EVALUATE_SPAN_ONLY;
			builder.renderer = &renderer;

			if (mode == IGuiRenderContext::RENDER)
			{
				renderer.FlushLayer();
			}

			job.Render(builder);

			if (mode == IGuiRenderContext::RENDER)
			{
				ID3D11ShaderResourceView* gtaViews[1] = { osFont.array->View() };
				dc.PSSetShaderResources(TXUNIT_GENERIC_TXARRAY, 1, gtaViews);
				renderer.ApplyHQFontsShader();
				renderer.FlushLayer();
				renderer.ApplyGuiShader();
			}
		}

		float zoomLevel = 1.0f;

		void SetZoomLevel(float zoomLevel) override
		{
			if (this->zoomLevel != zoomLevel && activeDC)
			{
				this->zoomLevel = zoomLevel;

				for (auto& font : osFonts)
				{
					DX11::OSFont replacement = DuplicateScaled(font, zoomLevel, device, *activeDC);
					font.array->Free();
					font.arrayFont->Free();
					font.array = replacement.array;
					font.arrayFont = replacement.arrayFont;
				}
			}
		}

		ID_FONT FindBestSmallerFont(ID_FONT idRawFont) const override
		{
			if (osFonts.empty())
			{
				return ID_FONT::Invalid();
			}

			ID_FONT idFont = NormalizeFontId(idRawFont);

			int32 index = idFont.value - ID_FONT_OSFONT_OFFSET;
			if (index < 0 || index >= (int32)osFonts.size())
			{
				return ID_FONT::Invalid();
			}

			auto srcFontMetrics = osFonts[index].arrayFont->Metrics();
			int32 fontHeight = srcFontMetrics.height;

			int bestIndex = -1;
			int bestHeight = 0;

			// Search for the next smaller font with the same family
			for (size_t i = 0; i < osFonts.size(); i++)
			{
				auto& metrics = osFonts[i].arrayFont->Metrics();
				if (i != index && EqI(osFonts[i].spec.fontName, osFonts[index].spec.fontName))
				{
					if (metrics.height < fontHeight && metrics.height > bestHeight)
					{
						bestHeight = metrics.height;
						bestIndex = (int) i;
					}
				}
			}

			return bestIndex == -1 ? ID_FONT::Invalid() : ID_FONT{ (int) (ID_FONT_OSFONT_OFFSET + bestIndex) };
		}

		ID_FONT FindSmallestFont() const override
		{
			if (osFonts.empty())
			{
				return ID_FONT::Invalid();
			}

			size_t bestIndex = 0;
			size_t smallestHeight = osFonts[0].spec.height;

			for (size_t i = 1; i < osFonts.size(); i++)
			{
				if (osFonts[i].spec.height < smallestHeight)
				{
					smallestHeight = osFonts[i].spec.height;
					bestIndex = i;
				}
			}

			return ID_FONT((int) (ID_FONT_OSFONT_OFFSET + bestIndex));
		}
	};

	IDX11HQFontResource* CreateDX11HQFonts(IDX11FontRenderer& renderer, ID3D11Device& device, ID3D11DeviceContext& dc)
	{
		return new DX11HQFontFonts(renderer, device, dc);
	}
}