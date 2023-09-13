#include "sexystudio.impl.h"
#include <vector>
#include <rococo.maths.h>
#include <rococo.strings.h>
#include <rococo.os.h>

using namespace Rococo;
using namespace Rococo::SexyStudio;

namespace
{
	struct DefaultWidgetSet : IWidgetSetSupervisor
	{
		WidgetContext& context;
		Rococo::Windows::IWindow& parent;
		mutable std::vector<IGuiWidget*> widgets;

		DefaultWidgetSet(Rococo::Windows::IWindow& _parent, WidgetContext& _context):
			context(_context),
			parent(_parent)
		{

		}

		~DefaultWidgetSet()
		{
			for (auto* widget : widgets)
			{
				widget->Free();
			}
		}

		void Add(IGuiWidget* widget) override
		{
			widgets.push_back(widget);
		}

		void Free() override
		{
			delete this;
		}

		IGuiWidget** begin() override
		{
			return widgets.empty() ? nullptr : &widgets[0];
		}

		IGuiWidget** end() override
		{
			return widgets.empty() ? nullptr : &widgets[0] + widgets.size();
		}

		const IGuiWidget** begin() const override
		{
			if (widgets.empty())
			{
				return nullptr;
			}
			else
			{
				auto* result = const_cast<IGuiWidget**>(&widgets[0]);
				return const_cast<const IGuiWidget**>(result);
			}
		}

		const IGuiWidget** end() const override
		{
			if (widgets.empty())
			{
				return nullptr;
			}
			else
			{
				auto* result = const_cast<IGuiWidget**>(&widgets[0] + widgets.size());
				return const_cast<const IGuiWidget**>(result);
			}
		}
		 
		Rococo::Windows::IWindow& Parent() override
		{
			return parent;
		}

		WidgetContext& Context() override
		{
			return context;
		}
	};
}

namespace Rococo::SexyStudio
{
	void SetPosition(IWindow& window, const GuiRect& rect)
	{
		MoveWindow(window, rect.left, rect.top, Width(rect), Height(rect), TRUE);
	}

	Vec2i GetSpan(IWindow& window)
	{
		RECT rect;
		GetClientRect(window, &rect);
		return { rect.right - rect.left, rect.bottom - rect.top };
	}

	IWidgetSetSupervisor* CreateDefaultWidgetSet(Rococo::Windows::IWindow& parent, WidgetContext& context)
	{
		return new DefaultWidgetSet(parent, context);
	}

	HFONT SetFont(int size, cstr name, HFONT oldFont, HWND hTarget)
	{
		LOGFONTA font = { 0 };
		font.lfHeight = size;
		SafeFormat(font.lfFaceName, LF_FACESIZE, name);

		if (oldFont)
		{
			DeleteObject((HGDIOBJ)oldFont);
		}

		HFONT hFont = CreateFontIndirectA(&font);
		if (!hFont)
		{
			try
			{
				Throw(GetLastError(), "Error creating listview font, %s size %d", name, size);
			}
			catch (IException& ex)
			{
				THIS_WINDOW owner(GetAncestor(hTarget, GA_ROOT));
				Windows::ShowErrorBox(owner, ex, "SexyStudio error");
			}

			SafeFormat(font.lfFaceName, LF_FACESIZE, "Courier New");
			font.lfHeight = -11;

			hFont = CreateFontIndirectA(&font);
		}

		SendMessage(hTarget, WM_SETFONT, (WPARAM)hFont, TRUE);
		return hFont;
	}
}