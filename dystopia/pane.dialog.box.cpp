#include "dystopia.h"
#include "rococo.renderer.h"
#include "human.types.h"

#include "rococo.ui.h"
#include "rococo.fonts.h"

#include <string>

#include "dystopia.ui.h"

using namespace Rococo;
using namespace Dystopia;

namespace
{
	struct ButtonParser
	{
		enum { CAPACITY = 128 };
		rchar name[CAPACITY];
		rchar script[CAPACITY];

		ButtonParser(cstr text)
		{
			name[0] = script[0] = 0;

			for (cstr s = text; *s != 0; s++)
			{
				if (*s == L'=')
				{
					size_t nTokenChars = s - text + 1;
					if (nTokenChars >= CAPACITY - 1) nTokenChars = CAPACITY - 2;
					memcpy_s(name, sizeof(rchar) * CAPACITY, text, sizeof(rchar) * nTokenChars);
					name[nTokenChars - 1] = 0;

					cstr pscript = s + 1;
					wcsncpy_s(script, pscript);
				}
			}
		}
	};

	// Draw buttons in a horizontal line and returns index of button under cursor, or negative if no button is under the cursor
	int32 DrawHorzButtons(cstr text, size_t length, cstr seperators, const Vec2i& pos, IGuiRenderContext& grc, bool rightToLeft)
	{
		struct : IEventCallback<cstr>
		{
			Vec2i nextPos;
			IGuiRenderContext* grc;
			Vec2i cursorPos;
			int32 cursorIndex;
			int32 index;
			bool rightToLeft;

			virtual void OnEvent(cstr text)
			{
				ButtonParser button(text);

				Graphics::StackSpaceGraphics ss;
				auto& job = Graphics::CreateHorizontalCentredText(ss, 3, button.name, 0xFFFFFFFF);
				auto span = grc->EvalSpan(Vec2i{ 0,0 }, job);

				GuiRect buttonRect;

				if (rightToLeft)
				{
					buttonRect = GuiRect(nextPos.x - span.x - 6, nextPos.y, nextPos.x, nextPos.y + span.y);
				}
				else
				{
					buttonRect = GuiRect(nextPos.x, nextPos.y, nextPos.x + span.x + 6, nextPos.y + span.y);
				}

				RGBAb buttonCol1(32, 0, 0, 0xFF);
				RGBAb buttonCol2(0, 0, 32, 0xFF);

				RGBAb borderCol1(192, 192, 192, 0xFF);
				RGBAb borderCol2(128, 128, 128, 0xFF);

				if (IsPointInRect(cursorPos, buttonRect))
				{
					cursorIndex = index;

					buttonCol1 = RGBAb(128, 0, 0, 0xFF);
					buttonCol2 = RGBAb(0, 0, 128, 0xFF);
					borderCol1 = borderCol2 = RGBAb(255, 255, 255, 255);
				}

				Graphics::DrawRectangle(*grc, buttonRect, buttonCol1, buttonCol2);
				Graphics::DrawBorderAround(*grc, buttonRect, Vec2i{ 1, 1 }, borderCol1, borderCol2);
				grc->RenderText(TopCentre(buttonRect) - Vec2i{ span.x >> 1, 0 }, job);

				nextPos.x = rightToLeft ? buttonRect.left - 20 : buttonRect.right + 20;

				if (IsPointInRect(cursorPos, buttonRect))
				{
					cursorIndex = index;
				}

				index++;
			}
		} cb;

		GuiMetrics metrics;
		grc.Renderer().GetGuiMetrics(metrics);

		cb.index = 0;
		cb.cursorPos = metrics.cursorPosition;
		cb.cursorIndex = -1;

		cb.nextPos = pos;
		cb.grc = &grc;
		cb.rightToLeft = rightToLeft;

		SplitString(text, length, seperators, cb);

		return cb.cursorIndex;
	}

	void InvokeButtonHandler(IEventCallback<GuiEventArgs>& handler, int buttonIndex, const std::string& buttonDef)
	{
		if (buttonIndex < 0 || buttonDef.empty())
			return;

		struct : IEventCallback<cstr>
		{
			int count;
			int buttonIndex;
			IEventCallback<GuiEventArgs>* guiEvent;
			virtual void OnEvent(cstr text)
			{
				if (count == buttonIndex)
				{
					ButtonParser button(text);
					GuiEventArgs args{ button.script, GuiEventType_CURSOR_BUTTON1_RELEASED };
					if (guiEvent) guiEvent->OnEvent(args);
				}
				count++;
			}
		} cb;

		cb.count = 0;
		cb.buttonIndex = buttonIndex;
		cb.guiEvent = &handler;

		cstr buttonText = buttonDef.c_str();
		size_t len = buttonDef.length();

		if (*buttonText == L'>')
		{
			buttonText++;
			len--;
		}

		SplitString(buttonText, len, L"'", cb);
	}


	class DialogBox : public IUIPaneSupervisor
	{
		Environment& e;
		
		std::string title;
		std::string message;
		std::string buttons;
		Vec2i span;
		int32 retzone;
		int32 hypzone;
		int lastHighlight;

		IEventCallback<GuiEventArgs>& guiEventHandler;
	public:
		DialogBox(Environment& _e,
			IEventCallback<GuiEventArgs>& _guiEventHandler,
			cstr _title,
			cstr _message, 
			cstr _buttons,
			Vec2i _span,
			int32 _retzone,
			int32 _hypzone) 
			
			:

			e(_e), 
			guiEventHandler(_guiEventHandler),
			title(_title),
			message(_message),
			buttons(_buttons),
			span(_span),
			retzone(_retzone),
			hypzone(_hypzone)
		{

		}

		virtual void Free()
		{
			delete this;
		}

		virtual Relay OnTimestep(const TimestepEvent& clock)
		{
			return Relay_None;
		}

		virtual Relay OnKeyboardEvent(const KeyboardEvent& ke)
		{
			return Relay_None;
		}

		virtual Relay OnMouseEvent(const MouseEvent& me)
		{
			if (me.HasFlag(MouseEvent::LUp) && lastHighlight >= 0)
			{
				InvokeButtonHandler(guiEventHandler, lastHighlight, buttons);
				e.uiStack.PopTop();
				Free();
			}
			else if (me.HasFlag(MouseEvent::LUp) && buttons.empty())
			{
				e.uiStack.PopTop();
				Free();
			}

			return Relay_None;
		}

		virtual void OnPop()
		{

		}

		virtual void RenderObjects(IRenderContext& rc)
		{
		}

		virtual void RenderGui(IGuiRenderContext& grc)
		{
			GuiMetrics metrics;
			grc.Renderer().GetGuiMetrics(metrics);

			Vec2i centre = Graphics::GetScreenCentre(metrics);

			Vec2i topLeft{ (metrics.screenSpan.x - span.x) >> 1, (metrics.screenSpan.y - span.y) >> 1 };

			GuiRect rect{ topLeft.x, topLeft.y, topLeft.x + span.x, topLeft.y + span.y };

			Graphics::DrawRectangle(grc, rect, RGBAb(64, 0, 0, 0xE0), RGBAb(0, 0, 64, 0xE0));
			Graphics::DrawBorderAround(grc, rect, Vec2i{ 2,2 }, RGBAb(192, 192, 192, 0xFF), RGBAb(128, 128, 128, 0xFF));

			Vec2i introSpan = Graphics::RenderHorizontalCentredText(grc, title.c_str(), RGBAb{ 0xFF, 0xFF, 0xFF, 0xFF }, 3, { centre.x, (int32)topLeft.y });

			int32 buttonHeight = 60;

			GuiRect targetRect(rect.left, rect.top + introSpan.y, rect.right, rect.bottom - buttonHeight);
			Graphics::StackSpaceGraphics ss;
			auto& body = CreateLeftAlignedText(ss, targetRect, retzone, hypzone, 7, message.c_str(), RGBAb{ 0xFF, 0xFF, 0xFF, 0xFF });
			grc.RenderText(Vec2i{ 0, 0 }, body);

			cstr buttonText = buttons.c_str();
			size_t len = buttons.length();
			if (len != 0)
			{
				bool rightToLeft = false;
				int32 buttonPos = rect.left + 20;

				if (*buttonText == L'>')
				{
					rightToLeft = true;
					len--;
					buttonText++;
					buttonPos = rect.right - 20;
				}

				lastHighlight = DrawHorzButtons(buttonText, len, L"|", Vec2i{ buttonPos, rect.bottom - 40 }, grc, rightToLeft);
			}
			else
			{
				lastHighlight = -1;
			}
		}

		virtual void OnLostTop()
		{

		}
	};
}

namespace Dystopia
{
	IUIPaneSupervisor* CreateDialogBox(Environment& e, IEventCallback<GuiEventArgs>& _handler,
		cstr _title,
		cstr _message,
		cstr _buttons,
		Vec2i _span,
		int32 _retzone,
		int32 _hypzone)
	{
		return new DialogBox(e, _handler, _title, _message, _buttons, _span, _retzone, _hypzone);
	}
}