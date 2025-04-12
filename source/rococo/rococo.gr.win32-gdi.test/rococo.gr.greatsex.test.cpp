#include <rococo.gr.client.h>
#include <rococo.gui.retained.h>
#include <rococo.reflector.h>
#include <rococo.strings.h>
#include <rococo.game.options.ex.h>
#include <rococo.allocators.h>
#include <rococo.io.h>
#include <rococo.great.sex.h>

using namespace Rococo;
using namespace Rococo::Gui;
using namespace Rococo::Strings;
using namespace Rococo::Game::Options;

void RunMessageLoop(IGRClientWindow& client);
void BuildMenus(IGRWidgetMainFrame& frame);
void BuildUpperRightToolbar(IGRWidgetMainFrame& frame);
void UseTestColourScheme(IGRWidgetMainFrame& frame);

IGameOptions& GetGraphicsOptions();

void TestGreatSex(IGRClientWindow& client, IGRSystem& gr)
{
	GRIdWidget mainFrame{ "Main-Frame" };
	auto& frame = gr.BindFrame(mainFrame);
	//frame.SetTitleBarHeight(30);

	frame.MenuBar().Panel().Parent()->SetCollapsed(true);

	auto& scheme = gr.Root().Scheme();
	SetSchemeColours_ThemeGrey(scheme);
	UseTestColourScheme(frame);

	struct : IEventCallback<GreatSex::IGreatSexGenerator>
	{
		void OnEvent(GreatSex::IGreatSexGenerator& generator) override
		{
			generator.AddOptions(GetGraphicsOptions(), "GraphicsOptions");
		}
	} onConstruct;

	client.LoadFrame("!tests/greatsex.test.sexml", frame.Widget(), onConstruct);

	RunMessageLoop(client);
}

void TestGreatSex(IGRClientWindow& client)
{
	TestGreatSex(client, client.GRSystem());
}
