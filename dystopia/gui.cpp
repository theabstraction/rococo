#include "dystopia.h"
#include <string>

#include "rococo.renderer.h"
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
		Vec2 span;
	};

	Fonts::FontColour FontColourFromRGBAb(RGBAb colour)
	{
		Fonts::FontColour* pCol = (Fonts::FontColour*) &colour;
		return *pCol;
	}

	class HorizontalCentredText : public Fonts::IDrawTextJob
	{
	private:
		const wchar_t* text;
		Fonts::FontColour colour;
		int fontIndex;

	public:
		Quad target;

		HorizontalCentredText(int _fontIndex, const wchar_t* _text, Fonts::FontColour _colour) : text(_text), colour(_colour), fontIndex(_fontIndex)
		{
			float minFloat = std::numeric_limits<float>::min();
			float maxFloat = std::numeric_limits<float>::max();
			target = Quad(maxFloat, maxFloat, minFloat, minFloat);
		}

		void DrawNextGlyph(char c, Fonts::IGlyphBuilder& builder)
		{
			Quad outputRect;
			builder.AppendChar(c, outputRect);

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
				else
				{
					DrawNextGlyph((char)c, builder);
				}
			}
		}
	};

	void RenderDialog(DialogBox& dialog, IGuiRenderContext& gc)
	{
		HorizontalCentredText text(3, dialog.message.c_str(), FontColourFromRGBAb(RGBAb{ 0xFF, 0xFF, 0xFF, 0xFF }));
		gc.RenderText(Vec2i{ 10, 10 }, text);
	}

	class Gui : public IGuiSupervisor
	{
	private:
		IRenderer& renderer;
		DialogBox modalDialog;

	public:
		Gui(IRenderer& _renderer) :renderer(_renderer) {}

		virtual void AppendKeyboardEvent(const KeyboardEvent& ke)
		{
		}

		virtual void AppendMouseEvent(const MouseEvent& me)
		{
			if (me.HasFlag(MouseEvent::LUp))
			{
				modalDialog.isOperational = false;
			}
		}

		virtual void Free() { delete this; }

		virtual bool HasFocus() const { return modalDialog.isOperational;  }

		virtual void ShowDialogBox(const Vec2& span, const fstring& title, const fstring& message, const fstring& buttons)
		{
			modalDialog = { true, title.buffer, message.buffer, buttons.buffer, span };
		}

		virtual void Render(IGuiRenderContext& gc)
		{
			if (modalDialog.isOperational)
			{
				RenderDialog(modalDialog, gc);
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