#include "mplat.panel.base.h"
#include <rococo.strings.h>
#include <rococo.ui.h>
#include <rococo.maths.h>
#include <rococo.sexy.api.h>

using namespace Rococo;
using namespace Rococo::Events;
using namespace Rococo::MPlatImpl;

static auto evFileUpdated = "OnFileUpdated"_event;

void BasePane::SetParent(IPaneSupervisor* panel)
{
	parent = panel;
}

IPaneSupervisor* BasePane::Parent()
{
	return parent;
}

void BasePane::SetBkImage(const fstring& pingPath)
{
	bkImageName = pingPath;
}

void BasePane::SetCommand(int32 stateIndex, boolean32 deferAction, const fstring& text)
{
	if (stateIndex < 0 || stateIndex > 4)
	{
		Throw(0, "BasePane::SetCommand: stateIndex %d out of bounds [0,4]", stateIndex);
	}

	if (stateIndex >= commands.size())
	{
		commands.resize(stateIndex + 1);
	}

	commands[stateIndex] = { std::string(text), deferAction };
}

void BasePane::Invoke(IPublisher& publisher, int32 stateIndex)
{
	if (stateIndex >= 0 && stateIndex < (int32)commands.size())
	{
		auto& c = commands[stateIndex];

		if (!c.command.empty())
		{
			UIInvoke invoke;
			SecureFormat(invoke.command, sizeof(invoke.command), "%s", c.command.c_str());

			if (c.defer)
			{
				publisher.Post(invoke, evUIInvoke, true);
			}
			else
			{
				publisher.Publish(invoke, evUIInvoke);
			}
		}
	}
}

void BasePane::SetPopulator(int32 stateIndex, const fstring& populatorName)
{
	if (stateIndex < 0 || stateIndex > 4)
	{
		Throw(0, "BasePane::SetCommand: stateIndex %d out of bounds [0,4]", stateIndex);
	}

	if (stateIndex >= populators.size())
	{
		populators.resize(stateIndex + 1);
	}

	if (populatorName.length >= 192)
	{
		Throw(0, "BasePane::SetPopulator(...): Maximum length for populator name is 192 chars");
	}

	populators[stateIndex] = { std::string(populatorName) };
}

void BasePane::AppendEventToChildren(IPublisher& publisher, const MouseEvent& me, const Vec2i& absTopLeft, int stateIndex)
{
	int32 hitCount = 0;

	for (auto i : children)
	{
		auto& rect = i->ClientRect();
		auto topLeft = TopLeft(i->ClientRect()) + absTopLeft;
		auto span = Span(rect);
		GuiRect childRect{ topLeft.x, topLeft.y, topLeft.x + span.x, topLeft.y + span.y };
		if (IsPointInRect(me.cursorPos, childRect))
		{
			i->AppendEvent(me, topLeft);
			hitCount++;
		}
	}

	if (hitCount == 0 && stateIndex >= 0 && stateIndex < (int32)populators.size())
	{
		if (!populators[stateIndex].name.empty())
		{
			UIPopulate populate;
			populate.renderElement = nullptr;
			populate.name = populators[stateIndex].name.c_str();
			publisher.Publish(populate, evUIPopulate);

			if (populate.renderElement)
			{
				populate.renderElement->OnRawMouseEvent(me);

				if (me.HasFlag(MouseEvent::LUp) || me.HasFlag(MouseEvent::LDown))
				{
					populate.renderElement->OnMouseLClick(me.cursorPos, me.HasFlag(MouseEvent::LDown));
				}
				else if (me.HasFlag(MouseEvent::RUp) || me.HasFlag(MouseEvent::RDown))
				{
					populate.renderElement->OnMouseRClick(me.cursorPos, me.HasFlag(MouseEvent::RDown));
				}
				else
				{
					int dz = ((int32)(short)me.buttonData) / 120;
					populate.renderElement->OnMouseMove(me.cursorPos, { me.dx, me.dy }, dz);
				}
			}
		}
	}
}

bool BasePane::AppendEventToChildren(IPublisher& publisher, const KeyboardEvent& ke, const Vec2i& focusPoint, const Vec2i& absTopLeft, int stateIndex)
{
	for (auto i : children)
	{
		auto& rect = i->ClientRect();
		auto topLeft = TopLeft(i->ClientRect()) + absTopLeft;
		auto span = Span(rect);
		GuiRect childRect{ topLeft.x, topLeft.y, topLeft.x + span.x, topLeft.y + span.y };
		if (IsPointInRect(focusPoint, childRect))
		{
			if (i->AppendEvent(ke, focusPoint, topLeft))
			{
				return true;
			}
		}
	}

	if (stateIndex >= 0 && stateIndex < (int32)populators.size())
	{
		if (!populators[stateIndex].name.empty())
		{
			UIPopulate populate;
			populate.renderElement = nullptr;
			populate.name = populators[stateIndex].name.c_str();
			publisher.Publish(populate, evUIPopulate);

			if (populate.renderElement)
			{
				if (populate.renderElement->OnKeyboardEvent(ke))
				{
					return true;
				}
			}
		}
	}

	return false;
}

void BasePane::AlignLeftEdges(int32 borderPixels, boolean32 preserveSpan)
{
	for (auto i : children)
	{
		GuiRect childRect = i->ClientRect();
		if (preserveSpan)
		{
			Vec2i span = Span(childRect);
			childRect.right = borderPixels + span.x;
		}

		childRect.left = borderPixels;
		i->SetRect(childRect);
	}
}

void BasePane::AlignRightEdges(int32 borderPixels, boolean32 preserveSpan)
{
	int x0 = (rect.right - rect.left) - borderPixels;
	for (auto i : children)
	{
		GuiRect childRect = i->ClientRect();
		if (preserveSpan)
		{
			Vec2i span = Span(childRect);
			childRect.left = borderPixels - span.x;
		}

		childRect.right = x0;
		i->SetRect(childRect);
	}
}

void BasePane::AlignTopEdges(int32 borderPixels, boolean32 preserveSpan)
{
	int y0 = borderPixels;

	for (auto i : children)
	{
		GuiRect childRect = i->ClientRect();

		if (preserveSpan)
		{
			Vec2i span = Span(childRect);
			childRect.bottom = y0 + span.y;
		}

		childRect.top = y0;

		i->SetRect(childRect);
	}
}

void BasePane::AlignBottomEdges(int32 borderPixels, boolean32 preserveSpan)
{
	int y0 = rect.bottom - borderPixels;

	for (auto i : children)
	{
		GuiRect childRect = i->ClientRect();

		if (preserveSpan)
		{
			Vec2i span = Span(childRect);
			childRect.top = max(rect.top, y0 - span.y);
		}

		childRect.bottom = y0;

		i->SetRect(childRect);
	}
}

void BasePane::LayoutVertically(int32 vertBorder, int32 vertSpacing)
{
	if (children.empty()) return;

	if (vertSpacing < 0)
	{
		// Vertically centred

		int dy = 0;
		for (auto i : children)
		{
			GuiRect rect = i->ClientRect();
			Vec2i span = Span(rect);
			dy += span.y;
		}

		int containerHeight = rect.bottom - rect.top - 2 * vertBorder;
		int freeSpace = containerHeight - dy;
		vertSpacing = freeSpace / (int32)children.size();
	}

	int y = vertBorder;

	if (vertBorder < 0)
	{
		int dy = 0;
		for (auto i : children)
		{
			GuiRect rect = i->ClientRect();
			Vec2i span = Span(rect);
			dy += span.y + vertSpacing;
		}

		y = rect.bottom + vertBorder - dy;
	}

	for (auto i : children)
	{
		GuiRect rect = i->ClientRect();
		Vec2i span = Span(rect);
		rect.top = y;
		rect.bottom = y + span.y;
		i->SetRect(rect);

		y = rect.bottom + vertSpacing;
	}
}

void BasePane::SetRect(const GuiRect& rect)
{
	this->rect = rect;
}

const GuiRect& BasePane::ClientRect() const
{
	return rect;
}

void BasePane::GetRect(GuiRect& outRect)
{
	outRect = rect;
}

boolean32 BasePane::IsVisible()
{
	return isVisible;
}

boolean32 BasePane::IsNormalized()
{
	return rect.right > rect.left && rect.bottom > rect.top;
}

void BasePane::SetVisible(boolean32 visible)
{
	this->isVisible = isVisible;
}

IPaneSupervisor* BasePane::operator[](int index)
{
	return GetChild(index);
}

IPaneSupervisor* BasePane::GetChild(int index)
{
	if (index < 0 || index >= children.size())
	{
		Throw(0, "BasePane[index] -> index %d was out of bounds. Child count: %d", index, children.size());
	}

	return children[index];
}

int BasePane::Children() const
{
	return (int)children.size();
}

void BasePane::AddChild(IPaneSupervisor* child)
{
	children.push_back(child);
	child->SetParent(this);
}

void BasePane::RemoveChild(IPaneSupervisor* child)
{
	auto i = std::remove(children.begin(), children.end(), child);
	children.erase(i, children.end());
}

void BasePane::FreeAllChildren()
{
	for (auto i : children)
	{
		i->FreeAllChildren();
		i->Free();
	}
	children.clear();
}

void BasePane::SetScheme(const ColourScheme& scheme)
{
	this->scheme = scheme;
}

const ColourScheme& BasePane::Scheme() const
{
	return scheme;
}

void BasePane::SetColourBk1(RGBAb normal, RGBAb hilight)
{
	scheme.topLeft = normal;
	scheme.hi_topLeft = hilight;
}

void BasePane::SetColourBk2(RGBAb normal, RGBAb hilight)
{
	scheme.bottomRight = normal;
	scheme.hi_bottomRight = hilight;
}

void BasePane::SetColourEdge1(RGBAb normal, RGBAb hilight)
{
	scheme.topLeftEdge = normal;
	scheme.hi_topLeftEdge = hilight;
}

void BasePane::SetColourEdge2(RGBAb normal, RGBAb hilight)
{
	scheme.bottomRightEdge = normal;
	scheme.hi_bottomRightEdge = hilight;
}

void BasePane::SetColourFont(RGBAb normal, RGBAb hilight)
{
	scheme.fontColour = normal;
	scheme.hi_fontColour = hilight;
}

void BasePane::Populate(IPublisher& publisher, IGuiRenderContext& grc, int32 stateIndex, const Vec2i& topLeft)
{
	if (stateIndex >= 0 && stateIndex < (int32)populators.size())
	{
		UIPopulate populate;
		populate.renderElement = nullptr;
		populate.name = populators[stateIndex].name.c_str();
		publisher.Publish(populate, evUIPopulate);

		if (populate.renderElement)
		{
			GuiRect absRect = GuiRect{ 0, 0, Width(rect), Height(rect) } + topLeft;
			populate.renderElement->Render(grc, absRect);
		}
	}
}

void BasePane::RenderBkImage(IGuiRenderContext& grc, const Vec2i& topLeft, const Modality& modality)
{
	if (bkImageName.length() > 0)
	{
		if (bkBitmap.txUV.left == bkBitmap.txUV.right)
		{
			if (!grc.Renderer().SpriteBuilder().TryGetBitmapLocation(bkImageName, bkBitmap))
			{
				Throw(0, "%s: Cannot find image %s", __FUNCTION__, bkImageName.c_str());
			}
		}
	}
	else
	{
		return;
	}

	GuiRect absRect{ topLeft.x, topLeft.y, topLeft.x + rect.right - rect.left, topLeft.y + rect.bottom - rect.top };

	if (bkBitmap.txUV.left != bkBitmap.txUV.right)
	{
		Graphics::StretchBitmap(grc, bkBitmap, absRect);
	}
}

void BasePane::RenderBackground(IGuiRenderContext& grc, const Vec2i& topLeft, const Modality& modality)
{
	GuiMetrics metrics;
	grc.Renderer().GetGuiMetrics(metrics);

	auto p = metrics.cursorPosition;

	GuiRect absRect{ topLeft.x, topLeft.y, topLeft.x + rect.right - rect.left, topLeft.y + rect.bottom - rect.top };

	if (!modality.isUnderModal && IsPointInRect(p, absRect))
	{
		Graphics::DrawRectangle(grc, absRect, scheme.hi_topLeft, scheme.hi_bottomRight);
		RenderBkImage(grc, topLeft, modality);
		Graphics::DrawBorderAround(grc, absRect, Vec2i{ 1, 1 }, scheme.hi_topLeftEdge, scheme.hi_bottomRightEdge);
	}
	else
	{
		Graphics::DrawRectangle(grc, absRect, scheme.topLeft, scheme.bottomRight);
		RenderBkImage(grc, topLeft, modality);
		Graphics::DrawBorderAround(grc, absRect, Vec2i{ 1, 1 }, scheme.topLeftEdge, scheme.bottomRightEdge);
	}
}

void BasePane::RenderChildren(IGuiRenderContext& grc, const Vec2i& topLeft, const Modality& modality)
{
	auto& currentRect = ClientRect();
	if (IsVisible())
	{
		if (IsNormalized()) RenderBackground(grc, topLeft, modality);

		for (auto i : children)
		{
			auto childRect = i->ClientRect() + topLeft;
			i->Render(grc, { childRect.left, childRect.top }, modality);
		}
	}
}

namespace Rococo
{
	namespace MPlatImpl
	{
		void RenderLabel(IGuiRenderContext& grc, cstr text, const GuiRect& absRect, int horzAlign, int vertAlign, Vec2i padding, int fontIndex, const ColourScheme& scheme, bool enableHighlights)
		{
			GuiMetrics metrics;
			grc.Renderer().GetGuiMetrics(metrics);

			int fontHeight = Height(absRect) - 4;

			using namespace Rococo::Fonts;
			struct : IDrawTextJob
			{
				cstr text;
				int fontIndex;
				RGBAb colour;
				int fontHeight;

				virtual void OnDraw(IGlyphBuilder& builder)
				{
					builder.SetTextColour((FontColour&)colour);
					builder.SetFontIndex(fontIndex);
					builder.SetFontHeight(fontHeight);

					for (cstr p = text; *p != 0; p++)
					{
						char c = *p;
						GuiRectf outputRect;
						builder.AppendChar(c, outputRect);
					}
				}
			} job;

			job.text = text;
			job.fontIndex = fontIndex;
			job.fontHeight = fontHeight;
			job.colour = enableHighlights && IsPointInRect(metrics.cursorPosition, absRect) ? scheme.hi_fontColour : scheme.fontColour;

			Vec2i span = grc.EvalSpan({ 0,0 }, job, nullptr);

			Vec2i pos;

			if (horzAlign < 0)
			{
				pos.x = absRect.left + padding.x;
			}
			else if (horzAlign == 0)
			{
				pos.x = Centre(absRect).x - (span.x >> 1);
			}
			else
			{
				pos.x = absRect.right - span.x - padding.x;
			}

			if (vertAlign < 0)
			{
				pos.y = absRect.top + padding.y;
			}
			else if (vertAlign == 0)
			{
				pos.y = Centre(absRect).y - (span.y >> 1);
			}
			else
			{
				pos.y = absRect.bottom - span.y - padding.y;
			}

			GuiRect clipRect = absRect;
			clipRect.left += (padding.x - 1);
			clipRect.right -= (padding.x - 1);
			clipRect.top += (padding.y - 1);
			clipRect.bottom -= (padding.y + 1);

			grc.RenderText(pos, job, &clipRect);
		}
	} // MPlatImpl
} // Rococo

struct PaneContainer : public BasePane, virtual public IPaneContainer
{
	Platform& platform;

	PaneContainer(Platform& _platform) : platform(_platform)
	{

	}

	Rococo::IPaneContainer* AddContainer(const GuiRect& rect)
	{
		auto* container = new PaneContainer(platform);
		AddChild(container);
		container->SetRect(rect);
		return container;
	}

	Rococo::IArrayBox* /* box */ AddArrayBox(int32 fontIndex, const fstring& populatorEvent, const GuiRect& rect)
	{
		auto* arrayBox = Rococo::MPlatImpl::AddArrayBox(platform, *this, fontIndex, populatorEvent, rect);
		return arrayBox;
	}

	Rococo::IEnumListPane* AddEnumList(int32 fontIndex, const fstring& populateEvent, const GuiRect& rect) override
	{
		auto* enumList = Rococo::MPlatImpl::AddEnumList(platform, *this, fontIndex, populateEvent, rect);
		return enumList;
	}

	Rococo::ILabelPane* AddLabel(int32 fontIndex, const fstring& text, const GuiRect& rect)
	{
		auto* label = Rococo::MPlatImpl::AddLabel(platform.publisher, *this, fontIndex, text, rect);
		return label;
	}

	Rococo::ISlider* AddSlider(int32 fontIndex, const fstring& text, const GuiRect& rect, float minValue, float maxValue)
	{
		auto* s = Rococo::MPlatImpl::AddSlider(platform.publisher, platform.renderer, *this, fontIndex, text, rect, minValue, maxValue);
		return s;
	}

	Rococo::ITabContainer* AddTabContainer(int32 tabHeight, int32 fontIndex, const GuiRect& rect)
	{
		auto* tabs = Rococo::MPlatImpl::AddTabContainer(platform.publisher, (IKeyboardSupervisor&) platform.keyboard, *this, tabHeight, fontIndex, rect);
		return tabs;
	}

	Rococo::IFramePane* AddFrame(const GuiRect& rect);

	Rococo::ITextOutputPane* AddTextOutput(int32 fontIndex, const fstring& eventKey, const GuiRect& rect)
	{
		auto* to = Rococo::MPlatImpl::AddTextOutput(platform.publisher, *this, fontIndex, eventKey, rect);
		to->SetRect(rect);
		return to;
	}

	Rococo::IRadioButton* AddRadioButton(int32 fontIndex, const fstring& text, const fstring& key, const fstring& value, const GuiRect& rect)
	{
		auto* radio = Rococo::MPlatImpl::AddRadioButton(platform.publisher, *this, fontIndex, text, key, value, rect);
		return radio;
	}

	Rococo::IScroller* AddScroller(const fstring& key, const GuiRect& rect, boolean32 isVertical)
	{
		auto* scroller = Rococo::MPlatImpl::AddScroller(platform.publisher, *this, key, rect, isVertical);
		scroller->SetRect(rect);
		return scroller;
	}

	Rococo::IContextMenuPane* AddContextMenu(const fstring& key, const GuiRect& rect)
	{
		auto* menu = Rococo::MPlatImpl::AddContextMenuPane(platform.publisher, platform.keyboard, *this, key, rect, platform.utilities.GetContextMenu());
		return menu;
	}

	void Free() override
	{
		FreeAllChildren();
		delete this;
	}

	bool AppendEvent(const KeyboardEvent& ke, const Vec2i& focusPoint, const Vec2i& absTopLeft)
	{
		return AppendEventToChildren(platform.publisher, ke, focusPoint, absTopLeft, 0);
	}

	void AppendEvent(const MouseEvent& me, const Vec2i& absTopLeft)
	{
		AppendEventToChildren(platform.publisher, me, absTopLeft, 0);
	}

	void Render(IGuiRenderContext& grc, const Vec2i& topLeft, const Modality& modality) override
	{
		Populate(platform.publisher, grc, 0, topLeft);
		RenderChildren(grc, topLeft, modality);
	}
};

class ScriptedPanel : IEventCallback<ScriptCompileArgs>, IObserver, public IPaneBuilderSupervisor, public PaneContainer
{
	GuiRect lastRect{ 0, 0, 0, 0 };
	std::string scriptFilename;
	Platform& platform;
	IEventCallback<ScriptCompileArgs>* onCompile;
public:
	ScriptedPanel(Platform& _platform, cstr _scriptFilename, IEventCallback<ScriptCompileArgs>* _onCompile) : PaneContainer(_platform),
		platform(_platform),
		scriptFilename(_scriptFilename),
		onCompile(_onCompile)
	{
		platform.publisher.Subscribe(this, evFileUpdated);
	}

	~ScriptedPanel()
	{
		platform.publisher.Unsubscribe(this);
	}

	void OnEvent(Event& ev) override
	{
		if (evFileUpdated == ev)
		{
			auto& fue = As<FileUpdatedEvent>(ev);
			if (Rococo::Eq(fue.pingPath, scriptFilename.c_str()))
			{
				RefreshScript();
			}
		}
	}

	IPaneContainer* Root() override
	{
		return this;
	}

	void Free() override
	{
		delete this;
	}

	void OnEvent(ScriptCompileArgs& args) override
	{
		AddNativeCalls_RococoITabContainer(args.ss, nullptr);
		AddNativeCalls_RococoIFramePane(args.ss, nullptr);
		AddNativeCalls_RococoIPaneContainer(args.ss, nullptr);
		AddNativeCalls_RococoILabelPane(args.ss, nullptr);
		AddNativeCalls_RococoIArrayBox(args.ss, nullptr);
		AddNativeCalls_RococoIPaneBuilder(args.ss, this);
		AddNativeCalls_RococoITextOutputPane(args.ss, nullptr);
		AddNativeCalls_RococoIRadioButton(args.ss, nullptr);
		AddNativeCalls_RococoIPane(args.ss, nullptr);
		AddNativeCalls_RococoISlider(args.ss, nullptr);
		AddNativeCalls_RococoIScroller(args.ss, nullptr);
		AddNativeCalls_RococoIEnumListPane(args.ss, nullptr);
		AddNativeCalls_RococoIInventoryArray(args.ss, nullptr);

		if (onCompile)
		{
			// N.B if the caller provides some extra native calls they
			// may want to do something sophisticated with the panels, so give them
			// more of the MPLAT API to work with.

		//	Rococo::Entities::AddNativeCalls_RococoEntitiesIInstances(args.ss, &platform.instances);
			onCompile->OnEvent(args);
		}
	}

	void RefreshScript()
	{
		FreeAllChildren();
		platform.utilities.RunEnvironmentScript(*this, scriptFilename.c_str(), true);
	}

	void Render(IGuiRenderContext& grc, const Vec2i& topLeft, const Modality& modality) override
	{
		if (IsVisible() && IsNormalized())
		{
			if (lastRect != BasePane::ClientRect())
			{
				lastRect = BasePane::ClientRect();
				RefreshScript();
			}
		}

		PaneContainer::Render(grc, topLeft, modality);
	}

	virtual IPaneSupervisor* Supervisor()
	{
		return this;
	}
};

#include <rococo.mplat.h>
#include "mplat.panel.base.h"
#include <rococo.strings.h>
#include <rococo.ui.h>

using namespace Rococo;
using namespace Rococo::Events;
using namespace Rococo::Windows;
using namespace Rococo::MPlatImpl;

class PaneFrame : public PaneContainer, public IFramePane, public IObserver
{
	IPublisher& publisher;
	IRenderer& renderer;
	int32 fontIndex = 1;
	char title[128] = { 0 };
	const int border = 1;
	const int captionHeight = 28;
	ELayoutAlgorithm layoutAlgorithm = ELayoutAlgorithm_MaximizeOnlyChild;

	int32 dragRightPos = -1;
	int32 dragBottomPos = -1;
	Vec2i preDragSpan = { 0,0 };
	EWindowCursor cursor;
	Vec2i captionDragPoint{ -1,-1 };
	Vec2i topLeftAtDrag{ -1, -1 };

	HString caption;
public:
	PaneFrame(Platform& platform) :
		PaneContainer(platform),
		publisher(platform.publisher), renderer(platform.renderer)
	{

	}

	~PaneFrame()
	{
		platform.publisher.Unsubscribe(this);
	}

	void Free() override
	{
		delete this;
	}

	void OnEvent(Event& ev)
	{
		if (ev == evUIMouseEvent)
		{
			auto& dme = As<DirectMouseEvent>(ev);
			OnDirectMouseEvent(dme.me);
			dme.consumed = true;
		}
	}

	void OnDirectMouseEvent(const MouseEvent& me)
	{
		// N.B this callback assumed to be called in response to a drag event
		// Once the drag is over, the subscription to the event is revoked.
		// Since the event is consumed, it is not passed on to the UI system after this function call

		platform.renderer.SetSysCursor(cursor);

		if (dragRightPos > 0)
		{
			GuiRect rect = ClientRect();

			int32 delta = me.cursorPos.x - dragRightPos;
			rect.right = rect.left + preDragSpan.x + delta;

			ClipRect(rect);
			SetRect(rect);
		}

		if (dragBottomPos > 0)
		{
			GuiRect rect = ClientRect();
			int32 delta = me.cursorPos.y - dragBottomPos;
			rect.bottom = rect.top + preDragSpan.y + delta;

			ClipRect(rect);
			SetRect(rect);
		}

		if (captionDragPoint.x > 0)
		{
			GuiRect rect = ClientRect();
			Vec2i delta = me.cursorPos - captionDragPoint;
			rect.left = max(0, topLeftAtDrag.x + delta.x);
			rect.top = max(0, topLeftAtDrag.y + delta.y);

			auto* parent = Parent();

			GuiRect parentRect = parent->ClientRect();

			rect.right = rect.left + preDragSpan.x;
			rect.bottom = rect.top + preDragSpan.y;

			if (rect.right > parentRect.right)
			{
				int32 delta = rect.right - parentRect.right;
				rect.left -= delta;
				rect.right -= delta;
			}

			if (rect.bottom > parentRect.bottom)
			{
				int32 delta = rect.bottom - parentRect.bottom;
				rect.top -= delta;
				rect.bottom -= delta;
			}

			ClipRect(rect);
			SetRect(rect);
		}

		if (me.HasFlag(MouseEvent::LUp))
		{
			dragRightPos = -1;
			dragBottomPos = -1;
			captionDragPoint = { -1,-1 };
			platform.publisher.Unsubscribe(this);
			platform.renderer.CaptureMouse(false);
		}
	}

	bool AppendEvent(const KeyboardEvent& ke, const Vec2i& focusPoint, const Vec2i& absTopLeft) override
	{
		return PaneContainer::AppendEvent(ke, focusPoint, absTopLeft);
	}

	void StartDrag()
	{
		preDragSpan = Span(ClientRect());
		platform.publisher.Subscribe(this, Rococo::Events::evUIMouseEvent);
		platform.renderer.CaptureMouse(true);
	}

	void AppendEvent(const MouseEvent& me, const Vec2i& absTopLeft) override
	{
		int32 farRight = ClientRect().right;
		int32 farBottom = ClientRect().bottom;

		if (me.cursorPos.x <= farRight && me.cursorPos.x > farRight - 4)
		{
			if (me.cursorPos.y <= farBottom && me.cursorPos.y > farBottom - 4)
			{
				cursor = EWindowCursor_BottomRightDrag;
				platform.renderer.SetSysCursor(EWindowCursor_BottomRightDrag);
			}
			else
			{
				cursor = EWindowCursor_HDrag;
				platform.renderer.SetSysCursor(EWindowCursor_HDrag);
			}

			if (me.HasFlag(MouseEvent::LDown))
			{
				dragRightPos = me.cursorPos.x;

				if (me.cursorPos.y <= farBottom && me.cursorPos.y > farBottom - 4)
				{
					dragBottomPos = me.cursorPos.y;
				}

				StartDrag();
				return;
			}
		}
		else if (me.cursorPos.y <= farBottom && me.cursorPos.y > farBottom - 4)
		{
			cursor = EWindowCursor_VDrag;
			platform.renderer.SetSysCursor(EWindowCursor_VDrag);

			if (me.HasFlag(MouseEvent::LDown))
			{
				dragBottomPos = me.cursorPos.y;
				StartDrag();
				return;
			}
		}

		GuiRect captionRect;
		GetCaptionRect(captionRect);

		if (IsPointInRect(me.cursorPos, captionRect))
		{
			if (me.HasFlag(MouseEvent::LDown))
			{
				StartDrag();
				preDragSpan = Span(ClientRect());
				captionDragPoint = me.cursorPos;
				topLeftAtDrag = TopLeft(ClientRect());
				cursor = EWindowCursor_HandDrag;
				return;
			}
		}

		PaneContainer::AppendEvent(me, absTopLeft);
	}

	void GetChildRect(GuiRect& child)
	{
		auto& controlRect = ClientRect();
		child.left = border;
		child.right = Width(controlRect) - 2 * border;
		child.top = captionHeight + border;
		child.bottom = Height(controlRect) - 2 * border;
	}

	void GetCaptionRect(GuiRect& caption)
	{
		auto& controlRect = ClientRect();
		caption.left = controlRect.left + border;
		caption.right = controlRect.right - border;
		caption.top = controlRect.top + border;
		caption.bottom = controlRect.top + captionHeight + 1;
	}

	void SetCaption(const fstring& caption) override
	{
		this->caption = caption;
	}

	void SetLayoutAlgorithm(ELayoutAlgorithm layout)
	{
		switch (layoutAlgorithm)
		{
		case ELayoutAlgorithm_MaximizeOnlyChild:
		case ELayoutAlgorithm_None:
			break;
		default:
			Throw(0, "FramePanel.SetLayoutAlgorithm(%d). Algorithm not implemented", layout);
		}

		this->layoutAlgorithm = layout;
	}

	Vec2i minSpan{ 32, 32 };
	Vec2i maxSpan{ 640, 480 };

	void SetMinMaxSpan(int32 minDX, int32 minDY, int32 maxDX, int32 maxDY) override
	{
		if (minDX < 0 || minDY < 0 || maxDX < 0 || maxDY < 0)
		{
			Throw(0, "FramePanel.SetMinMaxSpan supplied with negative arguments");
		}

		if (minDX > maxDX || minDY > maxDY)
		{
			Throw(0, "FramePanel.SetMinMaxSpan: arguments ordered incorrectly.");
		}

		minSpan = { minDX, minDY };
		maxSpan = { maxDX, maxDY };
	}

	void ClipRect(GuiRect& rect)
	{
		if (rect.right - rect.left < minSpan.x)
			rect.right = rect.left + minSpan.x;
		else if (rect.right - rect.left > maxSpan.x)
			rect.right = rect.left + maxSpan.x;

		if (rect.bottom - rect.top < minSpan.y)
			rect.bottom = rect.top + minSpan.y;
		else if (rect.bottom - rect.top > maxSpan.y)
			rect.bottom = rect.top + maxSpan.y;
	}

	void Render(IGuiRenderContext& grc, const Vec2i& topLeft, const Modality& modality) override
	{
		auto& controlRect = ClientRect();

		GuiRect captionRect;
		GetCaptionRect(captionRect);

		GuiRect child;
		GetChildRect(child);

		switch (layoutAlgorithm)
		{
		case ELayoutAlgorithm_None:
			break;
		case ELayoutAlgorithm_MaximizeOnlyChild:
			if (Children() == 1)
			{
				GetChild(0)->SetRect(child);
			}
			break;
		default:
			break;
		}

		RenderChildren(grc, topLeft, modality);

		Graphics::DrawRectangle(grc, captionRect, RGBAb(0, 0, 192, 255), RGBAb(0, 0, 192, 255));

		GuiRectf textRect{ (float)captionRect.left + 4, (float)captionRect.top + 2, (float)captionRect.right, (float)captionRect.bottom - 2 };
		Rococo::Graphics::DrawText(grc, textRect, 0, to_fstring(caption.c_str()), 0, RGBAb(255, 255, 255));
	}

	Rococo::IPaneContainer* Container()
	{
		return this;
	}

	void SetCaptionEvent(const fstring& eventName) override
	{
		EventIdRef id = publisher.CreateEventIdFromVolatileString(eventName);

		UIInvoke args;
		SafeFormat(args.command, sizeof(args.command), "%s", (cstr) eventName);
		publisher.Publish(args, id);

		caption = args.command;
	}
};


Rococo::IFramePane* PaneContainer::AddFrame(const GuiRect& rect)
{
	auto* f = new PaneFrame(platform);
	AddChild(f);
	f->SetRect(rect);
	return f;
}

namespace Rococo
{
	namespace MPlatImpl
	{
		IPaneContainer* CreatePaneContainer(Platform& platform)
		{
			return new PaneContainer(platform);
		}

		IPaneBuilderSupervisor* CreateScriptedPanel(Platform& platform, cstr filename, IEventCallback<ScriptCompileArgs>* onCompile)
		{
			return new ScriptedPanel(platform, filename, onCompile);
		}
	}
}