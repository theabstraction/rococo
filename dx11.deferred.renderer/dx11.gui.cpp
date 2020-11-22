#include <rococo.api.h>
#include <rococo.os.win32.h>
#include "rococo.dx11.h"
#include <rococo.auto-release.h>
#include <rococo.os.h>
#include <rococo.fonts.h>
#include <rococo.fonts.hq.h>
#include <rococo.DirectX.h>
#include <rococo.mplat.h>
#include <rococo.strings.h>
#include <vector>
#include <d3d11_4.h>

using namespace Rococo;
using namespace Rococo::Fonts;
using namespace Rococo::Graphics;

namespace ANON
{
	bool operator == (const Fonts::FontSpec& a, const Fonts::FontSpec& b)
	{
		return
			Eq(a.fontName, b.fontName) &&
			a.height == b.height &&
			a.italic == b.italic &&
			a.weight == b.weight;
	}


	struct GuiRenderPhase :
		IRenderPhasePopulator,
		public IGuiRenderContext,
		public IRendererMetrics,
		public IHQFontResource
	{
		IDX11System& system;
		IRenderStage& stage;

		std::vector<GuiTriangle> guiTriangles;
		MeshIndex idGuiMesh;
		LayoutId idGuiVertex;

		IScene* scene = nullptr;

		ID_VERTEX_SHADER idGuiVS;
		ID_PIXEL_SHADER idGuiPS;
		ID_VERTEX_SHADER idHQVS;
		ID_PIXEL_SHADER idHQPS;

		AutoFree<Graphics::IHQFontsSupervisor> hqFonts;

		enum { ID_FONT_OSFONT_OFFSET = 400 };

		struct OSFont
		{
			Fonts::IArrayFontSupervisor* arrayFont;
			Fonts::FontSpec spec;
			TextureId idOSFontTextureArray;
		};

		std::vector<OSFont> osFonts;

		enum { MAX_TRIANGLE_BATCH_COUNT = 4096 };

		GuiRenderPhase(IDX11System& ref_system, IRenderStage& ref_stage) :
			system(ref_system), stage(ref_stage)
		{
			guiTriangles.reserve(1024);

			hqFonts = CreateHQFonts(*this);

			idGuiPS = system.Shaders().AddPixelShader("!shaders/gui.ps.hlsl");
			idGuiVS = system.Shaders().AddVertexShader("!shaders/gui.vs.hlsl");
			idHQPS  = system.Shaders().AddPixelShader("!shaders/HQ-font.ps.hlsl");
			idHQVS  = system.Shaders().AddVertexShader("!shaders/HQ-font.vs.hlsl");

			auto& m = system.Meshes().GetPopulator();
			auto& lb = m.LayoutBuilder();

			static_assert(sizeof(GuiVertex) == 40);

			lb.Clear();
			lb.AddFloat(HLSL_Semantic::POSITION, 0); // vec2
			lb.AddFloat3(HLSL_Semantic::TEXCOORD, 0); // vd
			lb.AddFloat4(HLSL_Semantic::TEXCOORD, 1); // sd
			lb.AddRGBAb(0); // colour
			idGuiVertex = lb.Commit("GuiVertex"_fstring);

			m.Clear();
			m.Reserve(sizeof GuiTriangle, MAX_TRIANGLE_BATCH_COUNT);
			for (int i = 0; i < MAX_TRIANGLE_BATCH_COUNT; ++i)
			{
				GuiTriangle t = { 0 };
				m.AddVertex(&t, sizeof GuiTriangle);
			}
			idGuiMesh = m.CommitDynamicVertexBuffer<GuiTriangle>("gui_triangles"_fstring, "GuiVertex"_fstring);
		}

		virtual ~GuiRenderPhase()
		{
			for (auto& os : osFonts)
			{
				os.arrayFont->Free();
			}
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

			ID_FONT idFont ( i + ID_FONT_OSFONT_OFFSET );

			AutoFree<Fonts::IArrayFontSupervisor> new_Font = Fonts::CreateOSFont(glyphs, spec);
			AutoRelease<ID3D11Texture2D1> fontArray;

			char hqName[64];
			SafeFormat(hqName, "HQFONT_%d", idFont.value);

			Vec2i span { new_Font->Metrics().imgWidth, new_Font->Metrics().imgWidth };
			auto idFontArrayId = system.Textures().AddTx2DArray_Grey(hqName, span);

			OSFont osFont{ new_Font, spec , idFontArrayId };
			osFonts.push_back(osFont);
			new_Font.Release();

			struct : IEventCallback<const Fonts::GlyphDesc>
			{
				OSFont* font;
				size_t index = 0;
				TextureId idFontArrayId;
				IDX11System* system;

				void OnEvent(const Fonts::GlyphDesc& gd) override
				{
					struct : IImagePopulator<GRAYSCALE>
					{
						size_t index;
						OSFont* font;
						IDX11System* system;
						TextureId idFontArrayId;
						void OnImage(const GRAYSCALE* pixels, int32 width, int32 height) override
						{
							system->Textures().UpdateArray(idFontArrayId, (uint32) index, pixels, Vec2i{ width, height });
						}
					} addImageToTextureArray;
					addImageToTextureArray.system = system;
					addImageToTextureArray.font = font;
					addImageToTextureArray.index = index++;
					addImageToTextureArray.idFontArrayId = idFontArrayId;
					font->arrayFont->GenerateImage(gd.charCode, addImageToTextureArray);
				}
			} addGlyphToTextureArray;

			addGlyphToTextureArray.font = &osFont;
			addGlyphToTextureArray.idFontArrayId = idFontArrayId;
			addGlyphToTextureArray.system = &system;

			auto& font = *osFont.arrayFont;
			osFont.arrayFont->ForEachGlyph(addGlyphToTextureArray);

			return ID_FONT{ i + ID_FONT_OSFONT_OFFSET };
		}

		void SetScene(IScene* scene) override
		{
			this->scene = scene;
		}

		void Free() override
		{
			delete this;
		}

		void RenderStage(IPainter& painter) override
		{
			if (!scene) return;

			scene->RenderGui(*this);

			FlushLayer();
		}

		void AddTriangle(const GuiVertex triangle[3]) override
		{
			guiTriangles.push_back((GuiTriangle&) triangle);

			if (guiTriangles.size() == MAX_TRIANGLE_BATCH_COUNT)
			{
				FlushLayer();
			}
		}

		void DrawCustomTexturedMesh(const GuiRect& absRect, ID_TEXTURE id, cstr pixelShader, const GuiVertex* vertices, size_t nCount) override
		{

		}

		void FlushLayer() override
		{
			auto& m = system.Meshes().GetPopulator();
			m.UpdateDynamicVertexBuffer<GuiTriangle>(idGuiMesh, guiTriangles.data(), guiTriangles.size());
			guiTriangles.clear();
		}

		Vec2i EvalSpan(const Vec2i& pos, Fonts::IDrawTextJob& job, const GuiRect* clipRect) override
		{
			return Vec2i{ 0,0 };
		}

		void RenderText(const Vec2i& pos, Fonts::IDrawTextJob& job, const GuiRect* clipRect) override
		{

		}

		IRendererMetrics& Renderer() override
		{
			return *this;
		}

		void GetGuiMetrics(GuiMetrics& metrics) const override
		{
			metrics.cursorPosition = { 0,0 };
			metrics.screenSpan = { 1024, 768 };
		}

		auto SelectTexture(ID_TEXTURE id)->Vec2i override
		{
			return { 0,0 };
		}

		void SetGuiShader(cstr pixelShader) override
		{
			FlushLayer();
		}

		void SetScissorRect(const Rococo::GuiRect& rect) override
		{
			stage.SetEnableScissors(&rect);
			FlushLayer();
		}

		void ClearScissorRect() override
		{
			stage.SetEnableScissors(nullptr);
			FlushLayer();
		}

		void RenderHQText(ID_FONT id, Fonts::IHQTextJob& job, EMode mode = RENDER) override
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

			system.Textures().AssignTextureToShaders(osFont.idOSFontTextureArray, TXUNIT_GENERIC_TXARRAY);

			struct CLOSURE : IHQTextBuilder
			{
				Fonts::IArrayFont& font;
				const Fonts::ArrayFontMetrics& metrics;
				RGBAb colour{ 255,255,255,255 };
				Vec2 span;
				GuiRectf nextRect;
				bool evaluateSpanOnly = false;
				GuiRenderPhase* This;

				CLOSURE(Fonts::IArrayFont& _font) :
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
							{ {R.left, R.top},    {{0,0}, 0}, {0, i, 0, 0}, colour},
							{ {R.right,R.top},    {{1,0}, 0}, {0, i, 0, 0}, colour},
							{ {R.left, R.bottom}, {{0,1}, 0}, {0, i, 0, 0}, colour},
							{ {R.right,R.bottom}, {{1,1}, 0}, {0, i, 0, 0}, colour},
						};

						GuiTriangle A{ q.topLeft, q.topRight, q.bottomRight };
						GuiTriangle B{ q.bottomRight, q.bottomLeft, q.topLeft };
						This->AddTriangle(&A.a);
						This->AddTriangle(&B.a);
					}

					if (outputBounds != nullptr)
					{
						*outputBounds = R;
					}

					R.left += g.B + g.C;
				}

			} builder(font);

			builder.evaluateSpanOnly = mode == IGuiRenderContext::EVALUATE_SPAN_ONLY;
			builder.This = this;

			if (mode == IGuiRenderContext::RENDER)
			{
				FlushLayer();
			}

			job.Render(builder);

			if (mode == IGuiRenderContext::RENDER)
			{
				if (system.UseShaders(idGuiVertex, idHQVS, idHQPS))
				{
					return;
				}

				FlushLayer();

				if (!system.UseShaders(idGuiVertex, idGuiVS, idGuiPS))
				{
					return;
				}
			}
		}

		void GetMaterialArrayMetrics(MaterialArrayMetrics& metrics) const override
		{
			metrics.NumberOfElements = 0;
			metrics.Width = 1024;
		}

		Fonts::ArrayFontMetrics GetFontMetrics(ID_FONT idFont)  override
		{
			Fonts::ArrayFontMetrics metrics = { 0 };
			return metrics;
		}

		Textures::ITextureArrayBuilder& SpriteBuilder() override
		{
			Textures::ITextureArrayBuilder* s = nullptr;
			return *s;
		}

		Fonts::IFont& FontMetrics() override
		{
			Fonts::IFont* font = nullptr;
			return *font;
		}
	};
}

namespace Rococo::Graphics
{
	IRenderPhasePopulator* CreateStandardGuiRenderPhase(IDX11System& system, IRenderStage& stage)
	{
		return new ANON::GuiRenderPhase(system, stage);
	}
}

