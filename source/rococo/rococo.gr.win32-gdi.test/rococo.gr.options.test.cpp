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

	auto& optionsList = CreateGameOptionsList(viewport.ClientArea().Widget(), options);
	optionsList.Panel().Set(EGRSchemeColourSurface::BACKGROUND, RGBAb(255, 255, 255), GRGenerateIntensities());

	RunMessageLoop(client);
}

struct GraphicsOptions: IGameOptions
{
	OptionDatabase<GraphicsOptions> db;

	IOptionDatabase& DB() override
	{
		return db;
	}

	HString activeScreenMode = "Fullscreen";
	HString shadowQuality = "2";
	bool isFSAAEnabled = false;
	double musicVolume = 0.25;
	double fxVolume = 0.2;
	double narrationVolume = 0.4;

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

	void AddOptions(IGameOptionsBuilder& builder) override
	{
		ADD_GAME_OPTIONS(db, GraphicsOptions, ScreenMode)
		ADD_GAME_OPTIONS(db, GraphicsOptions, FSAA)
		ADD_GAME_OPTIONS(db, GraphicsOptions, MusicVolume)
		ADD_GAME_OPTIONS(db, GraphicsOptions, FXVolume)
		ADD_GAME_OPTIONS(db, GraphicsOptions, NarrationVolume)
		ADD_GAME_OPTIONS(db, GraphicsOptions, ShadowQuality)
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