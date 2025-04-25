#include <rococo.gui.retained.ex.h>
#include <rococo.maths.i32.h>
#include <rococo.strings.h>
#include <rococo.hashtable.h>
#include <vector>

using namespace Rococo;
using namespace Rococo::Gui;
using namespace Rococo::Strings;

namespace GRANON
{
	struct GRControlPrompt : IGRWidgetControlPrompt, IGRWidgetSupervisor
	{
		IGRPanel& panel;

		float transparency = 1.0f;

		GRControlPrompt(IGRPanel& owningPanel) : panel(owningPanel)
		{
		}

		virtual ~GRControlPrompt()
		{

		}

		IGRWidget& Widget() override
		{
			return *this;
		}

		void SetTransparency(float f) override
		{
			transparency = clamp(f, 0.0f, 1.0f);
		}

		IGRPanel& Panel() override
		{
			return panel;
		}

		void Free() override
		{
			delete this;
		}

		EGREventRouting OnCursorClick(GRCursorEvent& ce) override
		{
			UNUSED(ce);
			return EGREventRouting::NextHandler;
		}

		EGREventRouting OnCursorMove(GRCursorEvent& ce) override
		{
			UNUSED(ce);
			return EGREventRouting::NextHandler;
		}

		void OnCursorEnter() override
		{

		}

		void OnCursorLeave() override
		{

		}

		GRFontId fontId = GRFontId::MENU_FONT;
		GRAnchorPadding padding{ 2,2,2,2 };
		int cellPadding = 2;

		void Render(IGRRenderContext& g) override
		{
			DrawPanelBackgroundEx(
				panel,
				g, 
				EGRSchemeColourSurface::CONTAINER_BACKGROUND,
				EGRSchemeColourSurface::CONTAINER_TOP_LEFT,
				EGRSchemeColourSurface::CONTAINER_BOTTOM_RIGHT,
				transparency
			);

			cstr controlType = GetCustodian(panel).GetLastKnownControlType();

			GuiRect textRect = panel.AbsRect();
			GRAlignmentFlags alignment;
			alignment.Add(EGRAlignment::VCentre).Add(EGRAlignment::Left); 

			for (auto& p : prompts)
			{
				char fqKey[128];
				SafeFormat(fqKey, "%s.%s", p.iconId.c_str(), controlType);
				auto i = pathToImage.find(fqKey);
				if (i != pathToImage.end())
				{
					auto* image = i->second.image;
					if (image)
					{
						Vec2i imageSpan = image->Span();
						double aspectRatio = imageSpan.y != 0 ? imageSpan.x / (double) imageSpan.y : 0;

						GuiRect imageRect = textRect;
						imageRect.right = imageRect.left + (int) aspectRatio * Height(imageRect);
						g.DrawImageStretched(*image, imageRect);
						textRect.left = imageRect.right;
					}
				}

				Vec2i textSpan = g.Fonts().EvaluateMinimalSpan(fontId, p.text.to_fstring());
				textRect.left += cellPadding;
				textRect.right = textRect.left + textSpan.x;
				g.DrawText(fontId, textRect, alignment, { 0,0 }, p.text.to_fstring(), RGBAb(255, 255, 255));
				textRect.left = textRect.right;
			}
		}

		EGREventRouting OnChildEvent(GRWidgetEvent&, IGRWidget&)
		{
			return EGREventRouting::NextHandler;
		}

		EGREventRouting OnKeyEvent(GRKeyEvent&) override
		{
			return EGREventRouting::NextHandler;
		}

		EGRQueryInterfaceResult QueryInterface(IGRBase** ppOutputArg, cstr interfaceId) override
		{
			return QueryForParticularInterface<IGRWidgetControlPrompt>(this, ppOutputArg, interfaceId);
		}

		cstr GetImplementationTypeName() const override
		{
			return "GRControlPrompt";
		}

		struct Prompt
		{
			HString iconId;
			HString text;
		};

		struct ImageInfo
		{
			HString imagePath;
			IGRImage* image = nullptr;
		};

		stringmap<ImageInfo> pathToImage;

		std::vector<Prompt> prompts;

		void AddIcon(cstr iconId, cstr controlType, cstr imagePath) override
		{
			char fqKey[128];
			SafeFormat(fqKey, "%s.%s", iconId, controlType);

			ImageInfo img;
			img.imagePath = imagePath;
			img.image = GetCustodian(panel).CreateImageFromPath(fqKey, imagePath);
			pathToImage[fqKey] = img;
		}

		void AddPrompt(cstr iconId, cstr text) override
		{
			prompts.emplace_back(Prompt { iconId, text });
		}
	};

	struct GRPromptFactory : IGRWidgetFactory
	{
		IGRWidget& CreateWidget(IGRPanel& panel)
		{
			return *new GRControlPrompt(panel);
		}
	};
}

namespace Rococo::Gui
{
	ROCOCO_GUI_RETAINED_API cstr IGRWidgetControlPrompt::InterfaceId()
	{
		return "IGRWidgetControlPrompt";
	}

	ROCOCO_GUI_RETAINED_API IGRWidgetControlPrompt& CreateControlPrompt(IGRWidget& parent)
	{
		auto& gr = parent.Panel().Root().GR();
		GRANON::GRPromptFactory factory;
		auto& div = static_cast<GRANON::GRControlPrompt&>(gr.AddWidget(parent.Panel(), factory));
		return div;
	}
}