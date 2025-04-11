#include <rococo.gr.client.h>
#include <rococo.gui.retained.h>
#include <rococo.reflector.h>
#include <rococo.strings.h>
#include <rococo.game.options.ex.h>
#include <rococo.great.sex.h>
#include <rococo.allocators.h>
#include <sexy.types.h>
#include <Sexy.S-Parser.h>
#include <rococo.io.h>

using namespace Rococo;
using namespace Rococo::Gui;
using namespace Rococo::Strings;
using namespace Rococo::Game::Options;
using namespace Rococo::GreatSex;
using namespace Rococo::Sex;

void RunMessageLoop(IGRClientWindow& client);
void BuildMenus(IGRWidgetMainFrame& frame);
void BuildUpperRightToolbar(IGRWidgetMainFrame& frame);
void UseTestColourScheme(IGRWidgetMainFrame& frame);

void TestGreatSex(IGRClientWindow& client, IGRSystem& gr, IO::IInstallation& installation)
{
	GRIdWidget mainFrame{ "Main-Frame" };
	auto& frame = gr.BindFrame(mainFrame);
	//frame.SetTitleBarHeight(30);

	frame.ClientArea().Panel().Set(EGRSchemeColourSurface::CONTAINER_BACKGROUND, RGBAb(0, 255, 0, 0), GRGenerateIntensities());
	frame.MenuBar().Panel().Parent()->SetCollapsed(true);

	auto& scheme = gr.Root().Scheme();
	SetSchemeColours_ThemeGrey(scheme);
	UseTestColourScheme(frame);

	AutoFree<IAllocatorSupervisor> allocator = Memory::CreateBlockAllocator(64, 0, "GreatSexAllocator");
	AutoFree<IGreatSexGeneratorSupervisor> greatSex = CreateGreatSexGenerator(*allocator);

	Auto<ISParser> sParser = Sex::CreateSexParser_2_0(*allocator);
	AutoFree<IExpandingBuffer> buffer = CreateExpandingBuffer(4_kilobytes);

	cstr name = "!tests/greatsex.test.sexml";
	installation.LoadResource(name, *buffer, 16_megabytes);

	Auto<ISourceCode> src = sParser->ProxySourceBuffer((cstr)buffer->GetData(), (int)buffer->Length(), { 0,0 }, name, nullptr);

	try
	{
		Auto<ISParserTree> tree = sParser->CreateTree(*src);
		cr_sex s = tree->Root();

		greatSex->AppendWidgetTreeFromSexML(s, frame.ClientArea().Widget());
	}
	catch (ParseException& pex)
	{
		pex.Start();
		pex.End();
		client.ShowError(pex.Start(), pex.End(), pex.Name(), (cstr) buffer->GetData(), pex.Message());
	}

	RunMessageLoop(client);
}

void TestGreatSex(IGRClientWindow& client, IO::IInstallation& installation)
{
	TestGreatSex(client, client.GRSystem(), installation);
}
