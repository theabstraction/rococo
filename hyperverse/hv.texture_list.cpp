#include "hv.h"
#include "hv.events.h"
#include <rococo.ui.h>
#include <rococo.strings.h>

#include <string>

namespace
{
	using namespace HV;

	EventIdRef evScrollChanged = "editor.tools.vscroll_ui"_event;
	EventIdRef evScrollSet = "editor.tools.vscroll_set"_event;
	EventIdRef evScrollGet = "editor.tools.vscroll_get"_event;
	EventIdRef evScrollSendKey = "editor.tools.vscroll_sendkey"_event;
	EventIdRef evScrollSendMouse = "editor.tools.vscroll_sendmouse"_event;

	auto evEditorToolsVScrollSet = "editor.tools.vscroll_set"_event;

	class TextureList : public ITextureList, public IUIElement, public IObserver
	{
		Platform& platform;
		int32 scrollPosition = 0;
		Vec2i absTopLeft = { 0,0 };
		std::string target;
	public:
		TextureList(Platform& _platform) : platform(_platform)
		{
			platform.gui.RegisterPopulator("editor.tools.imagelist", this);
			platform.publisher.Subscribe(this, evScrollChanged);
			platform.publisher.Subscribe(this, evScrollGet);
		}

		~TextureList()
		{
			platform.publisher.Unsubscribe(this);
			platform.gui.UnregisterPopulator(this);
		}

		void Free() override
		{
			delete this;
		}

		int32 GetIndexOf(cstr name)
		{
			return (int32)platform.renderer.GetMaterialId(name);
		}

		cstr GetNeighbour(cstr name, bool forward)
		{
			int32 i = GetIndexOf(name);

			MaterialArrayMetrics mam;
			platform.renderer.GetMaterialArrayMetrics(mam);

			if (forward) i++;
			else i--;

			if (i < 0) i = 0;
			if (i >= mam.NumberOfElements) i = mam.NumberOfElements - 1;

			return platform.renderer.GetMaterialTextureName((float)i);
		}

		void ScrollTo(cstr filename) override
		{
			int index = -1;

			if (index < 0) return;

			scrollPosition = index * lastDy;

			ScrollEvent se;
			se.fromScrollbar = false;
			se.logicalMaxValue = 0 * lastDy;

			scrollPosition = min(scrollPosition, se.logicalMaxValue - lastPageSize);

			se.logicalMinValue = 0;
			se.logicalPageSize = lastPageSize;
			se.rowSize = lastDy / 4;
			se.logicalValue = scrollPosition;

			platform.publisher.Publish(se, evScrollSet);
		}

		cstr GetSelectedTexture() const
		{
			return "none";
		}

		bool OnKeyboardEvent(const KeyboardEvent& key) override
		{
			RouteKeyboardEvent rk;
			rk.ke = &key;
			rk.consume = false;
			platform.publisher.Publish(rk, evScrollSendKey);
			return rk.consume;
		}

		void OnRawMouseEvent(const MouseEvent& me) override
		{
			if (me.HasFlag(MouseEvent::LDown) || me.HasFlag(MouseEvent::LUp))
			{
				// We handle Lbutton ourselves as 'left-click to select'
				return;
			}
			RouteMouseEvent rm;
			rm.me = &me;
			rm.absTopleft = absTopLeft;
			platform.publisher.Publish(rm, evScrollSendMouse);
		}

		void OnMouseMove(Vec2i cursorPos, Vec2i delta, int dWheel)  override
		{
		}

		void OnMouseLClick(Vec2i cursorPos, bool clickedDown)  override
		{
			if (clickedDown)
			{
				struct : IEventCallback<ImageCallbackArgs>
				{
					Vec2i cursorPos;
					std::string target;
					virtual void OnEvent(ImageCallbackArgs& args)
					{
						GuiRect itemRect{ (int32)args.target.left, (int32)args.target.top, (int32)args.target.right, (int32)args.target.bottom };
						if (IsPointInRect(cursorPos, itemRect))
						{
							target = args.filename;
						}
					}
				} selectItem;

				selectItem.cursorPos = cursorPos;
				selectItem.target = "none";

				EnumerateVisibleImages(selectItem, lastRect);

				target = selectItem.target;

				HV::Events::ChangeDefaultTextureEvent ev;
				ev.wallName = selectItem.target.c_str();
				platform.publisher.Publish(ev, HV::Events::evChangeDefaultTextureId);
			}
		}

		void OnMouseRClick(Vec2i cursorPos, bool clickedDown)  override
		{
		}

		int32 lastDy = 0;
		int32 lastPageSize = 0;
		GuiRect lastRect = { 0,0,0,0 };

		void OnEvent(Event& ev) override
		{
			if (ev == evScrollChanged)
			{
				auto& se = As<ScrollEvent>(ev);
				if (!se.fromScrollbar)
				{
					se.fromScrollbar = false;
					se.logicalMaxValue = 0 * lastDy;
					se.logicalMinValue = 0;
					se.logicalPageSize = lastPageSize;
					se.rowSize = lastDy / 4;
					se.logicalValue = scrollPosition;
				}
				else
				{
					scrollPosition = se.logicalValue;
				}
			}
		}

		struct ImageCallbackArgs
		{
			GuiRectf target;
			cstr filename;
			float txUVtop;
			float txUVbottom;
			MaterialId matid;
		};

		void EnumerateVisibleImages(IEventCallback<ImageCallbackArgs>& cb, const GuiRect& absRect)
		{
			float x0 = absRect.left + 1.0f;
			float x1 = absRect.right - 1.0f;

			int32 width = Width(absRect) - 2;

			float y = absRect.top + 1.0f - scrollPosition;
			float dy = (float)width;

			lastPageSize = (int32)Height(absRect) - 2;

			MaterialArrayMetrics metrics;
			platform.renderer.GetMaterialArrayMetrics(metrics);

			for (MaterialId i = 0; i < metrics.NumberOfElements; i = i + 1)
			{
				float y1 = y + dy;

				float h = 1.0f;

				float b = y1;

				if (y1 >= absRect.bottom)
				{
					h = (absRect.bottom - y) / dy;
					b = (float)absRect.bottom - 1;
				}

				if (y1 > absRect.top)
				{
					float g = 0;
					float t = y;

					if (y <= absRect.top)
					{
						g = (absRect.top + 1 - y) / dy;
						t = (float)(absRect.top + 1);
					}

					ImageCallbackArgs args;
					args.filename = platform.renderer.GetMaterialTextureName(i);
					args.target = GuiRectf{ x0, t, x1, b };
					args.matid = i;
					args.txUVbottom = h;
					args.txUVtop = g;

					cb.OnEvent(args);
				}

				if (h < 1.0f)
				{
					break;
				}

				y = y1;
			}
		}

		void Render(IGuiRenderContext& grc, const GuiRect& absRect) override
		{
			int32 width = Width(absRect);

			absTopLeft = TopLeft(absRect);

			if (lastDy == 0 || lastDy != width)
			{
				lastDy = width;

				MaterialArrayMetrics mam;
				grc.Renderer().GetMaterialArrayMetrics(mam);

				int maxValue = mam.NumberOfElements * lastDy;
				lastPageSize = min(Height(absRect), maxValue);

				ScrollEvent se;
				se.fromScrollbar = false;
				se.logicalMaxValue = maxValue;
				se.logicalMinValue = 0;
				se.logicalPageSize = lastPageSize;
				se.rowSize = lastDy / 4;
				se.logicalValue = scrollPosition;
				platform.publisher.Publish(se, evEditorToolsVScrollSet);
			}

			lastRect = absRect;

			struct : IEventCallback<ImageCallbackArgs>
			{
				IGuiRenderContext* grc;
				cstr target;

				virtual void OnEvent(ImageCallbackArgs& args)
				{
					float x0 = args.target.left;
					float x1 = args.target.right;
					float t = args.target.top;
					float b = args.target.bottom;

					float g = args.txUVtop;
					float h = args.txUVbottom;

					SpriteVertexData material{ 0, 0, args.matid, 1 };
					RGBAb unused(0, 0, 0);
					GuiVertex v[6] =
					{
					   { {x0, t}, {{0, g}, 0}, material, unused },
					   { {x1, t}, {{1, g}, 0}, material, unused },
					   { {x1, b}, {{1, h}, 0}, material, unused },
					   { {x0, t}, {{0, g}, 0}, material, unused },
					   { {x0, b}, {{0, h}, 0}, material, unused },
					   { {x1, b}, {{1, h}, 0}, material, unused }
					};

					grc->AddTriangle(v);
					grc->AddTriangle(v + 3);

					if (target && Eq(args.filename, target))
					{
						GuiRect hilight{ (int32)args.target.left + 1, (int32)args.target.top + 1, (int32)args.target.right - 1, (int32)args.target.bottom - 1 };
						Rococo::Graphics::DrawBorderAround(*grc, hilight, Vec2i{ 2,2 }, RGBAb(224, 224, 224), RGBAb(255, 255, 255));
					}
				}
			} renderImages;

			renderImages.grc = &grc;
			renderImages.target = target.empty() ? nullptr : target.c_str();

			EnumerateVisibleImages(renderImages, absRect);
		}
	};
}

namespace HV
{
	ITextureList* CreateTextureList(Platform& platform)
	{
		return new TextureList(platform);
	}
}