#include <rococo.gr.client.h>
#include <rococo.gui.retained.h>
#include <rococo.reflector.h>
#include <rococo.strings.h>
#include <rococo.game.options.ex.h>
#include <rococo.allocators.h>
#include <rococo.io.h>
#include <rococo.great.sex.h>
#include <rococo.vkeys.h>

using namespace Rococo;
using namespace Rococo::Gui;
using namespace Rococo::Strings;
using namespace Rococo::Game::Options;

void RunMessageLoop(IGRClientWindow& client);
void BuildMenus(IGRWidgetMainFrame& frame);
void BuildUpperRightToolbar(IGRWidgetMainFrame& frame);
void UseTestColourScheme(IGRWidgetMainFrame& frame);

IGameOptions& GetGraphicsOptions();
IGameOptions& GetAudioOptions();

cstr textSexml = "!tests/greatsex.test.sexml";

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
			generator.AddOptions(GetAudioOptions(), "AudioOptions");
		}
	} onConstruct;

	client.LoadFrame(textSexml, frame, onConstruct);

	struct Reloader: IGRAppControl
	{
		IGRClientWindow* client = nullptr;
		IGRWidgetMainFrame* frame = nullptr;
		IEventCallback<GreatSex::IGreatSexGenerator>* onConstruct = nullptr;

		EGREventRouting OnRawVKey(uint16 vkeyCode) override
		{
			if (vkeyCode == IO::VirtualKeys::VKCode_F12)
			{
				auto& superPanel = static_cast<IGRPanelSupervisor&>(frame->ClientArea().Panel());
				superPanel.ClearChildren();
				client->LoadFrame(textSexml, *frame, *onConstruct);
				return EGREventRouting::Terminate;
			}

			return EGREventRouting::NextHandler;
		}
	} reloader;

	reloader.client = &client;
	reloader.frame = &frame;
	reloader.onConstruct = &onConstruct;	

	client.InterceptVKeys(reloader);

	RunMessageLoop(client);
}

void TestGreatSex(IGRClientWindow& client)
{
	TestGreatSex(client, client.GRSystem());
}
