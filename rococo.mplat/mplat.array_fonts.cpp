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
	char filename[12];
	int32 a;
	uint32 b;
	int32 c;
	int32 textureIndex;
};

struct ArrayFont
{
	ArrayFont() {}
	FontMetrics metrics;
	ID_TEXTURE arrayTextureId;
	
	std::unordered_map<char32_t, GlyphSpec> glyphs;
	enum { FONT_NAME_CAPACITY = 64 };
	char fontName[FONT_NAME_CAPACITY];
};

inline ObjectStub* InterfaceToInstance(InterfacePointer i)
{
	auto* p = ((uint8*)i) + (*i)->OffsetToInstance;
	auto* obj = (ObjectStub*)p;
	return obj;
}

cstr GetLastFolder(const fstring& pingPath)
{
	for (size_t i = pingPath.length - 1; i >= 0; i--)
	{
		if (pingPath.buffer[i] == '/')
		{
			return pingPath.buffer + i;
		}
	}

	return nullptr;
}

struct ArrayFonts : public IArrayFontsSupervisor, public IEventCallback<ScriptCompileArgs>
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

	void LoadImagesIntoArray(ArrayFont& font)
	{

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

	ID_FONT LoadFont(const fstring& pingPathToFolder) override
	{
		if (pingPathToFolder.length == 0 || pingPathToFolder[0] != '!')
		{
			Throw(0, "ArrayFonts.LoadFont '%s' - folder name must start with !", pingPathToFolder.buffer);
		}

		if (!EndsWith(pingPathToFolder, "/"))
		{
			Throw(0, "ArrayFonts.LoadFont '%s' - folder name must end with /", pingPathToFolder.buffer);
		}

		ID_FONT id{ (int32) fonts.size() };
		fonts.push_back(ArrayFont{});

		auto& f = fonts.back();

		cstr container = GetLastFolder(pingPathToFolder);

		if (container == nullptr)
		{
			fonts.pop_back();
			Throw(0, "ArrayFonts.LoadFont: Could not ascertain the final folder name in the ping path %s", pingPathToFolder.buffer);
		}

		size_t startLen = container - pingPathToFolder.buffer;
		if (pingPathToFolder.length - startLen > ArrayFont::FONT_NAME_CAPACITY)
		{
			fonts.pop_back();
			Throw(0, "ArrayFonts.LoadFont: ping path final folder name too long: %s", pingPathToFolder.buffer);
		}

		SafeFormat(f.fontName, ArrayFont::FONT_NAME_CAPACITY, "%s", container);
		f.fontName[strlen(f.fontName)] = 0;

		U8FilePath fontScript;
		SafeFormat(fontScript.buf, fontScript.CAPACITY, "%sdesc.sxy", pingPathToFolder.buffer);

		try
		{
			utils.RunEnvironmentScript(*this, fontScript, false);
			LoadImagesIntoArray(f);
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

		int32 index = fontId.value % fonts.size();

		if (index < 0) Throw(0, "ArrayFonts.GetFontMetrics: -ve font index");
		metrics = fonts[index].metrics;
	}

	void AppendFontName(ID_FONT fontId, IStringPopulator& sb) override
	{
		if (fonts.empty()) Throw(0, "ArrayFonts.AppendFontName: No fonts are loaded");

		int32 index = fontId.value % fonts.size();

		if (index < 0) Throw(0, "ArrayFonts.AppendFontName: -ve font index");

		sb.Populate(fonts[index].fontName);
	}

	ID_FONT DevFont()  override
	{
		return devFontId;
	}

	void MarkDevFont(ID_FONT id) override
	{
		if (id.value < 0) Throw(0, "ArrayFonts.MarkDevFont: -ve font id");
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