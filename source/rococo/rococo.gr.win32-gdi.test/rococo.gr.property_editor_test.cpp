#include <rococo.gr.client.h>
#include <rococo.gui.retained.h>
#include <rococo.reflector.h>
#include <rococo.strings.h>

using namespace Rococo;
using namespace Rococo::Gui;

void RunMessageLoop(IGRClientWindow& client);

void BuildMenus(IGRWidgetMainFrame& frame)
{
	auto& menu = frame.MenuBar();
	menu.Widget().Panel().
		Set(EGRSchemeColourSurface::MENU_BUTTON_TEXT, RGBAb(192, 192, 192, 255), GRRenderState(false, false, false)).
		Set(EGRSchemeColourSurface::MENU_BUTTON_TEXT, RGBAb(255, 255, 255, 255), GRRenderState(true, false, false)).
		Set(EGRSchemeColourSurface::MENU_BUTTON_TEXT, RGBAb(255, 255, 255, 255), GRRenderState(false, true, false)).
		Set(EGRSchemeColourSurface::MENU_BUTTON_TEXT, RGBAb(255, 255, 255, 255), GRRenderState(false, false, true)).
		Set(EGRSchemeColourSurface::MENU_BUTTON_TEXT, RGBAb(255, 255, 255, 255), GRRenderState(true, true, false)).
		Set(EGRSchemeColourSurface::MENU_BUTTON_TEXT, RGBAb(255, 255, 255, 255), GRRenderState(true, false, true)).
		Set(EGRSchemeColourSurface::MENU_BUTTON_TEXT, RGBAb(255, 255, 255, 255), GRRenderState(false, true, true)).
		Set(EGRSchemeColourSurface::MENU_BUTTON_TEXT, RGBAb(255, 255, 255, 255), GRRenderState(true, true, true)).
		Set(EGRSchemeColourSurface::MENU_BUTTON_EDGE_BOTTOM_RIGHT, RGBAb(32, 32, 32, 255), GRRenderState(false, false, false)).
		Set(EGRSchemeColourSurface::MENU_BUTTON_EDGE_BOTTOM_RIGHT, RGBAb(96, 96, 96, 255), GRRenderState(true, false, false)).
		Set(EGRSchemeColourSurface::MENU_BUTTON_EDGE_BOTTOM_RIGHT, RGBAb(96, 96, 96, 255), GRRenderState(false, true, false)).
		Set(EGRSchemeColourSurface::MENU_BUTTON_EDGE_BOTTOM_RIGHT, RGBAb(96, 96, 96, 255), GRRenderState(false, false, true)).
		Set(EGRSchemeColourSurface::MENU_BUTTON_EDGE_BOTTOM_RIGHT, RGBAb(96, 96, 96, 255), GRRenderState(true, true, false)).
		Set(EGRSchemeColourSurface::MENU_BUTTON_EDGE_BOTTOM_RIGHT, RGBAb(96, 96, 96, 255), GRRenderState(true, false, true)).
		Set(EGRSchemeColourSurface::MENU_BUTTON_EDGE_BOTTOM_RIGHT, RGBAb(96, 96, 96, 255), GRRenderState(false, true, true)).
		Set(EGRSchemeColourSurface::MENU_BUTTON_EDGE_BOTTOM_RIGHT, RGBAb(96, 96, 96, 255), GRRenderState(true, true, true)).
		Set(EGRSchemeColourSurface::MENU_BUTTON_EDGE_TOP_LEFT, RGBAb(32, 32, 32, 255), GRRenderState(false, false, false)).
		Set(EGRSchemeColourSurface::MENU_BUTTON_EDGE_TOP_LEFT, RGBAb(192, 192, 192, 255), GRRenderState(true, false, false)).
		Set(EGRSchemeColourSurface::MENU_BUTTON_EDGE_TOP_LEFT, RGBAb(192, 192, 192, 255), GRRenderState(false, true, false)).
		Set(EGRSchemeColourSurface::MENU_BUTTON_EDGE_TOP_LEFT, RGBAb(192, 192, 192, 255), GRRenderState(false, false, true)).
		Set(EGRSchemeColourSurface::MENU_BUTTON_EDGE_TOP_LEFT, RGBAb(192, 192, 192, 255), GRRenderState(true, true, false)).
		Set(EGRSchemeColourSurface::MENU_BUTTON_EDGE_TOP_LEFT, RGBAb(192, 192, 192, 255), GRRenderState(true, false, true)).
		Set(EGRSchemeColourSurface::MENU_BUTTON_EDGE_TOP_LEFT, RGBAb(192, 192, 192, 255), GRRenderState(false, true, true)).
		Set(EGRSchemeColourSurface::MENU_BUTTON_EDGE_TOP_LEFT, RGBAb(192, 192, 192, 255), GRRenderState(true, true, true)).
		Set(EGRSchemeColourSurface::MENU_BUTTON, RGBAb(32, 32, 32, 255), GRRenderState(false, false, false)).
		Set(EGRSchemeColourSurface::MENU_BUTTON, RGBAb(64, 64, 64, 255), GRRenderState(true, false, false)).
		Set(EGRSchemeColourSurface::MENU_BUTTON, RGBAb(64, 64, 64, 255), GRRenderState(false, true, false)).
		Set(EGRSchemeColourSurface::MENU_BUTTON, RGBAb(64, 64, 64, 255), GRRenderState(false, false, true)).
		Set(EGRSchemeColourSurface::MENU_BUTTON, RGBAb(80, 80, 80, 255), GRRenderState(true, true, false)).
		Set(EGRSchemeColourSurface::MENU_BUTTON, RGBAb(80, 80, 80, 255), GRRenderState(true, false, true)).
		Set(EGRSchemeColourSurface::MENU_BUTTON, RGBAb(64, 64, 64, 255), GRRenderState(false, true, true)).
		Set(EGRSchemeColourSurface::MENU_BUTTON, RGBAb(80, 80, 80, 255), GRRenderState(true, true, true));

	auto fileMenu = menu.AddSubMenu(GRMenuItemId::Root(), GRMenuSubMenu("File"));

	auto editMenu = menu.AddSubMenu(GRMenuItemId::Root(), GRMenuSubMenu("Edit"));
	auto viewMenu = menu.AddSubMenu(GRMenuItemId::Root(), GRMenuSubMenu("View"));
	auto projectMenu = menu.AddSubMenu(GRMenuItemId::Root(), GRMenuSubMenu("Project"));
	auto windowMenu = menu.AddSubMenu(GRMenuItemId::Root(), GRMenuSubMenu("Window"));
	auto helpMenu = menu.AddSubMenu(GRMenuItemId::Root(), GRMenuSubMenu("Help"));

	menu.AddButton(fileMenu, { "New", { 0, nullptr } });
	menu.AddButton(fileMenu, { "Open...", { 0, nullptr } });
	menu.AddButton(fileMenu, { "Save", { 0, nullptr } });
	menu.AddButton(fileMenu, { "Save As...", { 0, nullptr } });
	menu.AddButton(fileMenu, { "Exit", { 0, nullptr } });

	menu.AddButton(editMenu, { "Find...", { 0, nullptr } });
	menu.AddButton(editMenu, { "Replace...", { 0, nullptr } });
	menu.AddButton(editMenu, { "Copy", { 0, nullptr } });
	menu.AddButton(editMenu, { "Cut", { 0, nullptr } });
	menu.AddButton(editMenu, { "Paste", { 0, nullptr } });

	menu.AddButton(viewMenu, { "Solution", { 0, nullptr } });
	menu.AddButton(viewMenu, { "Classes", { 0, nullptr } });
	menu.AddButton(viewMenu, { "Repo", { 0, nullptr } });
	menu.AddButton(viewMenu, { "Debugger", { 0, nullptr } });
	menu.AddButton(viewMenu, { "Output", { 0, nullptr } });

	menu.AddButton(projectMenu, { "Build", { 0, nullptr } });
	menu.AddButton(projectMenu, { "Rebuild", { 0, nullptr } });
	menu.AddButton(projectMenu, { "Debug", { 0, nullptr } });
	menu.AddButton(projectMenu, { "Cancel", { 0, nullptr } });

	menu.AddButton(windowMenu, { "Split", { 0, nullptr } });
	menu.AddButton(windowMenu, { "Cascade", { 0, nullptr } });
	menu.AddButton(windowMenu, { "Merge", { 0, nullptr } });

	auto toggles = menu.AddSubMenu(windowMenu, GRMenuSubMenu("Toggles"));
	menu.AddButton(toggles, { "Toolkit", { 0, nullptr } });
	menu.AddButton(toggles, { "Properties", { 0, nullptr } });
	menu.AddButton(toggles, { "Log", { 0, nullptr } });
	menu.AddButton(windowMenu, { "Close All", { 0, nullptr } });

	menu.AddButton(helpMenu, { "About...", { 0, nullptr } });
	menu.AddButton(helpMenu, { "Check for updates", { 0, nullptr } });
	menu.AddButton(helpMenu, { "Version", { 0, nullptr } });
	menu.AddButton(helpMenu, { "Purchase License", { 0, nullptr } });

	auto& titleBar = *frame.MenuBar().Widget().Panel().Parent();
	titleBar.Set(EGRSchemeColourSurface::CONTAINER_BACKGROUND, RGBAb(32, 32, 32, 255), GRGenerateIntensities());
	titleBar.Set(EGRSchemeColourSurface::BUTTON, RGBAb(0, 0, 0, 255), GRGenerateIntensities());
}

enum class ToolbarMetaId : int64 { MINIMIZE = 400'000'001, RESTORE, EXIT };

void BuildUpperRightToolbar(IGRWidgetMainFrame& frame)
{
	auto& tools = frame.TopRightHandSideTools();
	tools.SetChildAlignment(EGRAlignment::Right);

	auto& minimizer = CreateButton(tools.Widget()).SetTitle("Min").SetImagePath("!textures/toolbars/3rd-party/www.aha-soft.com/Down.tiff").SetClickCriterion(EGRClickCriterion::OnDownThenUp).SetEventPolicy(EGREventPolicy::PublicEvent);
	auto& restorer = CreateButton(tools.Widget()).SetTitle("Max").SetImagePath("!textures/toolbars/3rd-party/www.aha-soft.com/Expand.tiff").SetClickCriterion(EGRClickCriterion::OnDownThenUp).SetEventPolicy(EGREventPolicy::PublicEvent);
	auto& closer = CreateButton(tools.Widget()).SetTitle("Close").SetImagePath("!textures/toolbars/3rd-party/www.aha-soft.com/Close.tiff").SetClickCriterion(EGRClickCriterion::OnDownThenUp).SetEventPolicy(EGREventPolicy::PublicEvent);

	minimizer.SetMetaData({ (int64)ToolbarMetaId::MINIMIZE, "OnMinimize" }, true);
	restorer.SetMetaData({ (int64)ToolbarMetaId::RESTORE, "OnRestore" }, true);
	closer.SetMetaData({ (int64)ToolbarMetaId::EXIT, "OnExit" }, true);

	minimizer.Widget().Panel().SetExpandToParentVertically();
	restorer.Widget().Panel().SetExpandToParentVertically();
	closer.Widget().Panel().SetExpandToParentVertically();

	minimizer.Widget().Panel().SetConstantWidth(32);
	restorer.Widget().Panel().SetConstantWidth(32);
	closer.Widget().Panel().SetConstantWidth(32);

	tools.Widget().Panel().Set(EGRSchemeColourSurface::CONTAINER_BACKGROUND, RGBAb(255, 32, 32, 0), GRGenerateIntensities());
	tools.Widget().Panel().Set(EGRSchemeColourSurface::CONTAINER_BOTTOM_RIGHT, RGBAb(255, 32, 32, 0), GRGenerateIntensities());
	tools.Widget().Panel().Set(EGRSchemeColourSurface::CONTAINER_TOP_LEFT, RGBAb(255, 32, 32, 0), GRGenerateIntensities());
	tools.Widget().Panel().SetLayoutDirection(ELayoutDirection::RightToLeft);
	tools.Widget().Panel().SetExpandToParentVertically();
	tools.Widget().Panel().SetConstantWidth(256);
}

void UseTestColourScheme(IGRWidgetMainFrame& frame)
{
	auto& framePanel = frame.Widget().Panel();

	framePanel.Set(EGRSchemeColourSurface::SCROLLER_BUTTON_BACKGROUND, RGBAb(192, 192, 192, 255), GRRenderState(false, false, false));
	framePanel.Set(EGRSchemeColourSurface::SCROLLER_BUTTON_BACKGROUND, RGBAb(160, 160, 160, 255), GRRenderState(false, false, true));
	framePanel.Set(EGRSchemeColourSurface::SCROLLER_BUTTON_BACKGROUND, RGBAb(160, 160, 160, 255), GRRenderState(false, true, false));
	framePanel.Set(EGRSchemeColourSurface::SCROLLER_BUTTON_BACKGROUND, RGBAb(160, 160, 160, 255), GRRenderState(false, true, true));
	framePanel.Set(EGRSchemeColourSurface::SCROLLER_BUTTON_BACKGROUND, RGBAb(120, 120, 120, 255), GRRenderState(true, false, false));
	framePanel.Set(EGRSchemeColourSurface::SCROLLER_BUTTON_BACKGROUND, RGBAb(120, 120, 120, 255), GRRenderState(true, false, true));
	framePanel.Set(EGRSchemeColourSurface::SCROLLER_BUTTON_BACKGROUND, RGBAb(120, 120, 120, 255), GRRenderState(true, true, false));
	framePanel.Set(EGRSchemeColourSurface::SCROLLER_BUTTON_BACKGROUND, RGBAb(120, 120, 120, 255), GRRenderState(true, true, true));
	framePanel.Set(EGRSchemeColourSurface::SCROLLER_BUTTON_TOP_LEFT, RGBAb(64, 64, 64, 255), GRGenerateIntensities());
	framePanel.Set(EGRSchemeColourSurface::SCROLLER_BUTTON_BOTTOM_RIGHT, RGBAb(32, 32, 32, 255), GRGenerateIntensities());
	framePanel.Set(EGRSchemeColourSurface::SCROLLER_BAR_BACKGROUND, RGBAb(225, 225, 225, 255), GRGenerateIntensities());
	framePanel.Set(EGRSchemeColourSurface::SCROLLER_BAR_TOP_LEFT, RGBAb(64, 64, 64, 255), GRGenerateIntensities());
	framePanel.Set(EGRSchemeColourSurface::SCROLLER_BAR_BOTTOM_RIGHT, RGBAb(32, 32, 32, 255), GRGenerateIntensities());
	framePanel.Set(EGRSchemeColourSurface::SCROLLER_SLIDER_BACKGROUND, RGBAb(64, 64, 64, 255), GRGenerateIntensities());
	framePanel.Set(EGRSchemeColourSurface::SCROLLER_SLIDER_TOP_LEFT, RGBAb(128, 128, 128, 255), GRGenerateIntensities());
	framePanel.Set(EGRSchemeColourSurface::SCROLLER_SLIDER_BOTTOM_RIGHT, RGBAb(96, 96, 96, 255), GRGenerateIntensities());
	framePanel.Set(EGRSchemeColourSurface::SCROLLER_TRIANGLE_NORMAL, RGBAb(224, 224, 224, 255), GRGenerateIntensities());
	framePanel.Set(EGRSchemeColourSurface::READ_ONLY_TEXT, RGBAb(128, 128, 128, 255), GRGenerateIntensities());
	SetUniformColourForAllRenderStates(framePanel, EGRSchemeColourSurface::ROW_COLOUR_EVEN, RGBAb(240, 240, 240));
	SetUniformColourForAllRenderStates(framePanel, EGRSchemeColourSurface::ROW_COLOUR_ODD, RGBAb(255, 255, 255));
	SetUniformColourForAllRenderStates(framePanel, EGRSchemeColourSurface::NAME_TEXT, RGBAb(0, 0, 0, 255));
	SetUniformColourForAllRenderStates(framePanel, EGRSchemeColourSurface::LABEL_BACKGROUND, RGBAb(255, 255, 255, 0));
	SetUniformColourForAllRenderStates(framePanel, EGRSchemeColourSurface::EDITOR, RGBAb(192, 192, 192));
	SetUniformColourForAllRenderStates(framePanel, EGRSchemeColourSurface::EDIT_TEXT, RGBAb(0, 0, 0));
	SetUniformColourForAllRenderStates(framePanel, EGRSchemeColourSurface::COLLAPSER_TITLE_DEPTH_EVEN, RGBAb(255, 240, 240));
	SetUniformColourForAllRenderStates(framePanel, EGRSchemeColourSurface::COLLAPSER_TITLE_DEPTH_ODD, RGBAb(240, 255, 240));
	SetUniformColourForAllRenderStates(framePanel, EGRSchemeColourSurface::COLLAPSER_TITLE_TEXT, RGBAb(0, 0, 0, 255));
	SetUniformColourForAllRenderStates(framePanel, EGRSchemeColourSurface::VALUE_TEXT, RGBAb(0, 0, 0, 255));
	SetUniformColourForAllRenderStates(framePanel, EGRSchemeColourSurface::BUTTON_IMAGE_FOG, RGBAb(0, 0, 0, 0));
	framePanel.Set(EGRSchemeColourSurface::BUTTON_IMAGE_FOG, RGBAb(192, 192, 192, 32), GRRenderState(0, 1, 0));
	framePanel.Set(EGRSchemeColourSurface::BUTTON_IMAGE_FOG, RGBAb(192, 192, 192, 48), GRRenderState(0, 0, 1));
	framePanel.Set(EGRSchemeColourSurface::BUTTON_IMAGE_FOG, RGBAb(192, 192, 192, 64), GRRenderState(0, 1, 1));

	frame.Widget().Panel().Set(EGRSchemeColourSurface::BACKGROUND, RGBAb(255, 0, 0, 0), GRGenerateIntensities());
}

void TestFrame(IGRClientWindow& client, IGRSystem& gr)
{
	GRIdWidget mainFrame { "Main-Frame" };
	auto& frame = gr.BindFrame(mainFrame);
	//frame.SetTitleBarHeight(30);

	frame.ClientArea().Panel().Set(EGRSchemeColourSurface::CONTAINER_BACKGROUND, RGBAb(0, 255, 0, 0), GRGenerateIntensities());

	auto& scheme = gr.Root().Scheme();
	SetSchemeColours_ThemeGrey(scheme);

	BuildMenus(frame);
	BuildUpperRightToolbar(frame);
	UseTestColourScheme(frame);

	struct EventHandler: Gui::IGRPropertyEditorPopulationEvents
	{
		void OnAddNameValue(IGRWidgetText& nameWidget, IGRWidgetEditBox& editorWidget)
		{
			UNUSED(nameWidget);
			UNUSED(editorWidget);
		}
	} eventHandler;

	struct Stats : Reflection::IReflectionTarget
	{
		int Toughness;
		int Speed;
		int Skill;
		int Magic;
		int Charm;

		Stats(int _Toughness, int _Speed, int _Skill, int _Magic, int _Charm) :
			Toughness(_Toughness),
			Speed(_Speed),
			Skill(_Skill),
			Magic(_Magic),
			Charm(_Charm)
		{

		}

		~Stats()
		{
		}

		void Visit(Reflection::IReflectionVisitor& v)
		{
			auto& statMeta = Reflection::Mutable().Range(1, 24);

			Reflection::Section target(v, "Stats");
			ROCOCO_REFLECT_EX(v, Toughness, statMeta);
			ROCOCO_REFLECT_EX(v, Speed, statMeta);
			ROCOCO_REFLECT_EX(v, Skill, statMeta);
			ROCOCO_REFLECT_EX(v, Magic, statMeta);
			ROCOCO_REFLECT_EX(v, Charm, statMeta);
		}

		Reflection::IReflectionVisitation* Visitation() override
		{
			return nullptr;
		}
	};

	struct Target1: Reflection::IReflectionTarget
	{
		void Visit(Reflection::IReflectionVisitor& v)
		{
			Reflection::Section target(v, Name);
			ROCOCO_REFLECT_EX(v, MeaningOfLife, Reflection::Mutable().AddThousandMarks());
			ROCOCO_REFLECT_EX(v, Age, Reflection::Mutable().Range(0.0f, 140.0f));
			ROCOCO_REFLECT_EX(v, Height, Reflection::Mutable().Range(0.0f, 330.0f).Precision(2));
			ROCOCO_REFLECT_EX(v, Weight, Reflection::Mutable().Range(0.0f, 400.0f).Precision(1));
			stats.Visit(v);
		}

		int MeaningOfLife = 42021;
		Strings::HString Name = "Arthur Dent";
		float Age = 42.0f;
		double Height = 175.0;
		double Weight = 60;

		Stats stats = Stats(10, 10, 9, 0, 6);

		Reflection::IReflectionVisitation* Visitation() override
		{
			return nullptr;
		}
	};

	struct Target2 : Reflection::IReflectionTarget
	{
		void Visit(Reflection::IReflectionVisitor& v)
		{
			Reflection::Section target(v, Name);
			ROCOCO_REFLECT_EX(v, Earnings, Reflection::Mutable().AddThousandMarks());
			ROCOCO_REFLECT_EX(v, Age, Reflection::Mutable().Range(0.0f, 140.0f));
			ROCOCO_REFLECT_EX(v, Height, Reflection::Mutable().Range(0.0f, 330.0f).Precision(2));
			ROCOCO_REFLECT_EX(v, Weight, Reflection::Mutable().Range(0.0f, 400.0f).Precision(1));
			stats.Visit(v);
		}

		int Earnings = 300'000'000;
		Strings::HString Name = "Stan Lee";
		float Age = 95.0f;
		double Height = 180.0;
		double Weight = 68;

		Stats stats = Stats(7, 6, 14, 1, 9);

		Reflection::IReflectionVisitation* Visitation() override
		{
			return nullptr;
		}
	};

	struct Target3 : Reflection::IReflectionTarget
	{
		void Visit(Reflection::IReflectionVisitor& v)
		{
			Reflection::Section target(v, Name);
			ROCOCO_REFLECT(v, BestConcerto);
			ROCOCO_REFLECT(v, BestOpera);
			ROCOCO_REFLECT(v, Earnings);
			ROCOCO_REFLECT_EX(v, Age, Reflection::Mutable().Range(0.0f, 140.0f));
			ROCOCO_REFLECT_EX(v, Height, Reflection::Mutable().Range(0.0f, 330.0f).Precision(2));
			ROCOCO_REFLECT_EX(v, Weight, Reflection::Mutable().Range(0.0f, 400.0f).Precision(1));

			stats.Visit(v);
		}

		int Earnings = 500;
		Strings::HString BestConcerto = "23rd in A Major";
		Strings::HString BestOpera = "The Marriage of Figaro";
		Strings::HString Name = "Amadeus Wolfgang Mozart";
		float Age = 36.0f;
		double Height = 163.0;
		double Weight = 50;

		Stats stats = Stats(7, 12, 24, 9, 13);

		Reflection::IReflectionVisitation* Visitation() override
		{
			return nullptr;
		}
	};

	struct Target4 : Reflection::IReflectionTarget
	{
		void Visit(Reflection::IReflectionVisitor& v)
		{
			Reflection::Section target(v, Name);
			ROCOCO_REFLECT(v, FunniestJoke);
			ROCOCO_REFLECT(v, FavouriteDrink);
			ROCOCO_REFLECT(v, Earnings);
			ROCOCO_REFLECT_EX(v, Age, Reflection::Mutable().Range(0.0f, 140.0f));
			ROCOCO_REFLECT_EX(v, Height, Reflection::Mutable().Range(0.0f, 330.0f).Precision(2));
			ROCOCO_REFLECT_EX(v, Weight, Reflection::Mutable().Range(0.0f, 400.0f).Precision(1));
			stats.Visit(v);
		}

		int Earnings = 2500;
		Strings::HString FunniestJoke = "The one about the monkey";
		Strings::HString FavouriteDrink = "Orange Juice";
		Strings::HString Name = "Lee Harvey Oswald";
		float Age = 24.0f;
		double Height = 175.0;
		double Weight = 50;

		Stats stats = Stats(14, 13, 11, 1, 9);

		Reflection::IReflectionVisitation* Visitation() override
		{
			return nullptr;
		}
	};

	struct Target5 : Reflection::IReflectionTarget
	{
		void Visit(Reflection::IReflectionVisitor& v)
		{
			Reflection::Section target(v, Name);
			ROCOCO_REFLECT(v, BestFriend);
			ROCOCO_REFLECT(v, Lyrics);
			ROCOCO_REFLECT_EX(v, Earnings, Reflection::ReadOnly().AddThousandMarks());
			ROCOCO_REFLECT_EX(v, Age, Reflection::Mutable().Range(0.0f, 140.0f).Precision(2));
			ROCOCO_REFLECT_EX(v, Height, Reflection::Mutable().Range(0.0f, 330.0f).Precision(2));
			ROCOCO_REFLECT_EX(v, Weight, Reflection::Mutable().Range(0.0f, 400.0f).Precision(1));
			stats.Visit(v);
		}

		int Earnings = 12300;
		Strings::HString BestFriend = "Papagena";
		char Lyrics[16] = "Pa pa pa";
		Strings::HString Name = "Papageno";
		float Age = 22.0f;
		double Height = 178.0;
		double Weight = 53;

		Stats stats = Stats(15, 12, 11, 4, 12);

		Reflection::IReflectionVisitation* Visitation() override
		{
			return nullptr;
		}
	};

	struct Target : Reflection::VisitationTarget
	{
		Target1 t1;
		Target2 t2;
		Target3 t3;
		Target4 t4;
		Target5 t5;

		Target()
		{

		}

		void Visit(Reflection::IReflectionVisitor& v)
		{
			t1.Visit(v);
			t2.Visit(v);
			t3.Visit(v);
			t4.Visit(v);
			t5.Visit(v);
		}
	} target;

	Gui::PropertyEditorSpec spec;

	FontSpec boldFont;
	boldFont.Bold = true;
	boldFont.CharHeight = 16;
	boldFont.CharSet = ECharSet::ANSI;
	boldFont.FontName = "Consolas";
	spec.NameplateFontId = gr.Fonts().BindFontId(boldFont);

	FontSpec headingFontSpec;
	headingFontSpec.Bold = true;
	headingFontSpec.CharHeight = 20;
	headingFontSpec.CharSet = ECharSet::ANSI;
	headingFontSpec.FontName = "Consolas";
	spec.HeadingFontId = gr.Fonts().BindFontId(headingFontSpec);

	FontSpec valueFontSpec;
	valueFontSpec.Bold = false;
	valueFontSpec.CharHeight = 16;
	valueFontSpec.CharSet = ECharSet::ANSI;
	valueFontSpec.FontName = "Consolas";
	spec.ValueFontId = gr.Fonts().BindFontId(valueFontSpec);

	spec.LeftAlignNameplates = true;

	auto& editor = Gui::CreatePropertyEditorTree(frame.ClientArea().Widget(), eventHandler, spec);

	editor.SetRowHeight(gr.Fonts().GetFontHeight(spec.NameplateFontId) + 4);

	editor.View(target.Visitation());
	editor.Widget().Panel().Set(EGRSchemeColourSurface::CONTAINER_BACKGROUND, RGBAb(192, 192, 192, 0), GRGenerateIntensities());

	RunMessageLoop(client);
}

void TestPropertyEditor(IGRClientWindow& client)
{
	TestFrame(client, client.GRSystem());
}