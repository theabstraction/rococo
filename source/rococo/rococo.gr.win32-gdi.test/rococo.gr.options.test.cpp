#include <rococo.gr.client.h>
#include <rococo.gui.retained.h>
#include <rococo.reflector.h>
#include <rococo.strings.h>
#include <rococo.game.options.ex.h>

using namespace Rococo;
using namespace Rococo::Gui;
using namespace Rococo::Strings;
using namespace Rococo::Game::Options;

void RunMessageLoop(IGRClientWindow& client);
void BuildMenus(IGRWidgetMainFrame& frame);
void BuildUpperRightToolbar(IGRWidgetMainFrame& frame);
void UseTestColourScheme(IGRWidgetMainFrame& frame);

void TestOptions(IGRClientWindow& client, IGRSystem& gr, IGameOptions& options)
{
	GRIdWidget mainFrame{ "Main-Frame" };
	auto& frame = gr.BindFrame(mainFrame);
	//frame.SetTitleBarHeight(30);

	frame.ClientArea().Panel().Set(EGRSchemeColourSurface::CONTAINER_BACKGROUND, RGBAb(0, 255, 0, 0), GRGenerateIntensities());
	frame.MenuBar().Panel().Parent()->SetCollapsed(true);

	auto& scheme = gr.Root().Scheme();
	SetSchemeColours_ThemeGrey(scheme);
	UseTestColourScheme(frame);

//	BuildMenus(frame);

//	BuildUpperRightToolbar(frame);

	auto& viewport = CreateViewportWidget(frame.ClientArea().Widget());
	viewport.Panel().SetExpandToParentHorizontally().SetExpandToParentVertically();

	GameOptionConfig config;
	auto& optionsList = CreateGameOptionsList(viewport.ClientArea().Widget(), options, config);
	optionsList.Panel().Set(EGRSchemeColourSurface::BACKGROUND, RGBAb(255, 255, 255), GRGenerateIntensities());

	RunMessageLoop(client);
}

struct AudioOptions : IGameOptions
{
	OptionDatabase<AudioOptions> db;

	double musicVolume = 0.25;
	double fxVolume = 0.2;
	double narrationVolume = 0.4;
	HString speakerConfig = "2";

	IOptionDatabase& DB() override
	{
		return db;
	}

	AudioOptions() : db(*this)
	{

	}

	void GetMusicVolume(IScalarInquiry& inquiry)
	{
		inquiry.SetTitle("Music Volume");
		inquiry.SetRange(0, 100.0);
		inquiry.SetActiveValue(musicVolume);
	}

	void SetMusicVolume(double value)
	{
		musicVolume = value;
	}

	void GetFXVolume(IScalarInquiry& inquiry)
	{
		inquiry.SetTitle("FX Volume");
		inquiry.SetRange(0, 100.0);
		inquiry.SetActiveValue(fxVolume);
	}

	void SetFXVolume(double value)
	{
		fxVolume = value;
	}

	void GetNarrationVolume(IScalarInquiry& inquiry)
	{
		inquiry.SetTitle("Narration Volume");
		inquiry.SetRange(0, 100.0);
		inquiry.SetActiveValue(narrationVolume);
	}

	void SetNarrationVolume(double value)
	{
		narrationVolume = value;
	}

	void GetSpeakerConfiguration(IChoiceInquiry& inquiry)
	{
		inquiry.SetTitle("Speaker Configuration");
		inquiry.AddChoice("2", "2 (Stereo speakers)");
		inquiry.AddChoice("2_1", "2.1 (Stereo + Subwoofer)");
		inquiry.AddChoice("5_1", "5.1 Dolby Surround");
		inquiry.AddChoice("7_1", "7.1 Dolby Surround");
		inquiry.AddChoice("2H", "Stereo Headphones");
		inquiry.SetActiveChoice(speakerConfig);
	}

	void SetSpeakerConfiguration(cstr choice)
	{
		speakerConfig = choice;
	}

	void AddOptions(IGameOptionsBuilder& builder) override
	{
		ADD_GAME_OPTIONS(db, AudioOptions, MusicVolume)
		ADD_GAME_OPTIONS(db, AudioOptions, FXVolume)
		ADD_GAME_OPTIONS(db, AudioOptions, NarrationVolume)
		ADD_GAME_OPTIONS(db, AudioOptions, SpeakerConfiguration)
		db.Build(builder);
	}
};

struct GraphicsOptions: IGameOptions
{
	OptionDatabase<GraphicsOptions> db;

	IOptionDatabase& DB() override
	{
		return db;
	}

	HString activeScreenMode = "Fullscreen";
	HString shadowQuality = "2";
	HString landscapeQuality = "1";
	HString reflectionAlgorithm = "1";
	HString monitor = "1";
	HString resolution = "1920x1080";
	HString textureQuality = "1";
	HString waterQuality = "1";
	bool isFSAAEnabled = false;

	GraphicsOptions() : db(*this)
	{

	}

	void GetScreenMode(IChoiceInquiry& inquiry)
	{
		inquiry.SetTitle("Screen Mode");
		inquiry.AddChoice("Fullscreen", "Fullscreen");
		inquiry.AddChoice("Windowed", "Windowed");
		inquiry.AddChoice("Fullscreen Windowed", "Fullscreen Windowed");
		inquiry.SetActiveChoice(activeScreenMode);
	}

	void SetScreenMode(cstr choice)
	{
		activeScreenMode = choice;
	}

	void GetFSAA(IBoolInquiry& inquiry)
	{
		inquiry.SetTitle("Fullscreen Anti-Aliasing");
		inquiry.SetActiveValue(isFSAAEnabled);
	}

	void SetFSAA(bool value)
	{
		isFSAAEnabled = value;
	}

	void GetShadowQuality(IChoiceInquiry& inquiry)
	{
		inquiry.SetTitle("Shadow Quality");
		inquiry.AddChoice("1", "512 x 512 1pt (low)");
		inquiry.AddChoice("2", "1024 x 1024 4pt (medium)");
		inquiry.AddChoice("3", "1024 x 1024 16pt (high)");
		inquiry.AddChoice("4", "2048 x 2048 4pt (very high)");
		inquiry.AddChoice("5", "2048 x 2048 16pt (ultra)");
		inquiry.SetActiveChoice(shadowQuality);
	}

	void SetShadowQuality(cstr value)
	{
		shadowQuality = value;
	}

	void GetLandscapeQuality(IChoiceInquiry& inquiry)
	{
		inquiry.SetTitle("Landscape Quality");
		inquiry.AddChoice("1", "Low");
		inquiry.AddChoice("2", "Medium");
		inquiry.AddChoice("3", "High");
		inquiry.AddChoice("4", "Ultra");
		inquiry.AddChoice("5", "Ultra (Experimental)");
		inquiry.SetActiveChoice(landscapeQuality);
	}

	void SetLandscapeQuality(cstr value)
	{
		landscapeQuality = value;
	}

	void GetReflectionAlgorithm(IChoiceInquiry& inquiry)
	{
		inquiry.SetTitle("Reflection Algorithm");
		inquiry.AddChoice("1", "Gloss (Minimal)");
		inquiry.AddChoice("2", "Global Environmental Mapping (Low)");
		inquiry.AddChoice("3", "Local Environmental Mapping (Medium)");
		inquiry.AddChoice("4", "Dynamic Environmental Mapping (High)");
		inquiry.AddChoice("5", "Raytracing (Ultra)");
		inquiry.SetActiveChoice(reflectionAlgorithm);
	}

	void SetReflectionAlgorithm(cstr value)
	{
		reflectionAlgorithm = value;
	}

	void GetActiveMonitor(IChoiceInquiry& inquiry)
	{
		inquiry.SetTitle("Active Monitor");
		inquiry.AddChoice("1", "1 - 1920x1080");
		inquiry.AddChoice("2", "2 - 3840x2160");
		inquiry.SetActiveChoice(monitor);
	}

	void SetActiveMonitor(cstr value)
	{
		monitor = value;
	}

	void GetFullscreenResolution(IChoiceInquiry& inquiry)
	{
		inquiry.SetTitle("Fullscreen Resolution");
		inquiry.AddChoice("1.1920x1080", "1920 x 1080 60Hz");
		inquiry.AddChoice("2.1920x1200", "1920 x 1200 60Hz");
		inquiry.AddChoice("3.2560x1536", "2560 x 1536 60Hz");
		inquiry.AddChoice("4.3840x2160", "3840 x 2160 60Hz");
		inquiry.AddChoice("5.1366x768", "1366 x 768 60Hz");
		inquiry.AddChoice("6.1280x1024", "1280 x 1024 60Hz");
		inquiry.AddChoice("7.1024x768", "1024 x 768 60Hz");
		inquiry.AddChoice("8.800x600", "800 x 600 60Hz");
		inquiry.AddChoice("9.640x480", "640 x 480 60Hz");
		inquiry.AddChoice("10.1920x1080", "1920 x 1080 144Hz");
		inquiry.AddChoice("11.1920x1200", "1920 x 1200 144Hz");
		inquiry.AddChoice("12.2560x1536", "2560 x 1536 144Hz");
		inquiry.AddChoice("13.3840x2160", "3840 x 2160 144Hz");
		inquiry.AddChoice("14.1366x768", "1366 x 768 144Hz");
		inquiry.AddChoice("15.1280x1024", "1280 x 1024 144Hz");
		inquiry.AddChoice("16.1024x768", "1024 x 768 144Hz");
		inquiry.AddChoice("17.800x600", "800 x 600 144Hz");
		inquiry.AddChoice("18.640x480", "640 x 480 144Hz");
		inquiry.AddChoice("19.1920x1080", "1920 x 1080 200Hz");
		inquiry.AddChoice("20.1920x1200", "1920 x 1200 200Hz");
		inquiry.AddChoice("21.2560x1536", "2560 x 1536 200Hz");
		inquiry.AddChoice("22.3840x2160", "3840 x 2160 200Hz");
		inquiry.AddChoice("23.1366x768", "1366 x 768 200Hz");
		inquiry.AddChoice("24.1280x1024", "1280 x 1024 200Hz");
		inquiry.AddChoice("25.1024x768", "1024 x 768 200Hz");
		inquiry.AddChoice("26.800x600", "800 x 600 200Hz");
		inquiry.AddChoice("27.640x480", "640 x 480 200Hz");
		inquiry.SetActiveChoice(resolution);
	}

	void SetFullscreenResolution(cstr choice)
	{
		resolution = choice;
	}

	void GetTextureQuality(IChoiceInquiry& inquiry)
	{
		inquiry.SetTitle("Texture Quality");
		inquiry.AddChoice("1", "Low 256x256");
		inquiry.AddChoice("2", "Medium 512x512");
		inquiry.AddChoice("3", "High 1024x1024 ");
		inquiry.AddChoice("4", "Very High 2048x2048");
		inquiry.AddChoice("5", "Ultra 4096x2096");
		inquiry.SetActiveChoice(textureQuality);
	}

	void SetTextureQuality(cstr choice)
	{
		textureQuality = choice;
	}

	void GetWaterQuality(IChoiceInquiry& inquiry)
	{
		inquiry.SetTitle("Water Quality");
		inquiry.AddChoice("1", "Low - flat");
		inquiry.AddChoice("2", "Medium - ripples");
		inquiry.AddChoice("3", "High - ripples & refraction");
		inquiry.AddChoice("4", "Very High - full physics");
		inquiry.AddChoice("5", "Ultra - physics and reflections");
		inquiry.SetActiveChoice(waterQuality);
	}

	void SetWaterQuality(cstr choice)
	{
		waterQuality = choice;
	}

	void AddOptions(IGameOptionsBuilder& builder) override
	{
		ADD_GAME_OPTIONS(db, GraphicsOptions, ScreenMode)
		ADD_GAME_OPTIONS(db, GraphicsOptions, FSAA)
		ADD_GAME_OPTIONS(db, GraphicsOptions, ShadowQuality)
		ADD_GAME_OPTIONS(db, GraphicsOptions, LandscapeQuality)
		ADD_GAME_OPTIONS(db, GraphicsOptions, ReflectionAlgorithm)
		ADD_GAME_OPTIONS(db, GraphicsOptions, ActiveMonitor)
		ADD_GAME_OPTIONS(db, GraphicsOptions, FullscreenResolution)
		ADD_GAME_OPTIONS(db, GraphicsOptions, TextureQuality)
		ADD_GAME_OPTIONS(db, GraphicsOptions, WaterQuality)
		db.Build(builder);
	}
};

GraphicsOptions s_GraphicsOptions;

IGameOptions& GetGraphicsOptions()
{
	return s_GraphicsOptions;
}

void TestGameOptions(IGRClientWindow& client)
{
	TestOptions(client, client.GRSystem(), s_GraphicsOptions);
}

void TestWidgets(IGRClientWindow& client)
{
	TestGameOptions(client);
}