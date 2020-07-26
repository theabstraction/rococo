#include "hv.h"
#include <vector>
#include <rococo.strings.h>

using namespace HV;

namespace
{
	void RenderHQText_LeftAligned_VCentre(IGuiRenderContext& grc, ID_FONT fontId, const GuiRect& rect, cstr text, RGBAb colour)
	{
		Vec2i span = grc.RenderHQText(fontId, text, { 0,0 }, RGBAb(0, 0, 0, 0));
		Vec2i pos{ rect.left, (rect.top + rect.bottom + span.y) >> 1 };
		grc.RenderHQText(fontId, text, pos, colour);
	}

	ROCOCOAPI IField
	{
		virtual cstr Name() const = 0;
		virtual const GuiRect& Rect() const = 0;
		virtual void SetRect(const GuiRect& rect) = 0;
		virtual void Render(IGuiRenderContext& grc) = 0;
		virtual void Free() = 0;
	};

	void RenderName(IGuiRenderContext& grc, cstr name, const GuiRect& lineRect, int32 width)
	{
		GuiRect nameRect = lineRect;
		nameRect.right = nameRect.left + width;
		RenderHQText_LeftAligned_VCentre(grc, ID_FONT{ 1 }, nameRect, name, RGBAb(255, 255, 255, 255));
	}

	void RenderEditorBackground(IGuiRenderContext& grc, cstr name, const GuiRect& lineRect, int32 width)
	{
		GuiRect editorRect
		{
			lineRect.left + width + 2,
			lineRect.top + 3,
			lineRect.right - 4,
			lineRect.bottom - 3
		};

		RGBAb edge1(224, 224, 224, 255);
		RGBAb edge2(255, 255, 255, 255);
		Graphics::DrawRectangle(grc, editorRect, RGBAb(0, 0, 0, 255), RGBAb(0, 0, 0, 255));
		Graphics::DrawBorderAround(grc, editorRect, { 1,1 }, edge1, edge2);
	}

	enum { NAME_WIDTH = 100 };

	struct UnboundedInt32Field : IField
	{
		HString name;
		GuiRect rect;

		int32 value;

		cstr Name() const override
		{
			return name;
		}

		const GuiRect& Rect() const  override
		{
			return rect;
		}

		void SetRect(const GuiRect& _rect)
		{
			rect = _rect;
		}

		void Render(IGuiRenderContext& grc)  override
		{
			RenderName(grc, name, rect, NAME_WIDTH);
			RenderEditorBackground(grc, name, rect, NAME_WIDTH);
		}

		void Free() override
		{
			delete this;
		}
	};

	struct BoundedInt32Field : IField
	{
		HString name;
		GuiRect rect;

		int32 value;
		int32 minValue;
		int32 maxValue;

		cstr Name() const override
		{
			return name;
		}

		const GuiRect& Rect() const  override
		{
			return rect;
		}

		void SetRect(const GuiRect& _rect)
		{
			rect = _rect;
		}

		void Render(IGuiRenderContext& grc)  override
		{
			RenderName(grc, name, rect, NAME_WIDTH);
			RenderEditorBackground(grc, name, rect, NAME_WIDTH);
		}

		void Free() override
		{
			delete this;
		}
	};

	struct StringField : IField
	{
		HString name;
		GuiRect rect;

		HString value;

		cstr Name() const override
		{
			return name;
		}

		const GuiRect& Rect() const  override
		{
			return rect;
		}

		void SetRect(const GuiRect& _rect)
		{
			rect = _rect;
		}

		void Render(IGuiRenderContext& grc)  override
		{
			RenderName(grc, name, rect, NAME_WIDTH);
			RenderEditorBackground(grc, name, rect, NAME_WIDTH);
		}

		void Free() override
		{
			delete this;
		}
	};

	struct FloatField : IField
	{
		HString name;
		GuiRect rect;

		float value;
		float minValue;
		float maxValue;

		cstr Name() const override
		{
			return name;
		}

		const GuiRect& Rect() const  override
		{
			return rect;
		}

		void SetRect(const GuiRect& _rect)
		{
			rect = _rect;
		}

		void Render(IGuiRenderContext& grc)  override
		{
			RenderName(grc, name, rect, NAME_WIDTH);
			RenderEditorBackground(grc, name, rect, NAME_WIDTH);
		}

		void Free() override
		{
			delete this;
		}
	};

	struct FieldEditor : IFieldEditor, IUIElement
	{
		std::vector<IField*> fields;

		FieldEditorContext context;
		FieldEditor(FieldEditorContext& _context): 
			context(_context)
		{

		}

		~FieldEditor()
		{
			Clear();
		}

		void Free() override
		{
			delete this;
		}

		IUIElement& UIElement() override { return *this; }

		void AddInt32FieldUnbounded(cstr name, int32 value) override
		{
			auto* field = new UnboundedInt32Field();
			field->value = value;
			field->name = name;
			auto* f = static_cast<IField*>(field);
			fields.push_back(f);
		}

		void AddInt32FieldBounded(cstr name, int32 value, int32 minValue, int32 maxValue) override
		{
			auto* field = new BoundedInt32Field();
			field->value = value;
			field->name = name;
			field->minValue = minValue;
			field->maxValue = maxValue;
			auto* f = static_cast<IField*>(field);
			fields.push_back(f);
		}

		void AddStringField(cstr name, cstr value)
		{
			auto* field = new StringField();
			field->value = value;
			field->name = name;
			auto* f = static_cast<IField*>(field);
			fields.push_back(f);
		}

		void AddFloat32FieldBounded(cstr name, float value, float minValue, float maxValue) override
		{
			auto* field = new FloatField();
			field->value = value;
			field->name = name;
			field->minValue = minValue;
			field->maxValue = maxValue;
			auto* f = static_cast<IField*>(field);
			fields.push_back(f);
		}

		void Clear() override
		{
			for (auto* i : fields)
			{
				i->Free();
			}

			fields.clear();
		}

		bool OnKeyboardEvent(const KeyboardEvent& key) override
		{
			return true;
		}

		void OnRawMouseEvent(const MouseEvent& ev) override
		{

		}

		void OnMouseMove(Vec2i cursorPos, Vec2i delta, int dWheel) override
		{

		}

		void OnMouseLClick(Vec2i cursorPos, bool clickedDown) override
		{

		}

		void OnMouseRClick(Vec2i cursorPos, bool clickedDown) override
		{

		}

		void Render(IGuiRenderContext& grc, const GuiRect& absRect) override
		{
			enum { CELL_HEIGHT = 32 };
			GuiRect lineRect = absRect;

			for (auto* f : fields)
			{
				lineRect.bottom = lineRect.top + CELL_HEIGHT;

				f->SetRect(lineRect);
				f->Render(grc);

				lineRect.top = lineRect.bottom + 1;
			}
		}
	};
}

namespace HV
{
	IFieldEditor* CreateFieldEditor(FieldEditorContext& context)
	{
		return new FieldEditor(context);
	}
}