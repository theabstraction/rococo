#include <rococo.gr.client.h>
#include <rococo.gui.retained.h>
#include <rococo.reflector.h>
#include <rococo.strings.h>

using namespace Rococo;
using namespace Rococo::Gui;

void RenderButton(Gui::IGRRenderContext& rc, const GuiRect& rect)
{
	rc.DrawRect(rect, RGBAb(192, 192, 192));
	rc.DrawRectEdge(rect, RGBAb(224, 224, 224), RGBAb(128, 128, 128));
}

IGR2DScene* TestDrawRect()
{
	struct Scene : IGR2DScene
	{
		AutoFree<IGRImageSupervisor> img1;

		void Render(Gui::IGRRenderContext& rc) override
		{
			rc.DrawRect(rc.ScreenDimensions(), RGBAb(64, 64, 64));

			FontSpec biggish;
			biggish.FontName = "Tahoma";
			biggish.CharHeight = 40;
			biggish.Bold = true;

			auto fontId = rc.Fonts().BindFontId(biggish);

			GuiRect topLeftRect{ 20, 20, 400, 60 };
			RenderButton(rc, topLeftRect);
			
			GRAlignmentFlags topLeftAlignmentFlags;
			topLeftAlignmentFlags.Add(EGRAlignment::Left);
			topLeftAlignmentFlags.Add(EGRAlignment::Top);

			rc.DrawText(fontId, topLeftRect, topLeftRect, topLeftAlignmentFlags, { 0,0 }, "Hello World! qyg"_fstring, RGBAb(0, 0, 128));

			GuiRect topCentreRect{ 440, 20, 840, 60 };
			RenderButton(rc, topCentreRect);

			GRAlignmentFlags centreRightAlignmentFlags;
			centreRightAlignmentFlags.Add(EGRAlignment::HCentre);
			centreRightAlignmentFlags.Add(EGRAlignment::Top);

			rc.DrawText(fontId, topCentreRect, topCentreRect, centreRightAlignmentFlags, { 0,0 }, "Hello World! qyg"_fstring, RGBAb(0, 0, 128));

			GuiRect topRightRect{ 880, 20, 1280, 60 };
			RenderButton(rc, topRightRect);

			GRAlignmentFlags topRightAlignmentFlags;
			topRightAlignmentFlags.Add(EGRAlignment::Right);
			topRightAlignmentFlags.Add(EGRAlignment::Top);

			rc.DrawText(fontId, topRightRect, topRightRect, topRightAlignmentFlags, { 0,0 }, "Hello World! qyg"_fstring, RGBAb(0, 0, 128));

			GuiRect middleLeftRect{ 20, 100, 400, 140 };
			RenderButton(rc, middleLeftRect);

			GRAlignmentFlags middleLeftAlignmentFlags;
			middleLeftAlignmentFlags.Add(EGRAlignment::Left);
			middleLeftAlignmentFlags.Add(EGRAlignment::VCentre);

			rc.DrawText(fontId, middleLeftRect, middleLeftRect, middleLeftAlignmentFlags, { 0,0 }, "Hello World! qyg"_fstring, RGBAb(0, 0, 128));

			GuiRect middleCentreRect{ 440, 100, 840, 140 };
			RenderButton(rc, middleCentreRect);

			GRAlignmentFlags middleCentreAlignmentFlags;
			middleCentreAlignmentFlags.Add(EGRAlignment::HCentre);
			middleCentreAlignmentFlags.Add(EGRAlignment::VCentre);

			rc.DrawText(fontId, middleCentreRect, middleCentreRect, middleCentreAlignmentFlags, { 0,0 }, "Hello World! qyg"_fstring, RGBAb(0, 0, 128));

			GuiRect middleRightRect{ 880, 100, 1280, 140 };
			RenderButton(rc, middleRightRect);

			GRAlignmentFlags middleRightAlignmentFlags;
			middleRightAlignmentFlags.Add(EGRAlignment::Right);
			middleRightAlignmentFlags.Add(EGRAlignment::VCentre);

			rc.DrawText(fontId, middleRightRect, middleRightRect, middleRightAlignmentFlags, { 0,0 }, "Hello World! qyg My God it is good"_fstring, RGBAb(0, 0, 128));

			GuiRect bottomLeftRect{ 20, 180, 400, 220 };
			RenderButton(rc, bottomLeftRect);

			GRAlignmentFlags bottomLeftAlignmentFlags;
			bottomLeftAlignmentFlags.Add(EGRAlignment::Left);
			bottomLeftAlignmentFlags.Add(EGRAlignment::Bottom);

			rc.DrawText(fontId, bottomLeftRect, bottomLeftRect, bottomLeftAlignmentFlags, { 0,0 }, "Hello World! qyg"_fstring, RGBAb(0, 0, 128));

			GuiRect bottomCentreRect{ 440, 180, 840, 220 };
			RenderButton(rc, bottomCentreRect);

			GRAlignmentFlags bottomCentreAlignmentFlags;
			bottomCentreAlignmentFlags.Add(EGRAlignment::HCentre);
			bottomCentreAlignmentFlags.Add(EGRAlignment::Bottom);

			rc.DrawText(fontId, bottomCentreRect, bottomCentreRect, bottomCentreAlignmentFlags, { 0,0 }, "Hello World!"_fstring, RGBAb(0, 0, 128));

			GuiRect bottomRightRect{ 880, 180, 1280, 220 };
			RenderButton(rc, bottomRightRect);

			GRAlignmentFlags bottomRightAlignmentFlags;
			bottomRightAlignmentFlags.Add(EGRAlignment::Right);
			bottomRightAlignmentFlags.Add(EGRAlignment::Bottom);

			rc.DrawText(fontId, bottomRightRect, bottomRightRect, bottomRightAlignmentFlags, { 0,0 }, "Hello World!"_fstring, RGBAb(0, 0, 128));

			if (!img1)
			{
				img1 = rc.Images().CreateImageFromPath("up", R"(D:\work\rococo\content\textures\toolbars\builder.tif)");
			}

			GRAlignmentFlags image1Flags;
			image1Flags.Add(EGRAlignment::Left);
			image1Flags.Add(EGRAlignment::Top);

			GuiRect image1Rect{ 20, 260, 400, 300 };
			GuiRect noClipping{ 0,0,0,0 };

			RenderButton(rc, image1Rect);
			rc.DrawImageUnstretched(*img1, image1Rect, noClipping, image1Flags);

			GRAlignmentFlags image2Flags;
			image2Flags.Add(EGRAlignment::HCentre);
			image2Flags.Add(EGRAlignment::VCentre);

			GuiRect image2Rect{ 440, 260, 840, 300 };

			RenderButton(rc, image2Rect);
			rc.DrawImageUnstretched(*img1, image2Rect, noClipping, image2Flags);

			GRAlignmentFlags image3Flags;
			image3Flags.Add(EGRAlignment::Right);
			image3Flags.Add(EGRAlignment::Bottom);

			GuiRect image3Rect{ 880, 260, 1280, 300 };

			RenderButton(rc, image3Rect);
			rc.DrawImageUnstretched(*img1, image3Rect, noClipping, image3Flags);

			GuiRect image4Rect{ 20, 340, 400, 380 };

			RenderButton(rc, image4Rect);
			rc.DrawImageStretched(*img1, image4Rect, noClipping);
		}
	};

	static Scene scene;
	return &scene;
}

IGR2DScene* TestBlackScene()
{
	struct Scene : IGR2DScene
	{
		void Render(Gui::IGRRenderContext& rc) override
		{
			GuiRect clientRect = rc.ScreenDimensions();
			rc.DrawRect(clientRect, RGBAb(0, 0, 0, 255));
		}
	};
	
	static Scene scene;
	return &scene;
}

IGR2DScene* TestScene()
{
	//return TestDrawRect();
	return TestBlackScene();
}

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

void TestFrame(IGRSystem& gr)
{
	GRIdWidget mainFrame { "Main-Frame" };
	auto& frame = gr.BindFrame(mainFrame);
	//frame.SetTitleBarHeight(30);

	frame.ClientArea().Panel().Set(EGRSchemeColourSurface::CONTAINER_BACKGROUND, RGBAb(0, 255, 0, 255), GRGenerateIntensities());

	auto& scheme = gr.Root().Scheme();
	SetSchemeColours_ThemeGrey(scheme);

	BuildMenus(frame);

	BuildUpperRightToolbar(frame);

	auto& framePanel = frame.Widget().Panel();

	framePanel.Set(EGRSchemeColourSurface::SCROLLER_BUTTON_BACKGROUND, RGBAb(48, 48, 48, 255), GRGenerateIntensities());
	framePanel.Set(EGRSchemeColourSurface::SCROLLER_BUTTON_TOP_LEFT, RGBAb(128, 128, 128, 255), GRGenerateIntensities());
	framePanel.Set(EGRSchemeColourSurface::SCROLLER_BUTTON_BOTTOM_RIGHT, RGBAb(96, 96, 96, 255), GRGenerateIntensities());
	framePanel.Set(EGRSchemeColourSurface::SCROLLER_BAR_BACKGROUND, RGBAb(48, 48, 48, 255), GRGenerateIntensities());
	framePanel.Set(EGRSchemeColourSurface::SCROLLER_BAR_TOP_LEFT, RGBAb(120, 120, 120, 255), GRGenerateIntensities());
	framePanel.Set(EGRSchemeColourSurface::SCROLLER_BAR_BOTTOM_RIGHT, RGBAb(104, 104, 104, 255), GRGenerateIntensities());
	framePanel.Set(EGRSchemeColourSurface::SCROLLER_SLIDER_BACKGROUND, RGBAb(64, 64, 64, 255), GRGenerateIntensities());
	framePanel.Set(EGRSchemeColourSurface::SCROLLER_SLIDER_TOP_LEFT, RGBAb(128, 128, 128, 255), GRGenerateIntensities());
	framePanel.Set(EGRSchemeColourSurface::SCROLLER_SLIDER_BOTTOM_RIGHT, RGBAb(96, 96, 96, 255), GRGenerateIntensities());
	framePanel.Set(EGRSchemeColourSurface::SCROLLER_TRIANGLE_NORMAL, RGBAb(128, 128, 128, 255), GRGenerateIntensities());

	frame.Widget().Panel().Set(EGRSchemeColourSurface::BACKGROUND, RGBAb(255, 0, 0, 255), GRGenerateIntensities());

	struct EventHandler: Gui::IGRPropertyEditorPopulationEvents
	{
		void OnAddNameValue(IGRWidgetText& nameWidget, IGRWidgetEditBox& editorWidget)
		{

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

		void Visit(Reflection::IReflectionVisitor& v)
		{
			Reflection::Section target(v, "Stats");
			ROCOCO_REFLECT(v, Toughness);
			ROCOCO_REFLECT(v, Speed);
			ROCOCO_REFLECT(v, Skill);
			ROCOCO_REFLECT(v, Magic);
			ROCOCO_REFLECT(v, Charm);
		}
	};

	struct Target1: Reflection::IReflectionTarget
	{
		void Visit(Reflection::IReflectionVisitor& v)
		{
			Reflection::Section target(v, Name);
			ROCOCO_REFLECT(v, MeaningOfLife);
			ROCOCO_REFLECT(v, Age);
			ROCOCO_REFLECT(v, Height);
			ROCOCO_REFLECT(v, Weight);
			stats.Visit(v);
		}

		int MeaningOfLife = 42;
		Strings::HString Name = "Arthur Dent";
		float Age = 42.0f;
		double Height = 175.0;
		double Weight = 60;

		Stats stats = Stats(10, 10, 9, 0, 6);
	};

	struct Target2 : Reflection::IReflectionTarget
	{
		void Visit(Reflection::IReflectionVisitor& v)
		{
			Reflection::Section target(v, Name);
			ROCOCO_REFLECT(v, Earnings);
			ROCOCO_REFLECT(v, Age);
			ROCOCO_REFLECT(v, Height);
			ROCOCO_REFLECT(v, Weight);
			stats.Visit(v);
		}

		int Earnings = 300'000'000;
		Strings::HString Name = "Stan Lee";
		float Age = 95.0f;
		double Height = 180.0;
		double Weight = 68;

		Stats stats = Stats(7, 6, 14, 1, 9);
	};

	struct Target3 : Reflection::IReflectionTarget
	{
		void Visit(Reflection::IReflectionVisitor& v)
		{
			Reflection::Section target(v, Name);
			ROCOCO_REFLECT(v, BestConcerto);
			ROCOCO_REFLECT(v, BestOpera);
			ROCOCO_REFLECT(v, Earnings);
			ROCOCO_REFLECT(v, Age);
			ROCOCO_REFLECT(v, Height);
			ROCOCO_REFLECT(v, Weight);

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
	};

	struct Target4 : Reflection::IReflectionTarget
	{
		void Visit(Reflection::IReflectionVisitor& v)
		{
			Reflection::Section target(v, Name);
			ROCOCO_REFLECT(v, FunniestJoke);
			ROCOCO_REFLECT(v, FavouriteDrink);
			ROCOCO_REFLECT(v, Earnings);
			ROCOCO_REFLECT(v, Age);
			ROCOCO_REFLECT(v, Height);
			ROCOCO_REFLECT(v, Weight);
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
	};


	struct Target : Reflection::IReflectionTarget
	{
		Target1 t1;
		Target2 t2;
		Target3 t3;
		Target4 t4;

		void Visit(Reflection::IReflectionVisitor& v)
		{
			t1.Visit(v);
			t2.Visit(v);
			t3.Visit(v);
			t4.Visit(v);
		}
	} target;


	auto& editor = Gui::CreatePropertyEditorTree(frame.ClientArea().InnerWidget(), eventHandler);
	editor.SetRowHeight(20);
	editor.View(target);
	editor.Widget().Panel().Set(EGRSchemeColourSurface::CONTAINER_BACKGROUND, RGBAb(192, 192, 192, 255), GRGenerateIntensities());
}

void TestWidgets(IGRSystem& gr)
{
	TestFrame(gr);
}