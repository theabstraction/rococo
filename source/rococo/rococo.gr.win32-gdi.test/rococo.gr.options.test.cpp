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

void TestOptions(IGRClientWindow& client, IGRSystem& gr, IGameOptions& options)
{
	GRIdWidget mainFrame{ "Main-Frame" };
	auto& frame = gr.BindFrame(mainFrame);
	//frame.SetTitleBarHeight(30);

	frame.ClientArea().Panel().Set(EGRSchemeColourSurface::CONTAINER_BACKGROUND, RGBAb(0, 255, 0, 0), GRGenerateIntensities());
	frame.MenuBar().Panel().Parent()->SetCollapsed(true);

	auto& scheme = gr.Root().Scheme();
	SetSchemeColours_ThemeGrey(scheme);
	Gui::SetPropertyEditorColours_PastelScheme(frame.Panel());

//	BuildMenus(frame);

//	BuildUpperRightToolbar(frame);

	auto& viewport = CreateViewportWidget(frame.ClientArea().Widget());
	viewport.Panel().SetExpandToParentHorizontally().SetExpandToParentVertically();

	GameOptionConfig config;
	auto& optionsList = CreateGameOptionsList(viewport.ClientArea().Widget(), options, config);
	optionsList.Panel().Set(EGRSchemeColourSurface::BACKGROUND, RGBAb(255, 255, 255), GRGenerateIntensities());

	RunMessageLoop(client);
}

void TestGameOptions(IGRClientWindow& client)
{
	UNUSED(client);
	//TestOptions(client, client.GRSystem(), s_GraphicsOptions);
}

void TestWidgets(IGRClientWindow& client)
{
	TestGameOptions(client);
}