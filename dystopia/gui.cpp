#include "dystopia.h"
#include <string>

#include "rococo.renderer.h"
#include "rococo.ui.h"
#include "rococo.fonts.h"

using namespace Dystopia;
using namespace Rococo;

namespace
{
	struct DialogBox
	{
		bool isOperational;
		std::wstring title;
		std::wstring message;
		std::wstring buttons;
		Vec2i span;
		int32 retzone;
		int32 hypzone;
	};

	Fonts::FontColour FontColourFromRGBAb(RGBAb colour)
	{
		Fonts::FontColour* pCol = (Fonts::FontColour*) &colour;
		return *pCol;
	}

	Quad GuiRectToQuad (const GuiRect& rect)
	{
		return Quad((float)rect.left, (float)rect.top, (float)rect.right, (float)rect.bottom);
	}

	class LeftAlignedText : public Fonts::IDrawTextJob
	{
	private:
		const wchar_t* text;
		Fonts::FontColour colour;
		int fontIndex;
		float lastCellHeight;
		Quad targetRect;
		int retzone;
		int hypzone;
	public:
		LeftAlignedText(const GuiRect& _targetRect, int _retzone, int _hypzone, int _fontIndex, const wchar_t* _text, Fonts::FontColour _colour) :
			targetRect(GuiRectToQuad(_targetRect)), retzone(_retzone), hypzone(_hypzone),
			text(_text), colour(_colour), fontIndex(_fontIndex), lastCellHeight(10.0f)
		{
		}

		void DrawNextGlyph(char c, Fonts::IGlyphBuilder& builder)
		{
			Quad outputRect;
			builder.AppendChar(c, outputRect);

			lastCellHeight = outputRect.bottom - outputRect.top;
		}

		virtual void OnDraw(Fonts::IGlyphBuilder& builder)
		{
			builder.SetTextColour(colour);
			builder.SetShadow(false);
			builder.SetFontIndex(fontIndex);

			builder.SetClipRect(Quad((float)targetRect.left, (float)targetRect.top, (float)targetRect.right, (float)targetRect.bottom));

			builder.SetCursor(Vec2{ targetRect.left, targetRect.top });

			const float retX = targetRect.right - retzone;
			const float hypX = targetRect.right - hypzone;

			for (const wchar_t* p = text; *p != 0; p++)
			{
				wchar_t c = *p;

				Vec2 nextGlyphPos = builder.GetCursor();

				if (c >= 255) c = '?';

				if (c == '\t')
				{
					if (nextGlyphPos.x > retX)
					{
						builder.SetCursor(Vec2{ targetRect.left, nextGlyphPos.y + lastCellHeight });
						continue;
					}

					for (int i = 0; i < 4; ++i)
					{
						DrawNextGlyph(' ', builder);
					}
				}
				else if (c == ' ')
				{
					if (nextGlyphPos.x > retX)
					{
						builder.SetCursor(Vec2{ targetRect.left, nextGlyphPos.y + lastCellHeight });
						continue;
					}

					DrawNextGlyph(' ', builder);
				}
				else if (c == '\n')
				{
					builder.SetCursor(Vec2{ targetRect.left, nextGlyphPos.y + lastCellHeight });
				}
				else
				{
					if (nextGlyphPos.x > hypX)
					{
						DrawNextGlyph('-', builder);
						builder.SetCursor(Vec2{ targetRect.left, nextGlyphPos.y + lastCellHeight });
						DrawNextGlyph(' ', builder);
						DrawNextGlyph('-', builder);
					}
					DrawNextGlyph((char)c, builder);
				}
			}
		}
	};

	class HorizontalCentredText : public Fonts::IDrawTextJob
	{
	private:
		const wchar_t* text;
		Fonts::FontColour colour;
		int fontIndex;
		float lastCellHeight;
	public:
		Quad target;

		HorizontalCentredText(int _fontIndex, const wchar_t* _text, Fonts::FontColour _colour) :
			text(_text), colour(_colour), fontIndex(_fontIndex), lastCellHeight(10.0f)
		{
			float minFloat = std::numeric_limits<float>::min();
			float maxFloat = std::numeric_limits<float>::max();
			target = Quad(maxFloat, maxFloat, minFloat, minFloat);
		}

		void DrawNextGlyph(char c, Fonts::IGlyphBuilder& builder)
		{
			Quad outputRect;
			builder.AppendChar(c, outputRect);

			lastCellHeight = outputRect.bottom - outputRect.top;

			if (outputRect.left < target.left) target.left = outputRect.left;
			if (outputRect.right > target.right) target.right = outputRect.right;
			if (outputRect.bottom > target.bottom) target.bottom = outputRect.bottom;
			if (outputRect.top < target.top) target.top = outputRect.top;
		}

		virtual void OnDraw(Fonts::IGlyphBuilder& builder)
		{
			builder.SetTextColour(colour);
			builder.SetShadow(false);
			builder.SetFontIndex(fontIndex);

			float fMin = -1000000.0f;
			float fMax = 1000000.0f;
			builder.SetClipRect(Quad(fMin, fMin, fMax, fMax));

			Vec2 firstGlyphPos = builder.GetCursor();

			for (const wchar_t* p = text; *p != 0; p++)
			{
				wchar_t c = *p;

				if (c >= 255) c = '?';

				if (c == '\t')
				{
					for (int i = 0; i < 4; ++i)
					{
						DrawNextGlyph(' ', builder);
					}
				}
				else if (c == '\n')
				{
					Vec2 nextGlyphPos = builder.GetCursor();
					builder.SetCursor(Vec2{ firstGlyphPos.x, nextGlyphPos.y + lastCellHeight });
				}
				else
				{
					DrawNextGlyph((char)c, builder);
				}
			}
		}
	};

	void DrawRectangle(IGuiRenderContext& grc, const GuiRect& grect, RGBAb diag, RGBAb backdiag)
	{
		Quad rect{ (float) grect.left, (float)grect.top, (float)grect.right, (float)grect.bottom };
		GuiVertex q[] =
		{
			{ rect.left,  rect.top,    1.0f, 0, diag,     0, 0, 0 },
			{ rect.right, rect.top,    1.0f, 0, backdiag, 0, 0, 0 },
			{ rect.right, rect.bottom, 1.0f, 0, diag,     0, 0, 0 },
			{ rect.right, rect.bottom, 1.0f, 0, diag,     0, 0, 0 },
			{ rect.left,  rect.bottom, 1.0f, 0, backdiag, 0, 0, 0 },
			{ rect.left,  rect.top,    1.0f, 0, diag,     0, 0, 0 },
		};

		grc.AddTriangle(q);
		grc.AddTriangle(q + 3);
	}

	Vec2i TopLeft(const GuiRect& rect) { return Vec2i{ rect.left, rect.top }; }
	Vec2i Span(const GuiRect& rect) { return Vec2i{ rect.right - rect.left, rect.bottom - rect.top }; }

	void DrawBorderAround(IGuiRenderContext& grc, const GuiRect& rect, const Vec2i& width, RGBAb diag, RGBAb backdiag)
	{
		GuiRect topRect{ rect.left - width.x, rect.top - width.y, rect.right, rect.top };
		DrawRectangle(grc, topRect, diag, diag);

		GuiRect bottomRect{ rect.left - width.x, rect.bottom, rect.right, rect.bottom + width.y };
		DrawRectangle(grc, bottomRect, backdiag, backdiag);

		GuiRect leftRect{ rect.left - width.x, rect.top, rect.left, rect.bottom };
		DrawRectangle(grc, leftRect, backdiag, backdiag);

		GuiRect rightRect{ rect.right, rect.top - width.y, rect.right + width.x, rect.bottom + width.y };
		DrawRectangle(grc, rightRect, diag, diag);
	}

	Vec2i GetScreenCentre(const GuiMetrics& metrics)
	{
		return Vec2i{  metrics.screenSpan.x >> 1, metrics.screenSpan.y >> 1};
	} 

	Vec2i RenderTextVCentred(IGuiRenderContext& grc, int32 x, int32 top, RGBAb colour, const wchar_t* text, int fontIndex)
	{
		HorizontalCentredText job(fontIndex, text, FontColourFromRGBAb(colour));
		Vec2i span = grc.EvalSpan(Vec2i{ 0,0 }, job);
		grc.RenderText(Vec2i{ x - (span.x >> 1), top }, job);
		return span;
	}

	struct IStringCallback
	{
		virtual void operator()(const wchar_t* text) = 0;
	};

	void SplitString(const wchar_t* text, size_t length, const wchar_t* seperators, IStringCallback& onSubString)
	{
		if (length == 0) length = wcslen(text);
		size_t bytecount = sizeof(wchar_t) * (length + 1);
		wchar_t* buf = (wchar_t*)_alloca(bytecount);
		memcpy_s(buf, bytecount, text, bytecount);
		buf[length] = 0;

		wchar_t* next_token = nullptr;
		wchar_t* token = wcstok_s(buf, L"|", &next_token);
		while (token != nullptr)
		{
			onSubString(token);
			token = wcstok_s(nullptr, L"|", &next_token);
		}
	}

	size_t CountSubStrings(const wchar_t* text, size_t length, const wchar_t* seperators)
	{
		struct : IStringCallback
		{
			size_t count;
			virtual void operator()(const wchar_t* text)
			{
				count++;
			}
		} cb;

		cb.count = 0;
		SplitString(text, length, seperators, cb);
		return cb.count;
	}

	Vec2i TopCentre(const GuiRect& rect)
	{
		return Vec2i{ (rect.left + rect.right) >> 1, rect.top };
	}

	bool IsPointInRect(const Vec2i& p, const GuiRect& rect)
	{
		return (p.x > rect.left && p.x < rect.right && p.y > rect.top && p.y < rect.bottom);
	}

	struct ButtonParser
	{
		enum  { CAPACITY  = 128 };
		wchar_t name[CAPACITY];
		wchar_t script[CAPACITY];

		ButtonParser(const wchar_t* text)
		{
			name[0] = script[0] = 0;

			for (const wchar_t* s = text; *s != 0; s++)
			{
				if (*s == L'=')
				{
					size_t nTokenChars = s - text + 1;
					if (nTokenChars >= CAPACITY-1) nTokenChars = CAPACITY - 2;			
					memcpy_s(name, sizeof(wchar_t) * CAPACITY, text, sizeof(wchar_t) * nTokenChars);
					name[nTokenChars - 1] = 0;

					const wchar_t* pscript = s + 1;
					wcsncpy_s(script, pscript, _TRUNCATE);
				}
			}
		}
	};

	// Draw buttons in a horizontal line and returns index of button under cursor, or negative if no button is under the cursor
	int32 DrawHorzButtons(const wchar_t* text, size_t length, const wchar_t* seperators, const Vec2i& pos, IGuiRenderContext& grc, bool rightToLeft)
	{
		struct : IStringCallback
		{
			Vec2i nextPos;
			IGuiRenderContext* grc;
			Vec2i cursorPos;
			int32 cursorIndex;
			int32 index;
			bool rightToLeft;

			virtual void operator()(const wchar_t* text)
			{
				ButtonParser button(text);

				HorizontalCentredText job(3, button.name, 0xFFFFFFFF);
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
				
				DrawRectangle(*grc, buttonRect, buttonCol1, buttonCol2);
				DrawBorderAround(*grc, buttonRect, Vec2i{ 1, 1 }, borderCol1, borderCol2);
				grc->RenderText(TopCentre(buttonRect) - Vec2i{ span.x >> 1, 0 }, job);

				nextPos.x = rightToLeft ? buttonRect.left - 20 :  buttonRect.right + 20;

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

	int32 RenderDialog(DialogBox& dialog, IGuiRenderContext& grc)
	{
		GuiMetrics metrics;
		grc.Renderer().GetGuiMetrics(metrics);

		Vec2i centre = GetScreenCentre(metrics);

		Vec2i topLeft{ (metrics.screenSpan.x - dialog.span.x) >> 1, (metrics.screenSpan.y - dialog.span.y) >> 1 };

		GuiRect rect{ topLeft.x, topLeft.y, topLeft.x + dialog.span.x, topLeft.y + dialog.span.y };

		DrawRectangle(grc, rect, RGBAb(64, 0, 0, 0xE0), RGBAb(0, 0, 64, 0xE0));
		DrawBorderAround(grc, rect, Vec2i{ 2,2 }, RGBAb(192, 192, 192, 0xFF), RGBAb(128, 128, 128, 0xFF));

		Vec2i introSpan = RenderTextVCentred(grc, centre.x, (int32)topLeft.y, RGBAb{ 0xFF, 0xFF, 0xFF, 0xFF }, dialog.title.c_str(), 3);

		int32 buttonHeight = 60;

		GuiRect targetRect(rect.left, rect.top + introSpan.y, rect.right, rect.bottom - buttonHeight);
		LeftAlignedText body(targetRect, dialog.retzone, dialog.hypzone, 7, dialog.message.c_str(), FontColourFromRGBAb(RGBAb{ 0xFF, 0xFF, 0xFF, 0xFF }));
		grc.RenderText(Vec2i{ 0, 0 }, body);

		const wchar_t* buttonText = dialog.buttons.c_str();
		size_t len = dialog.buttons.length();

		bool rightToLeft = false;
		int32 buttonPos = rect.left + 20;

		if (*buttonText == L'>')
		{
			rightToLeft = true;
			len--;
			buttonText++;
			buttonPos = rect.right - 20;
		}

		return DrawHorzButtons(buttonText, len, L"|", Vec2i{ buttonPos, rect.bottom - 40}, grc, rightToLeft);
	}

	void InvokeButtonHandler(IEventCallback<GuiEventArgs>* handler, int buttonIndex, const std::wstring& buttonDef)
	{
		if (handler == nullptr || buttonIndex < 0 || buttonDef.empty())
			return;

		struct : IStringCallback
		{
			int count;
			int buttonIndex;
			IEventCallback<GuiEventArgs>* guiEvent;
			virtual void operator()(const wchar_t* text)
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
		cb.guiEvent = handler;

		const wchar_t* buttonText = buttonDef.c_str();
		size_t len = buttonDef.length();

		if (*buttonText == L'>')
		{
			buttonText++;
			len--;
		}

		SplitString(buttonText, len, L"'", cb);
	}

	class Gui : public IGuiSupervisor
	{
	private:
		IRenderer& renderer;
		DialogBox modalDialog;

		int lastHighlight;

		IEventCallback<GuiEventArgs>* guiEventHandler;
	public:
		Gui(IRenderer& _renderer) :renderer(_renderer), guiEventHandler(nullptr) {}

		virtual void SetEventHandler(IEventCallback<GuiEventArgs>* guiEventHandler)
		{
			this->guiEventHandler = guiEventHandler;
		}

		virtual void AppendKeyboardEvent(const KeyboardEvent& ke)
		{
		}

		virtual void AppendMouseEvent(const MouseEvent& me)
		{
			if (me.HasFlag(MouseEvent::LUp) && lastHighlight >= 0 && modalDialog.isOperational)
			{
				InvokeButtonHandler(guiEventHandler, lastHighlight, modalDialog.buttons);
				modalDialog.isOperational = false;
			}
		}

		virtual void Free() { delete this; }

		virtual bool HasFocus() const { return modalDialog.isOperational;  }

		virtual void ShowDialogBox(const Vec2i& span, int32 retzone, int32 hypzone, const fstring& title, const fstring& message, const fstring& buttons)
		{
			modalDialog = { true, title.buffer, message.buffer, buttons.buffer, span, retzone, hypzone };
		}

		virtual void Render(IGuiRenderContext& gc)
		{
			if (modalDialog.isOperational)
			{
				lastHighlight = RenderDialog(modalDialog, gc);
			}
		}
	};
}

namespace Dystopia
{
	IGuiSupervisor* CreateGui(IRenderer& renderer)
	{
		return new Gui(renderer);
	}
}