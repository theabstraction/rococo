#include <rococo.mplat.h>
#include <rococo.strings.h>
#include <vector>

#include <sexy.script.h>
#include <sexy.vm.h>
#include <sexy.vm.cpu.h>
#include <unordered_map>

using namespace Rococo;
using namespace Rococo::Graphics;
using namespace Rococo::Script;

struct GlyphSpec
{
	char filename[16];
	int32 a;
	uint32 b;
	int32 c;
	mutable int32 textureIndex;
};

struct ArrayFont
{
	ArrayFont() {}
	FontMetrics metrics { 0 };
	ID_TEXTURE arrayTextureId { 0 };
	U8FilePath pingPath = { "",'/' };
	
	std::unordered_map<char32_t, GlyphSpec> glyphs;
	enum { FONT_NAME_CAPACITY = 64 };
	char fontName[FONT_NAME_CAPACITY] = "";
};

inline ObjectStub* InterfaceToInstance(InterfacePointer i)
{
	auto* p = ((uint8*)i) + (*i)->OffsetToInstance;
	auto* obj = (ObjectStub*)p;
	return obj;
}

cstr GetLastFolder(const fstring& pingPath)
{
	for (int32 i = pingPath.length - 2; i >= 0; i--)
	{
		if (pingPath.buffer[i] == '/')
		{
			return pingPath.buffer + i + 1;
		}
	}

	return nullptr;
}

ID_TEXTURE LoadImagesIntoArray(const ArrayFont& font, IRenderer& renderer, IInstallation& installation)
{
	using namespace Rococo::IO;

	struct : ITextureLoadEnumerator
	{
		const ArrayFont* font;
		IInstallation* installation;
		AutoFree<IExpandingBuffer> buffer{ CreateExpandingBuffer(128 * 1024) };
		void ForEachElement(IEventCallback<TextureLoadData>& callback, bool readData) override
		{
			int index = 0;
			for (auto& g : font->glyphs)
			{
				// The order with which we enumerate will determine the index of the texture slices for each glyph
				if (g.second.textureIndex < 0)
				{
					g.second.textureIndex = index;
				}

				index++;

				U8FilePath imgPath;
				SafeFormat(imgPath.buf, imgPath.CAPACITY, "%s%s", font->pingPath.buf, g.second.filename);

				TextureLoadData tld = { 0 };
				tld.filename = imgPath;

				if (readData)
				{
					buffer->Resize(0);
					installation->LoadResource(imgPath, *buffer, 128_kilobytes);
					tld.nBytes = buffer->Length();
					tld.pData = buffer->GetData();
				}

				callback.OnEvent(tld);
			}
		}
	} enumerator;

	enumerator.font = &font;
	enumerator.installation = &installation;

	Vec2i span{ font.metrics.imgWidth, font.metrics.imgHeight };

	return renderer.LoadAlphaTextureArray(font.fontName, span, (int32) font.glyphs.size(), enumerator);
}

struct ArrayFonts : public IArrayFontsSupervisor, public IEventCallback<ScriptCompileArgs>, public IHQFont
{
	ID_FONT devFontId;
	std::vector<ArrayFont> fonts;
	IInstallation& installation;
	IUtilitiies& utils;
	IRenderer& renderer;

	ArrayFonts(IInstallation& _installation, IUtilitiies& _utils, IRenderer& _renderer):
		installation(_installation), utils(_utils), renderer(_renderer)
	{

	}

	~ArrayFonts()
	{

	}

	void Free() override
	{
		delete this;
	}

	void AddGlyph(char32_t charCode, cstr filename, int32 a, uint32 b, int32 c)
	{
		if (filename == nullptr)
		{
			Throw(0, "ArrayFonts.AddGlyph: null filename");
		}

		if (charCode < 32)
		{
			Throw(0, "ArrayFonts.AddGlyph: charCode %d not allowed. Codes must be >= 32", charCode);
		}

		auto& f = fonts.back();

		GlyphSpec spec;
		SafeFormat(spec.filename, sizeof(spec.filename), "%s", filename);
		spec.a = a;
		spec.b = b;
		spec.c = c;
		spec.textureIndex = -1;
		f.glyphs[charCode] = spec;

		if (!EndsWith(filename, ".tiff"))
		{
			Throw(0, "ArrayFonts.AddGlyph: charCode %d: filename must have extension .tiff: %s", filename);
		}
	}

	enum { DEFAULT_CHAR = U'?' };

	GlyphData GetGlyphData(ID_FONT id, char c) const override
	{
		// This function is called by the renderer which calls GetFontHeight first
		// Since GetFontHeight has validated the id, we do not have to test the id here
		// Not testing the id or fonts.size() speeds the function a little

		int index = id.value % fonts.size();
		auto& g = fonts[index].glyphs;

		auto i = g.find(c);
		if (i == g.end())
		{
			i = g.find(DEFAULT_CHAR);
		}

		auto& glyph = i->second;
		return GlyphData{ glyph.textureIndex, glyph.a, glyph.b, glyph.c };
	}

	Vec2i GetFontCellSpan(ID_FONT id, ID_TEXTURE& fontArrayId) const override
	{
		if (fonts.empty()) Throw(0, "ArrayFonts.GetFontHeight -> no fonts loaded");
		if (id.value < 0)  Throw(0, "ArrayFonts.GetFontHeight -> -ve font Id");
		int index = id.value % fonts.size();
		fontArrayId = fonts[index].arrayTextureId;
		return { fonts[index].metrics.imgWidth, fonts[index].metrics.height };
	}

	bool IsFontIdMapped(ID_FONT id) const override
	{
		return !fonts.empty() && id.value >= 0;
	}

	IHQFont* HQ() override
	{
		return this;
	}

	void OnEvent(ScriptCompileArgs& args) override
	{
		AddNativeCalls_RococoIArrayFonts(args.ss, this);

		auto& ns = args.ss.AddNativeNamespace("Rococo.Graphics");
		
		struct ANON
		{
			static void DeclareFontMetrics(Rococo::Script::NativeCallEnvironment& e)
			{
				auto& f = *(ArrayFonts*) e.context;
				FontMetrics* metrics;
				ReadInput(0, metrics, e);
				f.fonts.back().metrics = *metrics;
			}

			static void AddGlyph(Rococo::Script::NativeCallEnvironment& e)
			{
				auto& f = *(ArrayFonts*)e.context;
				int32 charCode;
				ReadInput(0, charCode, e);

				InterfacePointer pFilename;
				ReadInput(1, pFilename, e);

				CStringConstant* sc = (CStringConstant*) InterfaceToInstance(pFilename);

				int32 a, b, c;

				ReadInput(2, a, e);
				ReadInput(3, b, e);
				ReadInput(4, c, e);

				if (b < 0)
				{
					Throw(0, "ArrayFonts.AddGlyph: charCode %d: negative b not allowed", charCode);
				}

				f.AddGlyph(charCode, sc->pointer, a, b, c);
			}
		};
		
		args.ss.AddNativeCall(ns, ANON::DeclareFontMetrics, this, 
			"DeclareFontMetrics (Rococo.Graphics.FontMetrics metrics)->", true);

		args.ss.AddNativeCall(ns, ANON::AddGlyph, this,
			"AddGlyph (Int32 charCode)(IString filename)(Int32 a)(Int32 b)(Int32 c)->", true);
	}

	ID_FONT FindFontByPingPath(const fstring& pingPathToFolder)
	{
		if (pingPathToFolder.length < 2 || pingPathToFolder[0] != '!')
		{
			Throw(0, "ArrayFonts.FindFontByPingPath '%s' - folder name must start with !", pingPathToFolder.buffer);
		}

		if (!EndsWith(pingPathToFolder, "/"))
		{
			Throw(0, "ArrayFonts.FindFontByPingPath '%s' - folder name must end with /", pingPathToFolder.buffer);
		}

		for (int32 i = 0; i < fonts.size(); ++i)
		{
			if (Eq(fonts[i].pingPath, pingPathToFolder))
			{
				return ID_FONT{ i };
			}
		}

		return ID_FONT::Invalid();
	}

	// Use rococo.font_array_gen to generate the images and script file from a Windows font
	// ...and make sure the font owner gives permission to share the font!
	ID_FONT LoadFont(const fstring& pingPathToFolder) override
	{
		ID_FONT idOfExistingFont = FindFontByPingPath(pingPathToFolder);
		if (idOfExistingFont) return idOfExistingFont;

		// It should be obvious here ID_FONT zero corresponds to first element of fonts array and so on
		ID_FONT id{ (int32) fonts.size() };
		fonts.push_back(ArrayFont{});

		try
		{
			auto& f = fonts.back();

			SafeFormat(f.pingPath.buf, f.pingPath.CAPACITY, "%s", (cstr) pingPathToFolder);

			cstr container = GetLastFolder(pingPathToFolder);

			if (container == nullptr)
			{
				Throw(0, "ArrayFonts.LoadFont: Could not ascertain the final folder name in the ping path %s", pingPathToFolder.buffer);
			}

			size_t startLen = container - pingPathToFolder.buffer;
			if (pingPathToFolder.length - startLen > ArrayFont::FONT_NAME_CAPACITY)
			{
				Throw(0, "ArrayFonts.LoadFont: ping path final folder name too long: %s", pingPathToFolder.buffer);
			}

			SafeFormat(f.fontName, ArrayFont::FONT_NAME_CAPACITY, "%s", container);
			f.fontName[strlen(f.fontName)-1] = 0; // Eliminate the trailing slash

			U8FilePath fontScript;
			SafeFormat(fontScript.buf, fontScript.CAPACITY, "%sdesc.sxy", pingPathToFolder.buffer);

			// The fontScript will issue callbacks to AddGlyph and DeclareFontMetrics (see above)
			utils.RunEnvironmentScript(*this, fontScript, false);

			if (f.glyphs.size() == 0)
			{
				Throw(0, "The font script did not add glyphs");
			}

			if (f.metrics.height == 0)
			{
				Throw(0, "The font script did not set the font height");
			}

			if (f.glyphs.find(DEFAULT_CHAR) == f.glyphs.end())
			{
				Throw(0, "No default character '%c' in the font", DEFAULT_CHAR);
			}

			f.arrayTextureId = LoadImagesIntoArray(f, renderer, installation);
			return id;
		}
		catch (IException&)
		{
			fonts.pop_back();
			throw;
		}
	}

	int32 FontCount() override
	{
		return (int32) fonts.size();
	}

	void GetFontMetrics(ID_FONT fontId, FontMetrics& metrics) override
	{
		if (fonts.empty()) Throw(0, "ArrayFonts.GetFontMetrics: No fonts are loaded");
		if (fontId.value < 0) Throw(0, "ArrayFonts.GetFontMetrics: -ve font id");

		int32 index = fontId.value % fonts.size();
		metrics = fonts[index].metrics;
	}

	void AppendFontName(ID_FONT fontId, IStringPopulator& sb) override
	{
		if (fonts.empty()) Throw(0, "ArrayFonts.AppendFontName: No fonts are loaded");
		if (fontId.value < 0) Throw(0, "ArrayFonts.AppendFontName: -ve font id");
		int32 index = fontId.value % fonts.size();
		sb.Populate(fonts[index].fontName);
	}

	ID_FONT DevFont()  override
	{
		return devFontId;
	}

	void MarkDevFont(ID_FONT id) override
	{
		devFontId = id;
	}
};

namespace Rococo
{
	IArrayFontsSupervisor* CreateArrayFonts(IInstallation& installion, IUtilitiies& utils, IRenderer& renderer)
	{
		return new ArrayFonts(installion, utils, renderer);
	}
}