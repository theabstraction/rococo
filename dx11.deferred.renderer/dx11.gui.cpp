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
	class SpanEvaluator : public Fonts::IGlyphRenderer
	{
	public:
		GuiRectf renderZone;

		SpanEvaluator() : renderZone(10000, 10000, -10000, -10000)
		{

		}

		void DrawGlyph(cr_vec2 t0, cr_vec2 t1, cr_vec2 p0, cr_vec2 p1, Fonts::FontColour colour) override
		{
			ExpandZoneToContain(renderZone, p0);
			ExpandZoneToContain(renderZone, p1);
		}

		Vec2 Span() const
		{
			return Rococo::Span(renderZone);
		}
	};

	bool operator == (const Fonts::FontSpec& a, const Fonts::FontSpec& b)
	{
		return
			Eq(a.fontName, b.fontName) &&
			a.height == b.height &&
			a.italic == b.italic &&
			a.weight == b.weight;
	}


	struct GuiRenderPhase :
		IGuiRenderPhasePopulator,
		public IGuiRenderContext,
		public IRendererMetrics,
		public IHQFontResource,
		public IGlyphRenderer
	{
		IDX11System& system;
		IRenderStage& stage;

		std::vector<GuiVertex> guiVertices;
		MeshIndex idGuiMesh;
		LayoutId idGuiVertex;
		MeshIndex idGlobalStateConstants;

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

		enum { MAX_TRIANGLE_BATCH_COUNT = 1024 };

		AutoFree<Fonts::IFontSupervisor> scalableFonts;
		TextureId idScalableFontTexture;

		TextureId idSprites;

		GlobalState globalState;

		GuiRenderPhase(IDX11System& ref_system, IRenderStage& ref_stage, IInstallation& installation, TextureId valIdSprites) :
			system(ref_system), stage(ref_stage), idSprites(valIdSprites)
		{
			guiVertices.reserve(1024);

			hqFonts = CreateHQFonts(*this);

			idGuiPS = system.Shaders().AddPixelShader("!shaders/gui.ps.hlsl");
			idGuiVS = system.Shaders().AddVertexShader("!shaders/gui.vs.hlsl");
			idHQPS  = system.Shaders().AddPixelShader("!shaders/HQ-font.ps.hlsl");
			idHQVS  = system.Shaders().AddVertexShader("!shaders/HQ-font.vs.hlsl");

			auto& m = system.Meshes().GetPopulator();

			static_assert(sizeof(GuiVertex) == 40);
			static_assert(sizeof(GuiTriangle) == 120);

			auto& lb = m.LayoutBuilder();
			lb.Clear();
			lb.AddFloat2(HLSL_Semantic::POSITION, 0); // vec2
			lb.AddFloat3(HLSL_Semantic::TEXCOORD, 0); // vd
			lb.AddFloat4(HLSL_Semantic::TEXCOORD, 1); // sd
			lb.AddRGBAb(0); // colour
			idGuiVertex = lb.Commit("GuiVertex"_fstring);

			m.Clear();
			m.Reserve(sizeof GuiVertex, MAX_TRIANGLE_BATCH_COUNT * 3);
			for (int i = 0; i < MAX_TRIANGLE_BATCH_COUNT; ++i)
			{
				GuiVertex v = { 0 };
				m.AddVertex(&v, sizeof GuiVertex);
			}
			idGuiMesh = m.CommitDynamicVertexBuffer<GuiVertex>("gui.triangles"_fstring, "GuiVertex"_fstring);
			idGlobalStateConstants = m.CommitDynamicConstantBuffer("global.state"_fstring, &globalState, sizeof globalState);

			idScalableFontTexture = system.Textures().AddTx2D_Grey("!font1.tif");
			system.Textures().ReloadAsset(idScalableFontTexture);

			cstr csvName = "!font1.csv";

			AutoFree<IExpandingBuffer> csvBuf = CreateExpandingBuffer(0);
			installation.LoadResource(csvName, *csvBuf, 256_kilobytes);
			scalableFonts = Fonts::LoadFontCSV(csvName, (const char*)csvBuf->GetData(), csvBuf->Length());

			stage.ApplyConstantToPS(idGlobalStateConstants, CBUFFER_INDEX_GLOBAL_STATE);
			stage.ApplyConstantToVS(idGlobalStateConstants, CBUFFER_INDEX_GLOBAL_STATE);
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

			char hqName[64];
			SafeFormat(hqName, "HQFONT_%d", idFont.value);

			Vec2i span { new_Font->Metrics().imgWidth, new_Font->Metrics().imgWidth };
			auto idFontArrayId = system.Textures().AddTx2DArray_Grey(hqName, span);
			system.Textures().InitAsBlankArray(idFontArrayId, new_Font->NumberOfGlyphs());

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

		IPrepForDraw* prep;

		void RenderStage(IPrepForDraw& prep) override
		{
			if (!scene) return;

			this->prep = &prep;

			scene->RenderGui(*this);

			FlushLayer();
		}

		void AddTriangle(const GuiVertex triangle[3]) override
		{
			guiVertices.push_back(triangle[0]);
			guiVertices.push_back(triangle[1]);
			guiVertices.push_back(triangle[2]);

			if (guiVertices.size() == MAX_TRIANGLE_BATCH_COUNT * 3)
			{
				FlushLayer();
			}
		}

		void DrawCustomTexturedMesh(const GuiRect& absRect, ID_TEXTURE id, cstr pixelShader, const GuiVertex* vertices, size_t nCount) override
		{

		}

		void FlushLayer() override
		{
			if (!guiVertices.empty())
			{
				auto& m = system.Meshes().GetPopulator();
				m.UpdateDynamicVertexBuffer<GuiVertex>(idGuiMesh, guiVertices.data(), guiVertices.size());
				system.Meshes().ApplyVertexBuffer(idGuiMesh, 0);
				system.Meshes().Draw(idGuiMesh, 0, (uint32)guiVertices.size());

				guiVertices.clear();
			}
		}

		Vec2i EvalSpan(const Vec2i& pos, Fonts::IDrawTextJob& job, const GuiRect* clipRect) override
		{
			char stackBuffer[128];
			SpanEvaluator spanEvaluator;
			Fonts::IGlyphRenderPipeline* pipeline = Fonts::CreateGlyphRenderPipeline(stackBuffer, sizeof(stackBuffer), spanEvaluator);

			GuiRectf qrect(-10000.0f, -10000.0f, 10000.0f, 10000.0f);

			if (clipRect != nullptr)
			{
				qrect.left = (float)clipRect->left;
				qrect.right = (float)clipRect->right;
				qrect.top = (float)clipRect->top;
				qrect.bottom = (float)clipRect->bottom;
			}
			RouteDrawTextBasic(pos, job, *scalableFonts, *pipeline, qrect);

			Vec2i span = Quantize(spanEvaluator.Span());
			if (span.x < 0) span.x = 0;
			if (span.y < 0) span.y = 0;
			return span;
		}

		void RenderText(const Vec2i& pos, Fonts::IDrawTextJob& job, const GuiRect* clipRect) override
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
			RouteDrawTextBasic(pos, job, *scalableFonts, *pipeline, qrect);
		}

		IRendererMetrics& Renderer() override
		{
			return *this;
		}

		void GetGuiMetrics(GuiMetrics& metrics) const override
		{
			metrics = this->metrics;
		}

		GuiMetrics metrics{ {0,0},{0,0} };

		void UpdateCursor(Vec2i cursorPos) override
		{
			metrics.cursorPosition = cursorPos;
		}

		void SetTargetSpan(Vec2i span) override
		{
			metrics.screenSpan = span;
			globalState = GlobalState{ 0 };

			globalState.guiScale.OOScreenWidth = 1.0f / span.x;
			globalState.guiScale.OOScreenHeight = 1.0f / span.y;

			Vec2i spriteSpan = system.Textures().GetSpan(idSprites);
			Vec2i scalableFontSpan = system.Textures().GetSpan(idScalableFontTexture);
			globalState.guiScale.OOFontWidth = 1.0f / scalableFontSpan.x;
			globalState.guiScale.OOSpriteWidth = 1.0f / spriteSpan.x;

			auto& m = system.Meshes().GetPopulator();
			m.UpdateDynamicConstantBuffer(idGlobalStateConstants, globalState);

			stage.SetViewport({ 0,0 }, Dequantize(span), 0, 1);
			stage.SetEnableScissors(nullptr);

			system.UseShaders(idGuiVertex, idGuiVS, idGuiPS);
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

			if (mode == IGuiRenderContext::EVALUATE_SPAN_ONLY)
			{
				job.Render(builder);
			}
			else
			{
				FlushLayer();

				if (system.Textures().AssignTextureToShaders(osFont.idOSFontTextureArray, TXUNIT_GENERIC_TXARRAY))
				{
					if (system.UseShaders(idGuiVertex, idHQVS, idHQPS))
					{
						prep->OnShaderChange();
						job.Render(builder);
						FlushLayer();
						system.UseShaders(idGuiVertex, idGuiVS, idGuiPS);
						prep->OnShaderChange();
					}
				}
			}
		}

		void GetMaterialArrayMetrics(MaterialArrayMetrics& metrics) const override
		{
			metrics.NumberOfElements = 0;
			metrics.Width = 1024;
		}

		const Fonts::ArrayFontMetrics& GetFontMetrics(ID_FONT idFont)  override
		{
			int32 index = idFont.value - ID_FONT_OSFONT_OFFSET;
			if (index < 0 || index >= (int32)osFonts.size())
			{
				Throw(0, "%s: ID_FONT parameter was unknown value", __FUNCTION__);
			}

			return osFonts[index].arrayFont->Metrics();
		}

		Textures::ITextureArrayBuilder& SpriteBuilder() override
		{
			return system.Textures().GetSpriteBuilder(idSprites);
		}

		Fonts::IFont& FontMetrics() override
		{
			return *scalableFonts;
		}

		void DrawGlyph(cr_vec2 uvTopLeft, cr_vec2 uvBottomRight, cr_vec2 posTopLeft, cr_vec2 posBottomRight, Fonts::FontColour fcolour)
		{
			float x0 = posTopLeft.x;
			float y0 = posTopLeft.y;
			float x1 = posBottomRight.x;
			float y1 = posBottomRight.y;

			float u0 = uvTopLeft.x;
			float v0 = uvTopLeft.y;
			float u1 = uvBottomRight.x;
			float v1 = uvBottomRight.y;

			struct CLOSURE
			{
				static RGBAb FontColourToSysColour(Fonts::FontColour colour)
				{
					RGBAb* pCol = (RGBAb*)&colour;
					return *pCol;
				}
			};

			RGBAb colour = CLOSURE::FontColourToSysColour(fcolour);

			SpriteVertexData drawFont{ 1.0f, 0.0f, 0.0f, 1.0f };

			GuiTriangle t1
			{
				GuiVertex{ {x0, y0}, {{ u0, v0}, 1 }, drawFont, (RGBAb)colour },
				GuiVertex{ {x0, y1}, {{ u0, v1}, 1 }, drawFont, (RGBAb)colour },
				GuiVertex{ {x1, y1}, {{ u1, v1}, 1 }, drawFont, (RGBAb)colour }
			};

			GuiTriangle t2
			{
				GuiVertex{ {x1, y1}, {{ u1, v1}, 1 }, drawFont, (RGBAb)colour },
				GuiVertex{ {x1, y0}, {{ u1, v0}, 1 }, drawFont, (RGBAb)colour },
				GuiVertex{ {x0, y0}, {{ u0, v0}, 1 }, drawFont, (RGBAb)colour } // bottomRight
			};

			AddTriangle(&t1.a);
			AddTriangle(&t2.a);
		}

		IHQFonts& HQFonts() override
		{
			return *hqFonts;
		}
	};
}

namespace Rococo::Graphics
{
	IGuiRenderPhasePopulator* CreateStandardGuiRenderPhase(IDX11System& system, IRenderStage& stage, IInstallation& installation, TextureId idSprites)
	{
		return new ANON::GuiRenderPhase(system, stage, installation, idSprites);
	}

	void ConfigueStandardGuiStage(IRenderStageSupervisor& stage, const GuiRect& scissorRect, TextureId idSprites, TextureId idScalableFontTexture)
	{
		stage.SetEnableScissors(&scissorRect);

		stage.SetDepthWriteEnable(false);
		stage.SetEnableDepth(false);
		stage.SetSrcBlend(BlendValue::SRC_ALPHA);
		stage.SetDestBlend(BlendValue::INV_SRC_ALPHA);
		stage.SetEnableBlend(true);
		stage.SetBlendOp(BlendOp::ADD);
		stage.SetCullMode(0);

		Sampler spriteDef;
		spriteDef.filter = SamplerFilter::MIN_MAG_MIP_POINT;
		spriteDef.maxAnisotropy = 1;
		spriteDef.comparison = ComparisonFunc::NEVER;
		spriteDef.maxLOD = 1000000.0f;
		spriteDef.minLOD = 0;
		spriteDef.mipLODBias = 0;
		spriteDef.U = SamplerAddressMode::BORDER;
		spriteDef.V = SamplerAddressMode::BORDER;
		spriteDef.W = SamplerAddressMode::BORDER;
		stage.AddInput(idSprites, TXUNIT_SPRITES, spriteDef);

		stage.AddInput(TextureId(), TXUNIT_MATERIALS, spriteDef);

		Sampler fontDef;
		fontDef.filter = SamplerFilter::MIN_MAG_MIP_LINEAR;
		fontDef.maxAnisotropy = 1;
		fontDef.comparison = ComparisonFunc::ALWAYS;
		fontDef.maxLOD = 1000000.0f;
		fontDef.minLOD = 0;
		fontDef.mipLODBias = 0;
		fontDef.U = SamplerAddressMode::BORDER;
		fontDef.V = SamplerAddressMode::BORDER;
		fontDef.W = SamplerAddressMode::BORDER;
		stage.AddInput(idScalableFontTexture, TXUNIT_FONT, fontDef);

		Sampler hqDef;
		hqDef.filter = SamplerFilter::MIN_MAG_MIP_POINT;
		hqDef.maxAnisotropy = 1;
		hqDef.comparison = ComparisonFunc::ALWAYS;
		hqDef.maxLOD = 1000000.0f;
		hqDef.minLOD = 0;
		hqDef.mipLODBias = 0;
		hqDef.U = SamplerAddressMode::BORDER;
		hqDef.V = SamplerAddressMode::BORDER;
		hqDef.W = SamplerAddressMode::BORDER;

		TextureId null;
		stage.AddInput(null, TXUNIT_GENERIC_TXARRAY, hqDef);
	}
}

