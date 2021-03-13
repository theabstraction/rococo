#include "sexystudio.impl.h"
#include <rococo.maths.h>

namespace Rococo::SexyStudio
{
	// These functions create a layout object.
	// These are applied in sequence to layout a child control relative to its parent

	void AnchorToParentLeft(ILayoutControl& layouts, int pixelBorder)
	{
		struct L: ILayout
		{
			int pixelBorder = 0;

			void Layout(IWindow& parent, GuiRect& rect) override
			{
				RECT parentRect;
				GetClientRect(parent, &parentRect);
				rect.left = parentRect.left + pixelBorder;
			}

			void Free() override
			{
				delete this;
			}
		};

		auto* l = new L();
		l->pixelBorder = pixelBorder;

		layouts.AttachLayoutModifier(l);
	}

	void AnchorToParentRight(ILayoutControl& layouts, int pixelBorder)
	{
		struct L : ILayout
		{
			int pixelBorder = 0;

			void Layout(IWindow& parent, GuiRect& rect) override
			{
				RECT parentRect;
				GetClientRect(parent, &parentRect);
				int width = Width(rect);
				rect.right = parentRect.right - pixelBorder;
			}

			void Free() override
			{
				delete this;
			}
		};

		auto* l = new L();
		l->pixelBorder = pixelBorder;

		layouts.AttachLayoutModifier(l);
	}

	void AnchorToParentTop(ILayoutControl& layouts, int pixelBorder)
	{
		struct L : ILayout
		{
			int pixelBorder = 0;

			void Layout(IWindow& parent, GuiRect& rect) override
			{
				RECT parentRect;
				GetClientRect(parent, &parentRect);
				rect.top = parentRect.top + pixelBorder;
				int32 height = rect.bottom - rect.top;
				rect.bottom = rect.top + height;
			}

			void Free() override
			{
				delete this;
			}
		};

		auto* l = new L();
		l->pixelBorder = pixelBorder;

		layouts.AttachLayoutModifier(l);
	}

	void ExpandBottomFromTop(ILayoutControl& layouts, int pixels)
	{
		struct L : ILayout
		{
			int pixelHeight = 0;

			void Layout(IWindow& parent, GuiRect& rect) override
			{
				rect.bottom = rect.top + pixelHeight;
			}

			void Free() override
			{
				delete this;
			}
		};

		auto* l = new L();
		l->pixelHeight = pixels;

		layouts.AttachLayoutModifier(l);
	}

	void ExpandLeftFromRight(ILayoutControl& layouts, int pixels)
	{
		struct L : ILayout
		{
			int pixels = 0;

			void Layout(IWindow& parent, GuiRect& rect) override
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

		layouts.AttachLayoutModifier(l);
	}
}