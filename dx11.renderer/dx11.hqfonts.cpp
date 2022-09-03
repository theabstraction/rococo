#include "dx11.renderer.h"
#include "rococo.fonts.hq.h"
#include <vector>

using namespace Rococo;
using namespace Rococo::Strings;
using namespace Rococo::Graphics::Fonts;

namespace Rococo::DX11
{
	struct OSFont
	{
		Fonts::IArrayFontSupervisor* arrayFont;
		Fonts::FontSpec spec;
		IDX11TextureArray* array;
	};

	bool operator == (const Fonts::FontSpec& a, const Fonts::FontSpec& b)
	{
		return
			Eq(a.fontName, b.fontName) &&
			a.height == b.height &&
			a.italic == b.italic &&
			a.weight == b.weight;
	}

	struct DX11HQFontFonts: IDX11HQFontResource
	{
		ID3D11Device& device;
		std::vector<OSFont> osFonts;
		IDX11FontRenderer& renderer;
		ID3D11DeviceContext* activeDC = nullptr;

		enum { ID_FONT_OSFONT_OFFSET = 400 };

		DX11HQFontFonts(IInstallation& installation, IDX11FontRenderer& _renderer, ID3D11Device& _device, ID3D11DeviceContext& dc) : renderer(_renderer), device(_device), activeDC(&dc)
		{
		}

		ID_FONT CreateOSFont(Fonts::IArrayFontSet& glyphs, const Fonts::FontSpec& spec) override
		{
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
			IDX11TextureArray* new_array2D = CreateDX11TextureArray(device, *activeDC);
			OSFont osFont{ new_Font, spec , new_array2D };
			osFonts.push_back(osFont); // osFonts manages lifetime, so we can release our references

			struct : IEventCallback<const Fonts::GlyphDesc>
			{
				OSFont* font;
				size_t index = 0;
				void OnEvent(const Fonts::GlyphDesc& gd) override
				{
					struct : IImagePopulator<GRAYSCALE>
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

		const Fonts::ArrayFontMetrics& GetFontMetrics(ID_FONT idFont) override
		{
			int32 index = idFont.value - ID_FONT_OSFONT_OFFSET;
			if (index < 0 || index >= (int32)osFonts.size())
			{
				Throw(0, "DX11Renderer.GetFontMetrics - ID_FONT parameter was unknown value");
			}

			return osFonts[index].arrayFont->Metrics();
		}

		void RenderHQText(ID_FONT id, IHQTextJob& job, IGuiRenderContext::EMode mode, ID3D11DeviceContext& dc, IShaders& shaders) override
		{
			int32 index = id.value - ID_FONT_OSFONT_OFFSET;
			if (index < 0 || index >= (int32)osFonts.size())
			{
				Throw(0, "DX11Renderer.RenderHQTest - ID_FONT parameter was unknown value");
			}

			auto& osFont = osFonts[index];
			auto& font = *osFont.arrayFont;

			auto& metrics = font.Metrics();

			Vec2i span{ metrics.imgWidth, metrics.imgHeight };

			ID3D11ShaderResourceView* gtaViews[1] = { osFont.array->View() };
			dc.PSSetShaderResources(TXUNIT_GENERIC_TXARRAY, 1, gtaViews);

			struct ANON : IHQTextBuilder
			{
				Fonts::IArrayFont& font;
				const Fonts::ArrayFontMetrics& metrics;
				RGBAb colour{ 255,255,255,255 };
				Vec2 span;
				GuiRectf nextRect;
				bool evaluateSpanOnly = false;
				IDX11FontRenderer* renderer;

				ANON(Fonts::IArrayFont& _font) :
					font(_font),
					metrics(_font.Metrics()),
					span{ (float)metrics.imgWidth, (float)metrics.imgHeight },
					nextRect{ 0, 0, span.x, span.y }
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
					nextRect = { pos.x, pos.y - span.y, span.x, pos.y };
				}

				void Write(char c, GuiRectf* outputBounds) override
				{
					Write((char32_t)(unsigned char)c, outputBounds);
				}

				void Write(wchar_t c, GuiRectf* outputBounds) override
				{
					Write((char32_t)c, outputBounds);
				}

				void Write(char32_t c, GuiRectf* outputBounds) override
				{
					const Fonts::ArrayGlyph& g = font[c];

					float i = (float)g.Index;

					auto& R = nextRect;

					R.left += g.A;
					R.right = R.left + span.x;

					if (!evaluateSpanOnly && colour.alpha > 0)
					{
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

							{ {R.left, R.top},    {{0,0}, 0}, {0, i, 0, 0}, colour},
							{ {R.right,R.top},    {{1,0}, 0}, {0, i, 0, 0}, colour},
							{ {R.left, R.bottom}, {{0,1}, 0}, {0, i, 0, 0}, colour},
							{ {R.right,R.bottom}, {{1,1}, 0}, {0, i, 0, 0}, colour},
						};

						GuiTriangle A{ q.topLeft, q.topRight, q.bottomRight };
						GuiTriangle B{ q.bottomRight, q.bottomLeft, q.topLeft };
						renderer->AddTriangle(&A.a);
						renderer->AddTriangle(&B.a);
					}

					if (outputBounds != nullptr)
					{
						*outputBounds = R;
					}

					R.left += g.B + g.C;
				}

			} builder(font);

			builder.evaluateSpanOnly = mode == IGuiRenderContext::EVALUATE_SPAN_ONLY;
			builder.renderer = &renderer;

			if (mode == IGuiRenderContext::RENDER)
			{
				renderer.FlushLayer();
			}

			job.Render(builder);

			if (mode == IGuiRenderContext::RENDER)
			{
				renderer.ApplyHQFontsShader();
				renderer.FlushLayer();
				renderer.ApplyGuiShader();
			}
		}

	};

	IDX11HQFontResource* CreateDX11HQFonts(IInstallation& installation, IDX11FontRenderer& renderer, ID3D11Device& device, ID3D11DeviceContext& dc)
	{
		return new DX11HQFontFonts(installation, renderer, device, dc);
	}
}