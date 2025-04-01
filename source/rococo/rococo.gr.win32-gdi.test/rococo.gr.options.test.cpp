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

	auto& scheme = gr.Root().Scheme();
	SetSchemeColours_ThemeGrey(scheme);

	BuildMenus(frame);

	BuildUpperRightToolbar(frame);

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
	bool isFSAAEnabled = false;
	double musicVolume = 0.25;
	double fxVolume = 0.2;

	GraphicsOptions() : db(*this)
	{

	}

	void GetScreenMode(IChoiceInquiry& inquiry)
	{
		inquiry.SetTitle("Screen Mode");
		inquiry.AddChoice("Fullscreen");
		inquiry.AddChoice("Windowed");
		inquiry.AddChoice("Fullscreen Windowed");
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

	void AddOptions(IGameOptionsBuilder& builder) override
	{
		ADD_GAME_OPTIONS(db, GraphicsOptions, ScreenMode)
		ADD_GAME_OPTIONS(db, GraphicsOptions, FSAA)
		ADD_GAME_OPTIONS(db, GraphicsOptions, MusicVolume)
		ADD_GAME_OPTIONS(db, GraphicsOptions, FXVolume)
		db.Build(builder);
	}
};

void TestGameOptions(IGRClientWindow& client)
{
	GraphicsOptions options;
	TestOptions(client, client.GRSystem(), options);
}

void TestWidgets(IGRClientWindow& client)
{
	TestGameOptions(client);
}