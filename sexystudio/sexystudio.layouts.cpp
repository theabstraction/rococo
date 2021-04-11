#include "sexystudio.api.h"
#include <rococo.maths.h>

namespace Rococo::SexyStudio::Widgets
{
	// These functions create a layout object.
	// These are applied in sequence to layout a child control relative to its parent

	void AnchorToParent(IGuiWidget& widget, int leftBorder, int topBorder, int rightBorder, int bottomBorder)
	{
		struct L : ILayout
		{
			int leftBorder = 0;
			int topBorder = 0;
			int rightBorder = 0;
			int bottomBorder = 0;

			void Layout(IGuiWidget& widget, GuiRect& rect) override
			{
				// Ignore the rectangle input
				Vec2i span = GetParentSpan(widget);
				rect = GuiRect {
					leftBorder,
					topBorder,
					span.x - rightBorder,
					span.y - bottomBorder
				};
			}

			void Free() override
			{
				delete this;
			}
		};

		auto* l = new L();
		l->leftBorder = leftBorder;
		l->topBorder = topBorder;
		l->rightBorder = rightBorder;
		l->bottomBorder = bottomBorder;
		widget.AddLayoutModifier(l);
	}

	void AnchorToParentLeft(IGuiWidget& widget, int pixelBorder)
	{
		struct L: ILayout
		{
			int pixelBorder = 0;

			void Layout(IGuiWidget& widget, GuiRect& rect) override
			{
				rect.left = pixelBorder;
			}

			void Free() override
			{
				delete this;
			}
		};

		auto* l = new L();
		l->pixelBorder = pixelBorder;

		widget.AddLayoutModifier(l);
	}

	void AnchorToParentRight(IGuiWidget& widget, int pixelBorder)
	{
		struct L : ILayout
		{
			int pixelBorder = 0;

			void Layout(IGuiWidget& widget, GuiRect& rect) override
			{
				auto parentSpan = Widgets::GetParentSpan(widget);
				if (parentSpan.x <= 0)
				{
					Throw(0, "%s: Bad parent span { %d, %d }", __FUNCTION__, parentSpan.x, parentSpan.y);
				}
				rect.right = parentSpan.x - pixelBorder;
			}

			void Free() override
			{
				delete this;
			}
		};

		auto* l = new L();
		l->pixelBorder = pixelBorder;

		widget.AddLayoutModifier(l);
	}

	void AnchorToParentTop(IGuiWidget& widget, int pixelBorder)
	{
		struct L : ILayout
		{
			int pixelBorder = 0;

			void Layout(IGuiWidget& widget, GuiRect& rect) override
			{
				rect.top = pixelBorder;
			}

			void Free() override
			{
				delete this;
			}
		};

		auto* l = new L();
		l->pixelBorder = pixelBorder;

		widget.AddLayoutModifier(l);
	}

	void ExpandBottomFromTop(IGuiWidget& widget, int pixels)
	{
		struct L : ILayout
		{
			int pixelHeight = 0;
			void Layout(IGuiWidget& widget, GuiRect& rect) override
			{
				if (rect.top < 0)
				{
					Throw(0, "%s: rec.top is %d (undefined)", __FUNCTION__, rect.top);
				}
				rect.bottom = rect.top + pixelHeight;
			}

			void Free() override
			{
				delete this;
			}
		};

		auto* l = new L();
		l->pixelHeight = pixels;

		widget.AddLayoutModifier(l);
	}

	void ExpandLeftFromRight(IGuiWidget& widget, int pixels)
	{
		struct L : ILayout
		{
			int pixels = 0;

			void Layout(IGuiWidget& widget, GuiRect& rect) override
			{
				rect.left = rect.right - pixels;
			}

			void Free() override
			{
				delete this;
			}
		};

		auto* l = new L();
		l->pixels = pixels;

		widget.AddLayoutModifier(l);
	}
}