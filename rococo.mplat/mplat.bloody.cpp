#include <rococo.mplat.h>
#include <rococo.strings.h>
#include <rococo.ui.h>
#include <rococo.textures.h>

#include <string>
#include <unordered_map>
#include <algorithm>

#include <rococo.file.browser.h>

namespace ANON
{
	using namespace Rococo;
	using namespace Rococo::Graphics;
	using namespace Rococo::Events;

	ROCOCOAPI IBloodyPropertyType
	{
		virtual void Click(bool clickedDown, Vec2i pos) = 0;
		virtual void Free() = 0;
		virtual cstr Name() const = 0;
		virtual void Render(IGuiRenderContext& rc, const GuiRect& rect, RGBAb colour) = 0;
		virtual RGBAb NameColour() const = 0;
		virtual cstr NotifyId() const = 0;
	};

	ROCOCOAPI IValidator
	{
		virtual bool IsLegal(char c, int charPos) const = 0;
		virtual void OnDetached(char* buffer) = 0;
	};

	class TextEditorBox : public IKeyboardSink
	{
		char* buffer;
		int32 capacity;
		int32 cursorPos = 0;
		Platform& platform;
		bool defaultToEnd;
		IValidator& validator;
		IEventCallback<IBloodyPropertyType>& dirtNotifier;
		IBloodyPropertyType& owner;
	public:
		TextEditorBox(Platform& _platform, IBloodyPropertyType& _owner, IEventCallback<IBloodyPropertyType>& _dirtNotifier, char* _buffer, size_t _capacity, bool _defaultToEnd, IValidator& _validator) :
			platform(_platform),
			owner(_owner),
			dirtNotifier(_dirtNotifier),
			buffer(_buffer),
			capacity((int32)_capacity),
			defaultToEnd(_defaultToEnd),
			validator(_validator)
		{
		}

		~TextEditorBox()
		{
			platform.gui.DetachKeyboardSink(this);
		}

		void Notify()
		{
			dirtNotifier.OnEvent(owner);
		}

		void PushBufferRightOne()
		{
			int32 len = (int32)strlen(buffer);
			if (len >= capacity - 1)
			{
				return;
			}

			for (int32 i = len; i > cursorPos; --i)
			{
				buffer[i] = buffer[i - 1];
			}
		}


		void DeleteRight(int pos)
		{
			int32 len = (int32)strlen(buffer);

			for (int32 i = pos; i < len; ++i)
			{
				buffer[i] = buffer[i + 1];
			}
		}

		void AddCharOverwrite(char c)
		{
			int len = (int)strlen(buffer);
			if (cursorPos < len)
			{
				buffer[cursorPos++] = c;
			}
			else if (len < capacity - 1)
			{
				buffer[cursorPos++] = c;
			}
		}

		void AddCharInsert(char c)
		{
			int len = (int)strlen(buffer);
			if (len < capacity - 1)
			{
				PushBufferRightOne();
				buffer[cursorPos++] = c;
			}
		}

		virtual bool OnKeyboardEvent(const KeyboardEvent& key)
		{
			dirtNotifier.OnEvent(owner);

			if (!key.IsUp())
			{
				switch (key.VKey)
				{
				case IO::VKCode_ENTER:
					platform.gui.DetachKeyboardSink(this);
					validator.OnDetached(buffer);
					return true;
				case IO::VKCode_BACKSPACE:
					if (cursorPos > 0)
					{
						DeleteRight(cursorPos - 1);
						cursorPos--;
					}
					return true;
				case IO::VKCode_DELETE:
					DeleteRight(cursorPos);
					return true;
				case IO::VKCode_HOME:
					cursorPos = 0;
					return true;
				case IO::VKCode_END:
					cursorPos = (int32)strlen(buffer);
					return true;
				case IO::VKCode_LEFT:
					if (cursorPos > 0)
					{
						cursorPos--;
					}
					return true;
				case IO::VKCode_RIGHT:
					if (cursorPos < (int32)strlen(buffer))
					{
						cursorPos++;
					}
					return true;
				}

				char c = (key.unicode > 0 && key.unicode < 128) ? key.unicode : 0;

				if (!validator.IsLegal(c, cursorPos))
				{
					OS::BeepWarning();
				}
				else
				{
					if (c >= 32)
					{
						if (platform.gui.IsOverwriting())
						{
							AddCharOverwrite(c);
						}
						else
						{
							AddCharInsert(c);
						}
						return true;
					}
				}
			}
			return true;
		}

		void Click(bool clickedDown)
		{
			dirtNotifier.OnEvent(owner);

			if (clickedDown)
			{
				if (platform.gui.CurrentKeyboardSink() == this)
				{
					platform.gui.DetachKeyboardSink(this);
					validator.OnDetached(buffer);
				}
				else
				{
					platform.gui.AttachKeyboardSink(this);
				}
			}
		}

		void Render(IGuiRenderContext& rc, const GuiRect& rect, RGBAb colour)
		{
			int32 len = (int32)strlen(buffer);
			if (cursorPos > len)
			{
				cursorPos = len;
			}

			int x = rect.left + 4;
			int y = Centre(rect).y;

			if (platform.gui.CurrentKeyboardSink() == this)
			{
				Rococo::Graphics::DrawRectangle(rc, rect, RGBAb(64, 0, 0, 128), RGBAb(64, 0, 0, 255));
				Rococo::Graphics::DrawBorderAround(rc, rect, { 1,1 }, RGBAb(255, 255, 255, 255), RGBAb(224, 224, 224, 255));
			}

			struct : IEventCallback<Rococo::Graphics::GlyphCallbackArgs>
			{
				int targetPos;
				GuiRect cursorRect{ 0,0,0,0 };
				GuiRect lastRect;
				virtual void OnEvent(Rococo::Graphics::GlyphCallbackArgs& args)
				{
					if (args.index == targetPos)
					{
						cursorRect = args.rect;
					}

					lastRect = args.rect;
				}
			} cb;

			cb.lastRect = { x, rect.top, x, rect.bottom };
			cb.targetPos = cursorPos;

			GuiRect clipRect = { x, rect.top, rect.right - 10, rect.bottom };

			int pos = platform.gui.CurrentKeyboardSink() == this ? cursorPos : (defaultToEnd ? len : 0);

			enum { TEXT_HEIGHT = 24 };
			Rococo::Graphics::RenderVerticalCentredTextWithCallback(rc, pos, cb, buffer, colour, TEXT_HEIGHT, { x, Centre(rect).y }, clipRect);

			if (platform.gui.CurrentKeyboardSink() == this)
			{
				if (cb.cursorRect.left == cb.cursorRect.right)
				{
					cb.cursorRect = { cb.lastRect.right,  y - 5, cb.lastRect.right + 8,  y + 5 };
				}

				cb.cursorRect.right = cb.cursorRect.left + 8;

				OS::ticks t = OS::CpuTicks();
				OS::ticks hz = OS::CpuHz();
				uint8 alpha = ((512 * t) / hz) % 255;

				if (platform.gui.IsOverwriting())
				{
					Rococo::Graphics::DrawRectangle(rc, cb.cursorRect, RGBAb(255, 255, 255, alpha >> 1), RGBAb(255, 255, 255, alpha >> 1));
				}
				else
				{
					Rococo::Graphics::DrawLine(rc, 2, BottomLeft(cb.cursorRect), BottomRight(cb.cursorRect), RGBAb(255, 255, 255, alpha));
				}
			}
		}
	};

	class BloodyFloatBinding : public IBloodyPropertyType, public IValidator
	{
		float *value;
		float minValue;
		float maxValue;
		TextEditorBox teb;
		Platform& platform;
		char buffer[12];
	public:
		BloodyFloatBinding(Platform& _platform, IEventCallback<IBloodyPropertyType>& onNotify, float* _value, float _minValue, float _maxValue) : 
			value(_value),
			minValue(_minValue),
			maxValue(_maxValue),
			platform(_platform),
			teb(_platform, *this, onNotify, buffer, 12, false, *this)
		{
			if (value == nullptr)
			{
				Throw(0, "BloodyFloatBinding: null value");
			}

			SafeFormat(buffer, 12, "%f", *_value);
		}

		cstr NotifyId() const override { return nullptr; }

		virtual bool IsLegal(char c, int charPos) const
		{
			if (c >= '0' && c <= '9')
			{
				return true;
			}

			if (charPos == 0)
			{
				if (c == '+' || c == '-')
				{
					return true;
				}
			}

			if (c == '.')
			{
				if (strstr(buffer, ".") == nullptr)
				{
					return true;
				}
			}

			return false;
		}

		virtual void OnDetached(char* buffer)
		{
			if (1 == sscanf_s(buffer, "%f", value))
			{
				if (*value < minValue) *value = minValue;
				if (*value > maxValue) *value = maxValue;
				SafeFormat(buffer, 12, "%f", *value);
			}
		}

		virtual void Free()
		{
			delete this;
		}

		virtual cstr Name() const
		{
			return "Float32";
		}

		GuiRect tebRect;

		virtual void Render(IGuiRenderContext& rc, const GuiRect& rect, RGBAb colour)
		{
			tebRect = rect;
			teb.Render(rc, tebRect, colour);
		}

		RGBAb NameColour() const override
		{
			return RGBAb(64, 0, 0, 128);
		}

		void Click(bool clickedDown, Vec2i pos) override
		{
			if (IsPointInRect(pos, tebRect))
			{
				teb.Click(clickedDown);
			}
		}
	};

	class BloodyRangeBinding : public IBloodyPropertyType 
	{
		TextEditorBox tebLeft;
		TextEditorBox tebRight;
		Platform& platform;

		struct BloodyRangeLeft: public IValidator
		{
			char buffer[12];
			float* value;
			float minValue;
			float maxValue;

			bool IsLegal(char c, int charPos) const
			{
				if (c >= '0' && c <= '9')
				{
					return true;
				}

				if (charPos == 0)
				{
					if (c == '+' || c == '-')
					{
						return true;
					}
				}

				if (c == '.')
				{
					if (strstr(buffer, ".") == nullptr)
					{
						return true;
					}
				}

				return false;
			}

			void OnDetached(char* buffer) override
			{
				if (1 == sscanf_s(buffer, "%f", value))
				{
					if (*value < minValue) *value = minValue;
					if (*value > maxValue) *value = maxValue;
					SafeFormat(buffer, 12, "%f", *value);
				}
			}
		} bloodyRangeLeft;

		struct BloodyRangeRight : public IValidator
		{
			float minValue;
			float maxValue;
			char buffer[12];
			float* value;

			bool IsLegal(char c, int charPos) const
			{
				if (c >= '0' && c <= '9')
				{
					return true;
				}

				if (charPos == 0)
				{
					if (c == '+' || c == '-')
					{
						return true;
					}
				}

				if (c == '.')
				{
					if (strstr(buffer, ".") == nullptr)
					{
						return true;
					}
				}

				return false;
			}

			void OnDetached(char* buffer) override
			{
				if (1 == sscanf_s(buffer, "%f", value))
				{
					if (*value < minValue) *value = minValue;
					if (*value > maxValue) *value = maxValue;
					SafeFormat(buffer, 12, "%f", *value);
				}
			}
		} bloodyRangeRight;
	public:
		BloodyRangeBinding(Platform& _platform, IEventCallback<IBloodyPropertyType>& onNotify, float* _leftValue, float* _rightValue, float _minValue, float _maxValue) :
			platform(_platform),
			tebLeft(_platform, *this, onNotify, bloodyRangeLeft.buffer, 12, false, bloodyRangeLeft),
			tebRight(_platform, *this, onNotify, bloodyRangeRight.buffer, 12, false, bloodyRangeRight)
		{
			bloodyRangeLeft.maxValue = bloodyRangeRight.maxValue = _maxValue;
			bloodyRangeLeft.minValue = bloodyRangeRight.minValue = _minValue;
			bloodyRangeLeft.value = _leftValue;
			bloodyRangeRight.value = _rightValue;

			if (_leftValue == nullptr || _rightValue == nullptr)
			{
				Throw(0, "BloodyRangeBinding: null value");
			}

			SafeFormat(bloodyRangeLeft.buffer, 12, "%f", *_leftValue);
			SafeFormat(bloodyRangeRight.buffer, 12, "%f", *_rightValue);
		}

		cstr NotifyId() const override { return nullptr; }

		virtual void Free()
		{
			delete this;
		}

		virtual cstr Name() const
		{
			return "Float32";
		}

		GuiRect tebRectLeft;
		GuiRect tebRectRight;

		virtual void Render(IGuiRenderContext& rc, const GuiRect& rect, RGBAb colour)
		{
			tebRectLeft = tebRectRight = rect;
			int dx = Width(rect) >> 1;
			tebRectLeft.right = tebRectLeft.left + dx - 2;
			tebRectRight.left = tebRectRight.right - dx + 2;
			tebLeft.Render(rc, tebRectLeft, colour);
			tebRight.Render(rc, tebRectRight, colour);
		}

		RGBAb NameColour() const override
		{
			return RGBAb(64, 0, 0, 128);
		}

		void Click(bool clickedDown, Vec2i pos) override
		{
			if (IsPointInRect(pos, tebRectLeft))
			{
				tebLeft.Click(clickedDown);
			}
			else if (IsPointInRect(pos, tebRectRight))
			{
				tebRight.Click(clickedDown);
			}
		}
	};

	class BloodyBoolBinding : public IBloodyPropertyType, public IKeyboardSink
	{
		bool* value;
		Platform& platform;
		IEventCallback<IBloodyPropertyType>& dirtNotifier;
	public:
		BloodyBoolBinding(Platform& _platform, IEventCallback<IBloodyPropertyType>& _dirtNotifier, bool* _value) :
			platform(_platform),
			dirtNotifier(_dirtNotifier),
			value(_value)
		{
			if (value == nullptr)
			{
				Throw(0, "BloodyBoolBinding: null value");
			}
		}

		~BloodyBoolBinding()
		{
			platform.gui.DetachKeyboardSink(this);
		}

		cstr NotifyId() const override { return nullptr; }

		virtual bool OnKeyboardEvent(const KeyboardEvent& key)
		{
			bool consumed = true;

			if (!key.IsUp())
			{
				switch (key.VKey)
				{
				case IO::VKCode_ENTER:
					platform.gui.DetachKeyboardSink(this);
					break;
				case IO::VKCode_HOME:
					*value = true;
					break;
				case IO::VKCode_END:
					*value = false;
					break;
				case IO::VKCode_LEFT:
				case IO::VKCode_RIGHT:
				case IO::VKCode_SPACEBAR:
					*value = !*value;
					break;
				}
			}

			if (consumed)
			{
				dirtNotifier.OnEvent(*this);
			}

			return true;
		}

		virtual void Free()
		{
			delete this;
		}

		virtual cstr Name() const
		{
			return "bool";
		}

		virtual void Render(IGuiRenderContext& rc, const GuiRect& rect, RGBAb colour)
		{
			if (platform.gui.CurrentKeyboardSink() == this)
			{
				Rococo::Graphics::DrawRectangle(rc, rect, RGBAb(64, 0, 0, 128), RGBAb(64, 0, 0, 255));
				Rococo::Graphics::DrawBorderAround(rc, rect, { 1,1 }, RGBAb(255, 255, 255, 255), RGBAb(224, 224, 224, 255));
			}

			char buffer[16];
			SafeFormat(buffer, 16, "%s", *value ? "true" : "false");
			Rococo::Graphics::RenderVerticalCentredText(rc, buffer, colour, 9, { rect.left + 4, Centre(rect).y }, &rect);

			int32 ds = Height(rect) - 4;
			GuiRect tickRect{ rect.right - ds, rect.top + 2, rect.right - 2, rect.bottom - 2 };
			Rococo::Graphics::DrawBorderAround(rc, tickRect, { 1,1 }, RGBAb(255, 255, 255), RGBAb(224, 224, 224));

			if (*value)
			{
				Vec2i centre = Centre(tickRect);
				Rococo::Graphics::DrawLine(rc, 2, centre + Vec2i{ -6, -4 }, centre + Vec2i{ -2,+4 }, RGBAb(255, 128, 128, 255));
				Rococo::Graphics::DrawLine(rc, 2, centre + Vec2i{ -2,4 }, centre + Vec2i{ 4, -12 }, RGBAb(255, 128, 128, 255));
			}
		}

		RGBAb NameColour() const override
		{
			return RGBAb(0, 0, 64, 128);
		}

		void Click(bool clickedDown, Vec2i pos) override
		{
			if (clickedDown)
			{
				*value = !*value;
				if (platform.gui.CurrentKeyboardSink() == this)
				{
					platform.gui.DetachKeyboardSink(this);
				}
				else
				{
					platform.gui.AttachKeyboardSink(this);
				}

				dirtNotifier.OnEvent(*this);
			}
		}
	};

	class BloodyEnumInt32Binding : public IBloodyPropertyType, public IKeyboardSink
	{
		int32* value;
		std::string name;
		std::unordered_map<std::string, int> constantsNameToValue;
		std::unordered_map<int, std::string> constantsValueToName;
		std::vector<std::string> orderedByName;
		Platform& platform;
		IEventCallback<IBloodyPropertyType>& dirtNotifier;
		HString notifyId;
	public:
		BloodyEnumInt32Binding(Platform& _platform, IEventCallback<IBloodyPropertyType>& _dirtNotifier, cstr _name, cstr _notifyId, int* _value) :
			name(_name),
			platform(_platform),
			value(_value),
			dirtNotifier(_dirtNotifier),
			notifyId(_notifyId)
		{

		}

		cstr NotifyId() const override { return notifyId; }

		~BloodyEnumInt32Binding()
		{
			platform.gui.DetachKeyboardSink(this);
		}

		virtual void AddEnumConstant(cstr key, int32 value)
		{
			constantsNameToValue[key] = value;
			constantsValueToName[value] = key;
			orderedByName.clear();
		}

		void Free() override
		{
			delete this;
		}

		cstr Name() const override
		{
			return name.c_str();
		}

		bool OnKeyboardEvent(const KeyboardEvent& key) override
		{
			if (!key.IsUp())
			{
				char c = (key.unicode > 0 && key.unicode < 128) ? key.unicode : 0;
				if (c > 0)
				{
					OnAsciiCode(c);
				}
				else
				{
					OnSysKeyEvent(key);
				}

				dirtNotifier.OnEvent(*this);
			}

			return true;
		}

		void OnSysKeyEvent(const KeyboardEvent& key)
		{
			switch (key.VKey)
			{
			case IO::VKCode_ENTER:
				platform.gui.DetachKeyboardSink(this);
				break;
			case IO::VKCode_HOME:
				if (!orderedByName.empty())
				{
					auto i = constantsNameToValue.find(orderedByName[0]);
					*value = i != constantsNameToValue.end() ? i->second : *value;
				}
				break;
			case IO::VKCode_END:
				if (!orderedByName.empty())
				{
					auto i = constantsNameToValue.find(*orderedByName.rbegin());
					*value = i != constantsNameToValue.end() ? i->second : *value;
				}
				break;
			case IO::VKCode_LEFT:
				if (!orderedByName.empty())
				{
					*value = GetLeftValue(*value);
				}
				break;
			case IO::VKCode_RIGHT:
				if (!orderedByName.empty())
				{
					*value = GetRightValue(*value);
				}
				return;
			case IO::VKCode_PGDOWN:
				{
					size_t delta = max(1ULL, orderedByName.size() / 10);
					for (size_t i = 0; i < delta; ++i)
					{
						*value = GetLeftValue(*value);
					}
				}
				break;
			case IO::VKCode_PGUP:
				{
					size_t delta = max(1ULL, orderedByName.size() / 10);
					for (size_t i = 0; i < delta; ++i)
					{
						*value = GetRightValue(*value);
					}
				}
				break;
			default:
				break;
			}
		}

		void OnAsciiCode(char c)
		{
			if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
			{
				for (size_t i = 0; i < orderedByName.size(); ++i)
				{
					char d = orderedByName[i][0];
					if ((d >= 'A' && d <= 'Z') || (d >= 'a' && d <= 'z'))
					{
						if ((c & ~32) == (d & ~32))
						{
							// case independent match
							auto k = constantsNameToValue.find(orderedByName[i]);
							*value = (k != constantsNameToValue.end()) ? k->second : *value;
							return;
						}
					}
				}
			}
			else if (c > 0)
			{
				for (size_t i = 0; i < orderedByName.size(); ++i)
				{
					char d = orderedByName[i][0];
					if (c == d)
					{
						auto k = constantsNameToValue.find(orderedByName[i]);
						*value = (k != constantsNameToValue.end()) ? k->second : *value;
						return;
					}
				}
			}
		}

		void RenderValue(IGuiRenderContext& rc, int32 v, const GuiRect& rect, RGBAb colour)
		{
			char buffer[64];
			auto i = constantsValueToName.find(v);
			if (i == constantsValueToName.end())
			{
				SafeFormat(buffer, sizeof(buffer), "uknown(%d)", v);
			}
			else
			{
				SafeFormat(buffer, sizeof(buffer), "%s", i->second);
			}

			GuiRectf textRect{ (float) rect.left, (float) rect.top, (float) rect.right, (float) rect.bottom };
			Rococo::Graphics::DrawText(rc, textRect, 0, to_fstring(buffer), 0, colour);
		}

		int GetLeftValue(int v)
		{
			if (orderedByName.empty()) return v;

			auto i = constantsValueToName.find(v);
			if (i != constantsValueToName.end())
			{
				auto& name = i->second;

				if (orderedByName[0] == name)
				{
					return v;
				}

				for (size_t j = 1; j < orderedByName.size(); ++j)
				{
					if (orderedByName[j] == name)
					{
						auto k = constantsNameToValue.find(orderedByName[j - 1]);
						if (k == constantsNameToValue.end())
						{
							break;
						}
						else
						{
							return k->second;
						}
					}
				}
			}

			return max(v - 1, 0);
		}

		int GetRightValue(int v)
		{
			if (orderedByName.empty()) return v;

			auto i = constantsValueToName.find(v);
			if (i != constantsValueToName.end())
			{
				auto& name = i->second;

				if (*orderedByName.rbegin() == name)
				{
					return v;
				}

				for (size_t j = 0; j < orderedByName.size() - 1; ++j)
				{
					if (orderedByName[j] == name)
					{
						auto k = constantsNameToValue.find(orderedByName[j + 1]);
						if (k == constantsNameToValue.end())
						{
							break;
						}
						else
						{
							return k->second;
						}
					}
				}
			}

			return  min(v + 1, 0x7FFFFFFF);
		}

		GuiRect absRect{ 0,0,0,0 };

		virtual void Render(IGuiRenderContext& rc, const GuiRect& rect, RGBAb colour)
		{
			if (orderedByName.empty())
			{
				for (auto& i : constantsNameToValue)
				{
					orderedByName.push_back(i.first.c_str());
				}

				std::sort(orderedByName.begin(), orderedByName.end());
			}

			absRect = rect;

			GuiMetrics metrics;
			rc.Renderer().GetGuiMetrics(metrics);

			if (platform.gui.CurrentKeyboardSink() == this)
			{
				Rococo::Graphics::DrawRectangle(rc, rect, RGBAb(64, 0, 0, 128), RGBAb(64, 0, 0, 255));
				Rococo::Graphics::DrawBorderAround(rc, rect, { 1,1 }, RGBAb(255, 255, 255, 255), RGBAb(224, 224, 224, 255));
			}

			int32 spinnerSpan = Height(rect);

			int32 textSpan = Width(rect) - 2 * spinnerSpan;

			int32 nVisibleValues = 3;

			const int32 cellSpan = textSpan / nVisibleValues;

			Vec2i centre = Centre(rect);

			GuiRect currentValueRect{ centre.x - (cellSpan >> 1), rect.top, centre.x + (cellSpan >> 1), rect.bottom };

			RenderValue(rc, *value, currentValueRect, colour);

			if (IsPointInRect(metrics.cursorPosition, rect))
			{
				GuiRect leftValueRect{ currentValueRect.left - cellSpan, rect.top, currentValueRect.left - 1, rect.bottom };
				GuiRect rightValueRect{ currentValueRect.right + 1, rect.top,  currentValueRect.right + cellSpan, rect.bottom };

				RGBAb dullColour(colour.red >> 1, colour.green >> 1, colour.blue >> 1);

				int leftValue = GetLeftValue(*value);
				if (leftValue != *value) RenderValue(rc, leftValue, leftValueRect, dullColour);

				int rightValue = GetRightValue(*value);
				if (rightValue != *value) RenderValue(rc, rightValue, rightValueRect, dullColour);

				int32 clickBorder = 4;
				GuiRect leftClickRect{ rect.left + clickBorder, rect.top + clickBorder, rect.left + spinnerSpan - clickBorder, rect.bottom - clickBorder };
				Rococo::Graphics::DrawTriangleFacingLeft(rc, leftClickRect, (leftValue != *value) ? RGBAb(255, 255, 128) : RGBAb(64, 64, 64));

				GuiRect rightClickRect{ rect.right - spinnerSpan + clickBorder, rect.top + clickBorder, rect.right - clickBorder, rect.bottom - clickBorder };
				Rococo::Graphics::DrawTriangleFacingRight(rc, rightClickRect, (rightValue != *value) ? RGBAb(255, 255, 128) : RGBAb(64, 64, 64));
			}
		}

		RGBAb NameColour() const override
		{
			return RGBAb(16, 64, 16, 128);
		}

		void Click(bool clickedDown, Vec2i pos) override
		{
			if (!clickedDown)
			{
				auto centre = Centre(absRect);
				if (pos.x > centre.x + 20) *value = GetRightValue(*value);
				else if (pos.x < centre.x - 20) *value = GetLeftValue(*value);

				if (platform.gui.CurrentKeyboardSink() == this)
				{
					platform.gui.DetachKeyboardSink(this);
				}
				else
				{
					platform.gui.AttachKeyboardSink(this);
				}

				dirtNotifier.OnEvent(*this);
			}
		}
	};

	struct PositiveIntegerValidator : public IValidator
	{
		bool IsLegal(char c, int ccursorPos) const override
		{
			return c >= '0' && c <= '9';
		}

		void OnDetached(char* buffer) override
		{
			if (*buffer == 0)
			{
				SafeFormat(buffer, 2, "0");
			}
		}
	};

	class BloodyIntBinding : public IBloodyPropertyType, public IValidator
	{
		int32* value;
		TextEditorBox teb;
		Platform& platform;
		char buffer[12];
		bool addHexView;
	public:
		BloodyIntBinding(Platform& _platform, IEventCallback<IBloodyPropertyType>& dirtNotifier, bool _addHexView, int* _value) :
			platform(_platform),
			teb(_platform, *this, dirtNotifier, buffer, 12, false, *this),
			addHexView(_addHexView),
			value(_value)
		{
			if (value == nullptr)
			{
				Throw(0, "BloodyIntBinding: null value");
			}

			SafeFormat(buffer, 12, "%d", *_value);
		}

		cstr NotifyId() const override { return nullptr; }

		virtual bool IsLegal(char c, int charPos) const
		{
			if (charPos == 0)
			{
				if (buffer[0] != '-' && c == '-')
				{
					return true;
				}

				if (buffer[0] == '-' && platform.gui.IsOverwriting())
				{
					return false;
				}
			}
			return c >= '0' && c <= '9';
		}

		virtual void OnDetached(char* buffer)
		{
			*value = atoi(buffer);
			SafeFormat(buffer, 10, "%d", *value);
		}

		virtual void Free()
		{
			delete this;
		}

		virtual cstr Name() const
		{
			return "Int32";
		}

		GuiRect tebRect;

		virtual void Render(IGuiRenderContext& rc, const GuiRect& rect, RGBAb colour)
		{
			tebRect = rect;
			teb.Render(rc, tebRect, colour);

			if (addHexView && platform.gui.CurrentKeyboardSink() != &teb)
			{
				char hex[12];
				SafeFormat(hex, 12, "0x%8.8X", *value);

				RGBAb dullColour(colour.red, colour.green, colour.blue, colour.alpha >> 1);
				GuiRect hexRect{ rect.left, rect.top, rect.right - 8, rect.bottom };
				Rococo::Graphics::RenderRightAlignedText(rc, hex, dullColour, 9, hexRect);
			}
		}

		RGBAb NameColour() const override
		{
			return RGBAb(48, 16, 0, 128);
		}

		void Click(bool clickedDown, Vec2i pos) override
		{
			if (IsPointInRect(pos, tebRect))
			{
				teb.Click(clickedDown);
			}
		}
	};

	class BloodyColour : public IBloodyPropertyType, public IKeyboardSink
	{
		RGBAb* value;
		std::string name;

		Platform& platform;

		PositiveIntegerValidator piv;

		char rbuffer[4] = "255";
		char gbuffer[4] = "0";
		char bbuffer[4] = "0";
		char abuffer[4] = "255";

		TextEditorBox rbox;
		TextEditorBox gbox;
		TextEditorBox bbox;
		TextEditorBox abox;

		TextEditorBox* teb[4] = { &rbox, &gbox, &bbox, &abox };
	public:
		BloodyColour(Platform& _platform, IEventCallback<IBloodyPropertyType>& dirtNotifier, cstr _name, RGBAb* _colour) :
			name(_name),
			platform(_platform),
			rbox(_platform, *this, dirtNotifier, rbuffer, 4, false, piv),
			gbox(_platform, *this, dirtNotifier, gbuffer, 4, false, piv),
			bbox(_platform, *this, dirtNotifier, bbuffer, 4, false, piv),
			abox(_platform, *this, dirtNotifier, abuffer, 4, false, piv),
			value(_colour)
		{
			CopyValueToBuffers();
		}

		void CopyValueToBuffers()
		{
			SafeFormat(rbuffer, 4, "%u", value->red);
			SafeFormat(gbuffer, 4, "%u", value->green);
			SafeFormat(bbuffer, 4, "%u", value->blue);
			SafeFormat(abuffer, 4, "%u", value->alpha);
		}

		~BloodyColour()
		{
			platform.gui.DetachKeyboardSink(this);
		}

		cstr NotifyId() const override { return nullptr; }

		void Free() override
		{
			delete this;
		}

		virtual cstr Name() const
		{
			return name.c_str();
		}

		virtual bool OnKeyboardEvent(const KeyboardEvent& key)
		{
			if (!key.IsUp())
			{
				switch (key.VKey)
				{
				case IO::VKCode_ENTER:
					return true;
				case IO::VKCode_HOME:
					return true;
				case IO::VKCode_END:
					return true;
				case IO::VKCode_LEFT:
					return true;
				case IO::VKCode_RIGHT:
					return true;
				case IO::VKCode_PGDOWN:
					return true;
				case IO::VKCode_PGUP:
					return true;
				}
			}
			return true;
		}

		GuiRect absRect{ 0,0,0,0 };

		GuiRect tebRect[4];

		void ParseValue()
		{
			int32 red = atoi(rbuffer);
			int32 green = atoi(gbuffer);
			int32 blue = atoi(bbuffer);
			int32 alpha = atoi(abuffer);

			value->red = (uint8)red;
			value->green = (uint8)green;
			value->blue = (uint8)blue;
			value->alpha = (uint8)alpha;
		}

		GuiRect buttonRect = { 0,0,0,0 };

		virtual void Render(IGuiRenderContext& rc, const GuiRect& rect, RGBAb colour)
		{
			ParseValue();

			absRect = rect;

			GuiMetrics metrics;
			rc.Renderer().GetGuiMetrics(metrics);

			int span = Height(rect);

			buttonRect = { rect.left + 2, rect.top + 2, rect.left + span - 2, rect.bottom - 2 };

			RGBAb fullColour{ value->red, value->green, value->blue, 255 };
			Rococo::Graphics::DrawRectangle(rc, buttonRect, fullColour, *value);

			if (IsPointInRect(metrics.cursorPosition, buttonRect))
			{
				Rococo::Graphics::DrawBorderAround(rc, buttonRect, { 1,1 }, RGBAb(255, 255, 255), RGBAb(224, 224, 224));
			}
			else
			{
				Rococo::Graphics::DrawBorderAround(rc, buttonRect, { 1,1 }, RGBAb(192, 192, 192), RGBAb(160, 160, 160));
			}

			int32 cellSpan = (Width(rect) - Width(buttonRect)) / 4;

			TextEditorBox* teb[] = { &rbox, &gbox, &bbox, &abox };
			RGBAb fontColours[4] = { RGBAb(255,0,0,255), RGBAb(0,255,0,255), RGBAb(0,0,255,255), RGBAb(128,128,128,255) };

			int32 x = buttonRect.right + 1;
			for (int i = 0; i < 4; ++i)
			{
				tebRect[i] = GuiRect{ x, rect.top, x + cellSpan - 2, rect.bottom };
				if (i == 3) tebRect[i].right = rect.right - 1;
				teb[i]->Render(rc, tebRect[i], fontColours[i]);

				Rococo::Graphics::DrawBorderAround(rc, tebRect[i], { 1,1 }, RGBAb(192, 192, 192, 32), RGBAb(160, 160, 160, 32));

				x += cellSpan;
			}
		}

		RGBAb NameColour() const override
		{
			return RGBAb(64, 32, 32, 128);
		}

		void Click(bool clickedDown, Vec2i pos) override
		{
			for (int i = 0; i < 4; ++i)
			{
				if (IsPointInRect(pos, tebRect[i]))
				{
					teb[i]->Click(clickedDown);
					return;
				}
			}

			if (IsPointInRect(pos, buttonRect))
			{
				RGBAb colour(value->red, value->green, value->blue, 255);
				if (OS::TryGetColourFromDialog(colour, platform.renderer.Window()))
				{
					value->red = colour.red;
					value->green = colour.green;
					value->blue = colour.blue;
					CopyValueToBuffers();
				}
			}
		}
	};

	class BloodyMaterialBinding : public IBloodyPropertyType, public IValidator
	{
		char* value;
		size_t len;
		Platform& platform;
		TextEditorBox teb;
		MaterialId& id;
		HString notifyId;
	public:
		BloodyMaterialBinding(Platform& _platform, MaterialId& _id, IEventCallback<IBloodyPropertyType>& dirtNotifier, cstr _notfiyId, char* matString, size_t _len) :
			id(_id),
			value(matString),
			len(_len),
			platform(_platform),
			teb(_platform, *this, dirtNotifier, value, _len, true, *this),
			notifyId(_notfiyId)
		{
			if (value == nullptr)
			{
				Throw(0, "BloodyMaterialBinding: null value");
			}

			SafeFormat(value, len, "%s", matString);
		}

		cstr NotifyId() const override { return notifyId; }

		bool IsLegal(char c, int charPos) const override
		{
			return true;
		}

		void OnDetached(char* buffer) override
		{
			try
			{
				id = platform.renderer.GetMaterialId(value);
			}
			catch (IException&)
			{
				id = -1;
			}

			if (*buffer == 0)
			{
				SafeFormat(buffer, len, "random");
			}
		}

		virtual void Free()
		{
			delete this;
		}

		virtual cstr Name() const
		{
			return "MaterialId";
		}

		MaterialId GetLeftValue(MaterialId v)
		{
			if (v >= 0) v -= 1;
			return v;
		}

		MaterialId GetRightValue(MaterialId v)
		{
			MaterialArrayMetrics metrics;
			platform.renderer.GetMaterialArrayMetrics(metrics);
			if (v < metrics.NumberOfElements - 1)
			{
				v += 1;
			}
			return v;
		}

		GuiRect leftClickRect;
		GuiRect rightClickRect;

		virtual void Render(IGuiRenderContext& rc, const GuiRect& rect, RGBAb colour)
		{
			int32 spinnerSpan = Height(rect);

			GuiRect editorRect{ rect.left + spinnerSpan, rect.top, rect.right - spinnerSpan, rect.bottom };

			if (Eq("random", value) && platform.gui.CurrentKeyboardSink() != &teb)
			{
				GuiRectf textRect{ (float)editorRect.left, (float)editorRect.top, (float)editorRect.right, (float)editorRect.bottom };
				Rococo::Graphics::DrawText(rc, textRect, 0, to_fstring(value), 0, RGBAb(128, 0, 0, 255));
			}
			else
			{
				teb.Render(rc, editorRect, id < 0 ? RGBAb(255, 64, 64, 255) : colour);
			}

			GuiMetrics metrics;
			rc.Renderer().GetGuiMetrics(metrics);

			MaterialId leftValue = GetLeftValue(id);
			MaterialId rightValue = GetRightValue(id);

			if (IsPointInRect(metrics.cursorPosition, rect))
			{
				RGBAb spinColour1 = IsPointInRect(metrics.cursorPosition, leftClickRect) ? RGBAb(255, 255, 128) : RGBAb(192, 192, 64);
				int32 clickBorder = 4;
				leftClickRect = GuiRect{ rect.left + clickBorder, rect.top + clickBorder, rect.left + spinnerSpan - clickBorder, rect.bottom - clickBorder };
				Rococo::Graphics::DrawTriangleFacingLeft(rc, leftClickRect, (leftValue != id) ? spinColour1 : RGBAb(64, 64, 64));

				RGBAb spinColour2 = IsPointInRect(metrics.cursorPosition, rightClickRect) ? RGBAb(255, 255, 128) : RGBAb(192, 192, 64);
				rightClickRect = GuiRect{ rect.right - spinnerSpan + clickBorder, rect.top + clickBorder, rect.right - clickBorder, rect.bottom - clickBorder };
				Rococo::Graphics::DrawTriangleFacingRight(rc, rightClickRect, (rightValue != id) ? spinColour2 : RGBAb(64, 64, 64));
			}
		}

		RGBAb NameColour() const override
		{
			return RGBAb(96, 0, 96, 128);
		}

		void Click(bool clickedDown, Vec2i pos) override
		{
			bool consumed = false;

			if (clickedDown)
			{
				if (IsPointInRect(pos, leftClickRect))
				{
					id = GetLeftValue(id);
					if (id != 0)
					{
						teb.Notify();
						SafeFormat(value, len, "random");
					}
					consumed = true;
				}
				else if (IsPointInRect(pos, rightClickRect))
				{
					id = GetRightValue(id);
					teb.Notify();
					consumed = true;
				}

				if (id == -1)
				{
					if (*value == 0)
					{
						SafeFormat(value, len, "random");
					}
				}
				
				auto mat = platform.renderer.GetMaterialTextureName(id);
				if (mat)
				{
					WideFilePath sysName;
					platform.installation.ConvertPingPathToSysPath(mat, sysName);

					try
					{
						platform.installation.ConvertSysPathToMacroPath(sysName, value, len, "#m");
					}
					catch (IException& ex)
					{
						SafeFormat(value, len, "%s", ex.Message());
					}			
				}
			}

			if (!consumed) teb.Click(clickedDown);
		}
	};


	class BloodySpacer : public IBloodyPropertyType
	{
	public:
		BloodySpacer()
		{
		}

		cstr NotifyId() const override { return nullptr; }

		virtual void Free()
		{
			delete this;
		}

		virtual cstr Name() const
		{
			return "Spacer";
		}

		virtual void Render(IGuiRenderContext& rc, const GuiRect& rect, RGBAb colour)
		{
		}

		RGBAb NameColour() const override
		{
			return RGBAb(0, 0, 0, 0);
		}

		void Click(bool clickedDown, Vec2i pos) override
		{
		}
	};


	class BloodyEventButton : public IBloodyPropertyType
	{
		IPublisher& publisher;
		std::string name;
		std::string eventName;
		EventIdRef id;
	public:
		BloodyEventButton(IPublisher& _publisher, cstr _name, cstr _eventName):
			publisher(_publisher), name(_name), eventName(_eventName)
		{
			id = { eventName.c_str(), 0 };
		}

		cstr NotifyId() const override { return nullptr; }

		virtual void Free()
		{
			delete this;
		}

		virtual cstr Name() const
		{
			return "EventButton";
		}

		virtual void Render(IGuiRenderContext& rc, const GuiRect& rect, RGBAb colour)
		{
			GuiMetrics metrics;
			rc.Renderer().GetGuiMetrics(metrics);

			GuiRectf textRect{ (float)rect.left, (float)rect.top, (float)rect.right, (float)rect.bottom };

			if (IsPointInRect(metrics.cursorPosition, rect))
			{
				Graphics::DrawRectangle(rc, rect, RGBAb(192, 192, 192, 255), RGBAb(192, 192, 192, 255));
				Rococo::Graphics::DrawText(rc, textRect, 0, to_fstring(name.c_str()), 0, RGBAb(0, 0, 0, 255));
				Graphics::DrawBorderAround(rc, rect, { 1,1 }, RGBAb(255, 255, 255, 255), RGBAb(224, 224, 224, 255));
			}
			else
			{
				Graphics::DrawRectangle(rc, rect, RGBAb(160, 160, 160, 255), RGBAb(160, 160, 160, 255));
				Rococo::Graphics::DrawText(rc, textRect, 0, to_fstring(name.c_str()), 0, RGBAb(0, 0, 0, 255));
				Graphics::DrawBorderAround(rc, rect, { 1,1 }, RGBAb(224, 224, 224, 255), RGBAb(200, 200, 200, 255));
			}
		}

		RGBAb NameColour() const override
		{
			return RGBAb(0, 0, 0, 0);
		}

		void Click(bool clickedDown, Vec2i pos) override
		{
			if (!clickedDown)
			{
				struct ANONEvent : EventArgs
				{

				} ev;
				publisher.Publish(ev, id);
			}
		}
	};

	class BloodyMessage : public IBloodyPropertyType
	{
		std::string msg;
	public:
		BloodyMessage(cstr message) : msg(message)
		{
		}

		cstr NotifyId() const override { return nullptr; }

		void Free() override
		{
			delete this;
		}

		cstr Name() const override
		{
			return "Spacer";
		}

		void Render(IGuiRenderContext& rc, const GuiRect& rect, RGBAb colour) override
		{
			GuiRectf textRect{ (float)rect.left, (float)rect.top, (float)rect.right, (float)rect.bottom };
			Rococo::Graphics::DrawText(rc, textRect, 0, to_fstring(msg.c_str()), 0, colour);
		}

		RGBAb NameColour() const override
		{
			return RGBAb(0, 0, 0, 0);
		}

		void Click(bool clickedDown, Vec2i pos) override
		{
		}
	};

	void ExpandMacros(cstr pingPath, U8FilePath& expandedPath, IInstallation& installation)
	{
		WideFilePath sysPath;
		installation.ConvertPingPathToSysPath(pingPath, sysPath);
		installation.ConvertSysPathToPingPath(sysPath, expandedPath);
	}

	void StripUntilFinalDirectory(char* pingPath)
	{
		int32 len = StringLength(pingPath);
		if (pingPath[len - 1] == '/)')
		{
			return;
		}

		for (int32 i = len - 1; i > 0; i--)
		{
			if (pingPath[i] == '/')
			{
				pingPath[i + 1] = 0;
				return;
			}
		}
	}

	class BloodyPingPathBinding : public IBloodyPropertyType, public IValidator
	{
		char* value;
		size_t len;
		Platform& platform;
		TextEditorBox teb;
		bool validated = false;
		HString root;
	public:
		BloodyPingPathBinding(Platform& _platform, IEventCallback<IBloodyPropertyType>& dirtNotifier, char* pingPath, size_t _len, cstr default) :
			platform(_platform),
			value(pingPath),
			len(_len),
			teb(_platform, *this, dirtNotifier, value, _len, true, *this)
		{
			U8FilePath expandedPath;
			ExpandMacros(default, expandedPath, platform.installation);

			StripUntilFinalDirectory(expandedPath.buf);

			root = expandedPath;

			if (value == nullptr)
			{
				Throw(0, "BloodyPingPathBinding: null value");
			}
			SafeFormat(value, _len, "%s", pingPath);
			OnDetached(value);
		}

		cstr NotifyId() const override { return nullptr; }

		bool IsLegal(char c, int cursorPos) const override
		{
			return true;
		}

		void OnDetached(char* buffer) override
		{
			WideFilePath sysPath;
			try
			{
				if (*buffer == 0)
				{
					validated = false;
					return;
				}
				platform.installation.ConvertPingPathToSysPath(buffer, sysPath);
				validated = OS::IsFileExistant(sysPath);
			}
			catch (IException&)
			{
				validated = false;
			}
		}

		void Free() override
		{
			delete this;
		}

		cstr Name() const override
		{
			return "Ping Path";
		}

		GuiRect editorRect;
		GuiRect buttonRect;

		const fstring loadImage = "!textures/toolbars/load.tif"_fstring;

		void Render(IGuiRenderContext& rc, const GuiRect& rect, RGBAb colour) override
		{
			int span = Height(rect);
			editorRect = GuiRect{ rect.left + span, rect.top, rect.right, rect.bottom };

			RGBAb colour2 = validated ? colour : RGBAb(255, 64, 64);
			teb.Render(rc, editorRect, colour2);

			buttonRect = GuiRect{ rect.left, rect.top + 2, editorRect.left - 1, rect.bottom - 2 };

			GuiMetrics metrics;
			rc.Renderer().GetGuiMetrics(metrics);

			Textures::BitmapLocation bml;
			if (platform.renderer.SpriteBuilder().TryGetBitmapLocation(loadImage, bml))
			{
				Rococo::Graphics::DrawSpriteCentred(buttonRect, bml, rc);
			}
			else
			{
				GuiRectf textRect{ (float)buttonRect.left, (float)buttonRect.top, (float)buttonRect.right, (float)buttonRect.bottom };
				Rococo::Graphics::DrawText(rc, textRect, 0, "~"_fstring, 0, RGBAb(255, 255, 255));
			}

			if (IsPointInRect(metrics.cursorPosition, buttonRect))
			{
				Rococo::Graphics::DrawBorderAround(rc, buttonRect, { 1,1 }, RGBAb(255, 255, 255), RGBAb(224, 224, 224));
			}
		}

		RGBAb NameColour() const override
		{
			return RGBAb(64, 64, 64, 128);
		}

		void OnSelected(cstr pingPath)
		{
			teb.Notify();
			platform.installation.CompressPingPath(value, len, pingPath);
			OnDetached(value);
		}

		void InvokeSelectFileUI()
		{
			using namespace Rococo::IO;

			struct BrowserRules_SelectScript : public IBrowserRules
			{
				IPublisher& publisher;
				HString lastError;
				HString prefix;
				HString initialFile;
				BloodyPingPathBinding& control;

				BrowserRules_SelectScript(IPublisher& _publisher, cstr _prefix, cstr _initalFile, BloodyPingPathBinding& _control) :
					publisher(_publisher), prefix(_prefix), initialFile(_initalFile), control(_control)
				{

				}

				void GetRoot(U32FilePath& path) const override
				{
					IO::PathFromAscii(prefix, '/',  path);
				}

				void GetInitialFilename(U32FilePath& path) const override
				{
					IO::PathFromAscii(initialFile, '/', path);
				}

				cstr GetLastError() const override
				{
					return lastError;
				}

				void GetCaption(char* caption, size_t capacity) override
				{
					SafeFormat(caption, capacity, "Select script file ...");
				}

				void Free() override
				{
					delete this;
				}

				bool Select(const U32FilePath& scriptName) override
				{
					U8FilePath pingPath;
					ToU8(scriptName, pingPath);

					if (EndsWith(pingPath, ".sxy"))
					{
						control.OnSelected(pingPath);
						return true;
					}
					else
					{
						lastError = "Filename must have extension '.sxy'";
						OS::BeepWarning();
						return false;
					}
				}
			};

			struct BrowserRules_SelectScriptFactory : public IBrowserRulesFactory
			{
				IPublisher* publisher;
				cstr prefix;
				cstr initialFilename;
				BloodyPingPathBinding* control;

				IBrowserRules* CreateRules() override
				{
					return new BrowserRules_SelectScript(*publisher, prefix, initialFilename, *control);
				}

				cstr GetPanePingPath() const
				{
					return "!scripts/panel.browser.sxy";
				}
			} forLevelFilename;
			forLevelFilename.prefix = root;
			forLevelFilename.initialFilename = value;
			forLevelFilename.publisher = &platform.publisher;
			forLevelFilename.control = this;
			platform.utilities.BrowseFiles(forLevelFilename);
		}

		void Click(bool clickedDown, Vec2i pos) override
		{
			if (IsPointInRect(pos, editorRect))
			{
				teb.Click(clickedDown);
			}
			else if (IsPointInRect(pos, buttonRect))
			{
				InvokeSelectFileUI();
			}
		}
	};

	class BloodyProperty
	{
		AutoFree<IBloodyPropertyType> prop;
		std::string name;
		int width;
		GuiRect lastRect{ 0,0,0,0 };
	public:
		BloodyProperty(IBloodyPropertyType* _prop, cstr _name, int _width = 130) :
			name(_name), prop(_prop), width(_width)
		{

		}

		int Width() const { return width; }
		cstr Name() const { return name.c_str(); }
		IBloodyPropertyType& Prop() { return *prop; }
		void SetRect(const GuiRect& rect) { lastRect = rect; }
		bool IsInRect(Vec2i p) const { return IsPointInRect(p, lastRect); }
	};

	class BloodyPropertySetEditor :
		public IBloodyPropertySetEditorSupervisor,
		public IEventCallback<IBloodyPropertyType>,
		private IEventCallback<Rococo::Events::ScrollEvent>
	{
		std::vector<BloodyProperty*> properties;
		Platform& platform;
		AutoFree<IScrollbar> vscroll;
		IEventCallback<BloodyNotifyArgs>& onDirty;

		bool ValidateUnique(cstr name) const
		{
			for (auto i : properties)
			{
				if (Eq(i->Name(), name))
				{
					return true;
				}
			}

			return false;
		}

		void ValidateNotFound(cstr name) const
		{
			if (ValidateUnique(name)) Throw(0, "Duplicate property with name %s", name);
		}

		void Add(BloodyProperty* bp)
		{
			ValidateNotFound(bp->Name());
			properties.push_back(bp);
		}

		void OnEvent(IBloodyPropertyType& p)
		{
			onDirty.OnEvent(BloodyNotifyArgs{ *this, p.Name(), p.NotifyId() });
		}
	public:
		BloodyPropertySetEditor(Platform& _platform, IEventCallback<BloodyNotifyArgs>& _onDirty) :
			platform(_platform),
			onDirty(_onDirty),
			vscroll(platform.utilities.CreateScrollbar(true))
		{
		}

		~BloodyPropertySetEditor()
		{
			Clear();
		}

		void Clear()
		{
			for (auto i : properties)
			{
				delete i;
			}
			properties.clear();
		}

		virtual void Free()
		{
			delete this;
		}

		void AddBool(cstr name, bool* value) override
		{
			Add(new BloodyProperty(new BloodyBoolBinding(platform, *this, value), name));
		}

		void AddButton(cstr name, cstr eventName) override
		{
			properties.push_back(new BloodyProperty(new BloodyEventButton(platform.publisher, name, eventName), ""));
		}

		void AddSpacer() override
		{
			properties.push_back(new BloodyProperty(new BloodySpacer(), ""));
		}

		void AddFloat(cstr name, float* value, float minValue, float maxValue) override
		{
			Add(new BloodyProperty(new BloodyFloatBinding(platform, *this, value, minValue, maxValue), name));
		}

		virtual void AddFloatRange(cstr name, float* leftValue, float* rightValue, float minValue, float maxValue) override
		{
			Add(new BloodyProperty(new BloodyRangeBinding(platform, *this, leftValue, rightValue, minValue, maxValue), name));
		}

		void AddInt(cstr name, bool addHexView, int* value) override
		{
			Add(new BloodyProperty(new BloodyIntBinding(platform, *this, addHexView, value), name));
		}

		void AddMaterialCategory(cstr name, cstr notifyId, Rococo::Graphics::MaterialCategory* cat) override
		{
			auto* b = new BloodyEnumInt32Binding(platform, *this, "MaterialCategory", notifyId, (int*)cat);
			b->AddEnumConstant("Rock", Rococo::Graphics::MaterialCategory_Rock);
			b->AddEnumConstant("Stone", Rococo::Graphics::MaterialCategory_Stone);
			b->AddEnumConstant("Marble", Rococo::Graphics::MaterialCategory_Marble);
			b->AddEnumConstant("Metal", Rococo::Graphics::MaterialCategory_Metal);
			b->AddEnumConstant("Wood", Rococo::Graphics::MaterialCategory_Wood);
			Add(new BloodyProperty(b, name));
		}

		void AddMessage(cstr text)
		{
			properties.push_back(new BloodyProperty(new BloodyMessage(text), ""));
		}

		void AddColour(cstr name, RGBAb* colour) override
		{
			auto* b = new BloodyColour(platform, *this, "Colour", colour);
			Add(new BloodyProperty(b, name));
		}

		void AddMaterialString(cstr name, MaterialId& id, cstr notifyId, char* matString, size_t len) override
		{
			auto* b = new BloodyMaterialBinding(platform, id, *this, notifyId, matString, len);
			Add(new BloodyProperty(b, name));
		}

		void AddPingPath(cstr name, char* pingPath, size_t len, cstr defaultSubDir, int32 width) override
		{
			Add(new BloodyProperty(new BloodyPingPathBinding(platform, *this, pingPath, len, defaultSubDir), name, width));
		}

		virtual bool OnKeyboardEvent(const KeyboardEvent& key)
		{
			return false;
		}

		virtual void OnRawMouseEvent(const MouseEvent& ev)
		{
		}

		virtual void OnMouseMove(Vec2i cursorPos, Vec2i delta, int dWheel)
		{

		}

		virtual void OnMouseLClick(Vec2i cursorPos, bool clickedDown)
		{
			for (auto i : properties)
			{
				if (i->IsInRect(cursorPos))
				{
					i->Prop().Click(clickedDown, cursorPos);
					return;
				}
			}
		}

		virtual void OnMouseRClick(Vec2i cursorPos, bool clickedDown)
		{

		}

		virtual void OnEvent(Rococo::Events::ScrollEvent& se)
		{

		}

		virtual void Render(IGuiRenderContext& rc, const GuiRect& absRect)
		{
			GuiRect scrollRect{ absRect.right - 20, absRect.top, absRect.right, absRect.bottom };

			Modality modality;
			modality.isModal = true;
			modality.isTop = true;
			modality.isUnderModal = false;
			vscroll->Render(rc, scrollRect, modality, RGBAb(48, 48, 48, 240), RGBAb(32, 32, 32, 240), RGBAb(255, 255, 255), RGBAb(192, 192, 192), *this, ""_event);

			GuiRect mainRect{ absRect.left, absRect.top, scrollRect.left - 1, absRect.bottom };

			GuiMetrics metrics;
			rc.Renderer().GetGuiMetrics(metrics);
			if (IsPointInRect(metrics.cursorPosition, mainRect))
			{
				Graphics::DrawRectangle(rc, mainRect, RGBAb(0, 0, 24, 255), RGBAb(0, 0, 24, 192));
			}
			else
			{
				Graphics::DrawRectangle(rc, mainRect, RGBAb(0, 0, 0, 255), RGBAb(0, 0, 0, 192));
			}

			int y = absRect.top + 2;

			int odd = true;

			for (auto p : properties)
			{
				enum { PROPERTY_RECT_HEIGHT = 28, PROPERTY_TEXT_HEIGHT = 24 };
				int y1 = y + PROPERTY_RECT_HEIGHT;
				GuiRect rowRect{ absRect.left + 2, y, mainRect.right - 2, y1 };
				RGBAb edge1 = IsPointInRect(metrics.cursorPosition, rowRect) ? RGBAb(255, 255, 255) : RGBAb(64, 64, 64, 64);
				RGBAb edge2 = IsPointInRect(metrics.cursorPosition, rowRect) ? RGBAb(224, 224, 224) : RGBAb(32, 32, 32, 64);

				RGBAb fontColour = IsPointInRect(metrics.cursorPosition, rowRect) ? RGBAb(255, 255, 255) : RGBAb(224, 224, 224, 224);

				if (*p->Name())
				{
					GuiRect nameRect{ rowRect.left, y, rowRect.left + p->Width(), y1 };
					Graphics::DrawRectangle(rc, nameRect, p->Prop().NameColour(), p->Prop().NameColour());
					Graphics::RenderVerticalCentredText(rc, p->Name(), fontColour, PROPERTY_TEXT_HEIGHT, { rowRect.left + 4, Centre(rowRect).y }, &nameRect);

					GuiRect valueRect{ nameRect.right + 1, y, rowRect.right, y1 };
					p->Prop().Render(rc, valueRect, fontColour);
					p->SetRect(valueRect);
					Graphics::DrawBorderAround(rc, valueRect, { 1,1 }, edge1, edge2);
				}
				else
				{
					p->SetRect(rowRect);
					p->Prop().Render(rc, rowRect, RGBAb(255, 255, 255));
				}

				y = y1 + 3;
				odd = !odd;
			}
		}
	};
}

namespace Rococo
{
	IBloodyPropertySetEditorSupervisor* CreateBloodyPropertySetEditor(Platform& _platform, IEventCallback<BloodyNotifyArgs>& _onDirty)
	{
		return new ANON::BloodyPropertySetEditor(_platform, _onDirty);
	}
}