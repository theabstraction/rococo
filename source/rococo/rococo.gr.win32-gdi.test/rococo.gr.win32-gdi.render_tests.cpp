#include <rococo.gr.client.h>
#include <rococo.gui.retained.h>

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

			GuiRect image1Rect{ 20, 260, 128, 300 };
			rc.DrawImage(*img1, image1Rect);
		}
	};

	static Scene scene;
	return &scene;
}

IGR2DScene* TestScene()
{
	return TestDrawRect();
}