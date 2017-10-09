#include <rococo.mplat.h>
#include <rococo.os.win32.h>

#include <rococo.window.h>
#include <rococo.sexy.ide.h>
#include <rococo.dx11.renderer.win32.h>
#include <rococo.strings.h>

#include <vector>
#include <algorithm>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include <rococo.fonts.h>
#include <rococo.variable.editor.h>

using namespace Rococo;
using namespace Rococo::Events;
using namespace Rococo::Windows;

EventId evFileUpdated = "OnFileUpdated"_event;
struct FileUpdatedEvent : public Event
{
   FileUpdatedEvent() : Event(evFileUpdated) {}
   cstr pingPath;
};

namespace Rococo
{
   namespace M
   {
      bool QueryYesNo(IWindow& ownerWindow, cstr message)
      {
         rchar title[256];
         GetWindowTextA(ownerWindow, title, 256);
         return ShowMessageBox(Windows::NullParent(), message, title, MB_ICONQUESTION | MB_YESNO) == IDYES;
      }

      void InitScriptSystem(IInstallation& installation);
   }
}


const char* CreatePersistentString(const char* volatileString)
{
   static std::unordered_set<std::string> persistentStrings;
   auto i = persistentStrings.find(volatileString);
   if (i == persistentStrings.end())
   {
      i = persistentStrings.insert(std::string(volatileString)).first;
   }

   return i->c_str();
}

EventId CreateEventIdFromVolatileString(const char* volatileString)
{
   auto* s = CreatePersistentString(volatileString);
   return EventId(s, (EventHash)FastHash(s));
}

namespace Rococo
{
   namespace M
   {
      void RunEnvironmentScript(Platform& platform, IEventCallback<ScriptCompileArgs>& _onScriptEvent, const char* name, bool addPlatform);
   }
}

class Utilities : public IUtilitiies
{
   IInstallation& installation;
public:
   Utilities(IInstallation& _installation) : installation(_installation) {}

   void EnumerateFiles(IEventCallback<cstr>& cb, cstr pingPathDirectory) override
   {
      struct : IEventCallback<cstr>
      {
         rchar shortdir[IO::MAX_PATHLEN];
         rchar directory[IO::MAX_PATHLEN];
         IEventCallback<cstr>* cb;

         virtual void OnEvent(cstr filename)
         {
            rchar contentRelativePath[IO::MAX_PATHLEN];
            SafeFormat(contentRelativePath, IO::MAX_PATHLEN, "%s%s", shortdir, filename);
            cb->OnEvent(contentRelativePath);
         }
      } onFileFound;

      onFileFound.cb = &cb;

      if (pingPathDirectory == nullptr || pingPathDirectory[0] != '!')
      {
         Throw(0, "Directories must be inside the content directory. Use the '!<directory>' notation");
      }

      StackStringBuilder sb(onFileFound.shortdir, _MAX_PATH);
      sb << pingPathDirectory;

      EndDirectoryWithSlash(onFileFound.shortdir, IO::MAX_PATHLEN);

      SafeFormat(onFileFound.directory, IO::MAX_PATHLEN, "%s%s", installation.Content(), (onFileFound.shortdir + 1));
      IO::ForEachFileInDirectory(onFileFound.directory, onFileFound);
   }

   bool QueryYesNo(Platform& platform, Windows::IWindow& parent, cstr question, cstr caption) override
   {
      cstr title = caption == nullptr ? platform.title : caption;
      return ShowMessageBox(parent, question, title, MB_ICONQUESTION | MB_YESNO) == IDYES;
   }

   void RunEnvironmentScript(Platform& platform, IEventCallback<ScriptCompileArgs>& _onScriptEvent, const char* name, bool addPlatform) override
   {
      return M::RunEnvironmentScript(platform, _onScriptEvent, name, addPlatform);
   }

   void ShowErrorBox(Windows::IWindow& parent, IException& ex, cstr message) override
   {
      OS::ShowErrorBox(parent, ex, message);
   }

   void RefreshResource(Platform& platform, cstr pingPath) override
   {
      FileUpdatedEvent fileUpdated;
      fileUpdated.pingPath = pingPath;
      platform.publisher.Publish(fileUpdated);
   }

   IVariableEditor* CreateVariableEditor(Windows::IWindow& window, const Vec2i& span, int32 labelWidth, cstr appQueryName, cstr defaultTab, cstr defaultTooltip, IVariableEditorEventHandler* eventHandler, const Vec2i* topLeft) override
   {
      return Rococo::CreateVariableEditor(window, span, labelWidth, appQueryName, defaultTab, defaultTooltip, eventHandler, topLeft);
   }
};

class GuiStack : public IGUIStack, public IObserver
{
   struct PanelDesc
   {
      IPanelSupervisor* panel;
      bool isModal;
   };
   std::vector<PanelDesc> panels;

   struct CommandHandler
   {
      std::string helpString;
      ICommandHandler* handler;
      FN_OnCommand method;
   };

   IPublisher& publisher;
   ISourceCache& sourceCache;
   IRenderer& renderer;
   IUtilitiies& utilities;

   std::unordered_map<std::string, CommandHandler> handlers;
   std::unordered_map<std::string, IUIElement*> renderElements;
public:
   Platform* platform;

   GuiStack(IPublisher& _publisher, ISourceCache& _sourceCache, IRenderer& _renderer, IUtilitiies& _utilities) :
      publisher(_publisher),
      sourceCache(_sourceCache),
      renderer(_renderer),
      utilities(_utilities)
   {
      publisher.Attach(this, UIInvoke::EvId());
      publisher.Attach(this, UIPopulate::EvId());
   }

   ~GuiStack()
   {
      publisher.Detach(this);
   }

   void AppendEvent(const MouseEvent& me) override
   {
      for (auto i = panels.rbegin(); i != panels.rend(); ++i)
      {
         if (IsPointInRect(me.cursorPos, i->panel->ClientRect()))
         {
            i->panel->AppendEvent(me, { 0,0 });

            if (i->isModal)
            {
               break;
            }
         }
      }
   }

   bool AppendEvent(const KeyboardEvent& ke)
   {
      GuiMetrics metrics;
      renderer.GetGuiMetrics(metrics);

      for (auto i = panels.rbegin(); i != panels.rend(); ++i)
      {
         if (IsPointInRect(metrics.cursorPosition, i->panel->ClientRect()))
         {
            if (i->panel->AppendEvent(ke, metrics.cursorPosition, { 0,0 }))
            {
               return true;
            }
         }
      }

      return false;
   }

   void OnEvent(Event& ev) override
   {
      if (ev.id == UIInvoke::EvId())
      {
         auto& ui = As<UIInvoke>(ev);

         char directive[256];
         const char* p = ui.command;
         for (int i = 0; i < 256; ++i)
         {
            if (p[i] == 0 || p[i] == ' ')
            {
               directive[i] = 0;
               break;
            }
            else
            {
               directive[i] = p[i];
            }
         }

         directive[255] = 0;

         auto i = handlers.find(directive);
         if (i != handlers.end())
         {
            FN_OnCommand method = i->second.method;
            ICommandHandler* obj = i->second.handler;
            method(obj, ui.command);
         }
      }
      else if (ev.id == UIPopulate::EvId())
      {
         auto& pop = As<UIPopulate>(ev);
         
         auto i = renderElements.find(pop.name);
         if (i != renderElements.end())
         {
            pop.renderElement = i->second;
         }
      }
   }

   void RegisterPopulator(cstr name, IUIElement* renderElement) override
   {
      renderElements[name] = renderElement;
   }

   void UnregisterPopulator(IUIElement* renderElement) override
   {
      auto i = renderElements.begin();
      while (i != renderElements.end())
      {
         if (i->second == renderElement)
         {
            i = renderElements.erase(i);
         }
         else
         {
            i++;
         }
      }
   }

   void RegisterEventHandler(ICommandHandler* handler, FN_OnCommand method, cstr cmd, cstr helpString) override
   {
      handlers[cmd] = { std::string(helpString == nullptr ? "" : helpString), handler, method };
   }

   void UnregisterEventHandler(ICommandHandler* handler)
   {
      auto i = handlers.begin();
      while (i != handlers.end())
      {
         if (i->second.handler == handler)
         {
            i = handlers.erase(i);
         }
         else
         {
            i++;
         }
      }
   }

   void Render(IGuiRenderContext& grc) override
   {
      if (panels.empty()) return;

      bool hiddenByModal = false;

      Modality modality;
      modality.isUnderModal = false;

      for (size_t i = 0; i < panels.size() - 1; ++i)
      {
         auto& p = panels[i];
         auto& rect = p.panel->ClientRect();
         modality.isModal = p.isModal;
         modality.isTop = false;
         modality.isUnderModal = false;

         for (size_t j = i + 1; j < panels.size(); ++j)
         {
            if (panels[j].isModal)
            {
               modality.isUnderModal = true;
               break;
            }
         }

         p.panel->Render(grc, { rect.left, rect.top }, modality);
      }

      auto& p = panels.back();
      auto& rect = p.panel->ClientRect();

      modality.isUnderModal = false;
      modality.isTop = true;
      modality.isModal = p.isModal;
      p.panel->Render(grc, { rect.left, rect.top }, modality);
   }

   void PushTop(IPanelSupervisor* panel, bool isModal) override
   {
      panels.push_back({ panel,isModal });
   }

   IPanelSupervisor* Pop() override
   {
      if (panels.empty()) Throw(0, "GuiStack: panels empty, nothing to pop");

      auto* p = panels.back().panel;
      panels.pop_back();
      return p;
   }

   IPanelSupervisor* Top() override
   {
      return panels.empty() ? nullptr : panels.back().panel;
   }

   IPaneBuilderSupervisor* BindPanelToScript(cstr scriptName) override;
};

class BasePanel : public IPanelSupervisor
{
   GuiRect rect{ 0, 0, 0, 0 };
   bool isVisible{ true };

   std::vector<IPanelSupervisor*> children;

   struct UICommand
   {
      std::string command;
      boolean32 defer;
   };

   std::vector<UICommand> commands;

   struct UIPopulator
   {
      std::string name;
   };

   std::vector<UIPopulator> populators;

   ColourScheme scheme
   {
      RGBAb(160,160,160, 192),
      RGBAb(192,192,192, 192),
      RGBAb(255,255,255, 224),
      RGBAb(224,224,224, 224),
      RGBAb(255,255,255, 255),
      RGBAb(160,160,160, 224),
      RGBAb(192,192,192, 224),
      RGBAb(255,255,255, 255),
      RGBAb(224,224,224, 255),
      RGBAb(255,255,255, 255),
   };

public:
   void SetCommand(int32 stateIndex, boolean32 deferAction, const fstring& text) override
   {
      if (stateIndex < 0 || stateIndex > 4)
      {
         Throw(0, "BasePanel::SetCommand: stateIndex %d out of bounds [0,4]", stateIndex);
      }

      if (stateIndex >= commands.size())
      {
         commands.resize(stateIndex + 1);
      }

      commands[stateIndex] = { std::string(text), deferAction };
   }

   void Invoke(IPublisher& publisher, int32 stateIndex)
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
               publisher.Post(invoke, true);
            }
            else
            {
               publisher.Publish(invoke);
            }
         }
      }
   }

   void SetPopulator(int32 stateIndex, const fstring& populatorName) override
   {
      if (stateIndex < 0 || stateIndex > 4)
      {
         Throw(0, "BasePanel::SetCommand: stateIndex %d out of bounds [0,4]", stateIndex);
      }

      if (stateIndex >= populators.size())
      {
         populators.resize(stateIndex + 1);
      }

      if (populatorName.length >= 192)
      {
         Throw(0, "BasePanel::SetPopulator(...): Maximum length for populator name is 192 chars");
      }

      populators[stateIndex] = { std::string(populatorName) };
   }

   void AppendEventToChildren(IPublisher& publisher, const MouseEvent& me, const Vec2i& absTopLeft, int stateIndex = 0)
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
            publisher.Publish(populate);

            if (populate.renderElement)
            {
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

   bool AppendEventToChildren(IPublisher& publisher, const KeyboardEvent& ke, const Vec2i& focusPoint, const Vec2i& absTopLeft, int stateIndex = 0)
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
            publisher.Publish(populate);

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

   void AlignLeftEdges(int32 x, boolean32 preserveSpan) override
   {
      for (auto i : children)
      {
         GuiRect rect = i->ClientRect();
         if (preserveSpan)
         {
            Vec2i span = Span(rect);
            rect.right = x + span.x;
         }

         rect.left = x;
         i->SetRect(rect);
      }
   }

   void AlignRightEdges(int32 x, boolean32 preserveSpan) override
   {
      int x0 = (rect.right - rect.left) - x;
      for (auto i : children)
      {
         GuiRect rect = i->ClientRect();
         if (preserveSpan)
         {
            Vec2i span = Span(rect);
            rect.left = x - span.x;
         }

         rect.right = x0;
         i->SetRect(rect);
      }
   }

   void LayoutVertically(int32 vertBorder, int32 vertSpacing) override
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
         vertSpacing = freeSpace / (int32) children.size();
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


   void SetRect(const GuiRect& rect) override
   {
      this->rect = rect;
   }

   const GuiRect& ClientRect() const override
   {
      return rect;
   }

   void GetRect(GuiRect& outRect) override
   {
      outRect = rect;
   }

   boolean32 IsVisible() override
   {
      return isVisible;
   }

   boolean32 IsNormalized() override
   {
      return rect.right > rect.left && rect.bottom > rect.top;
   }

   void SetVisible(boolean32 visible) override
   {
      this->isVisible = isVisible;
   }

   IPanelSupervisor* operator[](int index) override
   {
      return GetChild(index);
   }

   IPanelSupervisor* GetChild(int index)
   {
      if (index < 0 || index >= children.size())
      {
         Throw(0, "BasePanel[index] -> index %d was out of bounds. Child count: %d", index, children.size());
      }

      return children[index];
   }

   int Children() const override
   {
      return (int)children.size();
   }

   void AddChild(IPanelSupervisor* child) override
   {
      children.push_back(child);
   }

   void RemoveChild(IPanelSupervisor* child) override
   {
      auto i = std::remove(children.begin(), children.end(), child);
      children.erase(i, children.end());
   }

   void FreeAllChildren() override
   {
      for (auto i : children)
      {
         i->FreeAllChildren();
         i->Free();
      }
      children.clear();
   }

   void SetScheme(const ColourScheme& scheme) override
   {
      this->scheme = scheme;
   }

   const ColourScheme& Scheme() const
   {
      return scheme;
   }

   void SetColourBk1(RGBAb normal, RGBAb hilight) override
   {
      scheme.topLeft = normal;
      scheme.hi_topLeft = hilight;
   }

   void SetColourBk2(RGBAb normal, RGBAb hilight) override
   {
      scheme.bottomRight = normal;
      scheme.hi_bottomRight = hilight;
   }

   void SetColourEdge1(RGBAb normal, RGBAb hilight) override
   {
      scheme.topLeftEdge = normal;
      scheme.hi_topLeftEdge = hilight;
   }

   void SetColourEdge2(RGBAb normal, RGBAb hilight) override
   {
      scheme.bottomRightEdge = normal;
      scheme.hi_bottomRightEdge = hilight;
   }

   void SetColourFont(RGBAb normal, RGBAb hilight) override
   {
      scheme.fontColour = normal;
      scheme.hi_fontColour = hilight;
   }

   void Populate(IPublisher& publisher, IGuiRenderContext& grc, int32 stateIndex, const Vec2i& topLeft)
   {
      if (stateIndex >= 0 && stateIndex < (int32)populators.size())
      {
         UIPopulate populate;
         populate.renderElement = nullptr;
         populate.name = populators[stateIndex].name.c_str();      
         publisher.Publish(populate);

         if (populate.renderElement)
         {
            GuiRect absRect = GuiRect { 0, 0, Width(rect), Height(rect) } + topLeft;
            populate.renderElement->Render(grc, absRect);
         }
      }
   }

   void RenderBackground(IGuiRenderContext& grc, const Vec2i& topLeft, const Modality& modality)
   {
      GuiMetrics metrics;
      grc.Renderer().GetGuiMetrics(metrics);

      auto p = metrics.cursorPosition;

      GuiRect absRect{ topLeft.x, topLeft.y, topLeft.x + rect.right - rect.left, topLeft.y + rect.bottom - rect.top };

      if (!modality.isUnderModal && IsPointInRect(p, absRect))
      {
         Graphics::DrawRectangle(grc, absRect, scheme.hi_topLeft, scheme.hi_bottomRight);
         Graphics::DrawBorderAround(grc, absRect, Vec2i{ 1, 1 }, scheme.hi_topLeftEdge, scheme.hi_bottomRightEdge);
      }
      else
      {
         Graphics::DrawRectangle(grc, absRect, scheme.topLeft, scheme.bottomRight);
         Graphics::DrawBorderAround(grc, absRect, Vec2i{ 1, 1 }, scheme.topLeftEdge, scheme.bottomRightEdge);
      }
   }

   void RenderChildren(IGuiRenderContext& grc, const Vec2i& topLeft, const Modality& modality)
   {
      auto& currentRect = ClientRect();
      if (IsVisible())
      {
         if (IsNormalized()) RenderBackground(grc, topLeft, modality);

         for (auto i: children)
         {
            auto childRect = i->ClientRect() + topLeft;
            i->Render(grc, { childRect.left, childRect.top }, modality);
         }
      }
   }
};

bool operator != (const GuiRect& a, const GuiRect& b)
{
   return a.left != b.left || a.right != b.right || a.bottom != b.bottom || a.top != b.top;
}

class PanelContainer : public BasePanel, public IPaneContainer
{
   Platform& platform;
public:
   PanelContainer(Platform& _platform): platform(_platform)
   {

   }

   Rococo::IPaneContainer* AddContainer(const GuiRect& rect)
   {
      auto* container = new PanelContainer(platform);
      AddChild(container);
      container->SetRect(rect);
      return container;
   }

   Rococo::ITextOutputPane* AddTextOutput(int32 fontIndex, const fstring& eventKey, const GuiRect& rect) override;
   Rococo::ILabelPane* AddLabel(int32 fontIndex, const fstring& text, const GuiRect& rect) override;
   Rococo::ISlider* AddSlider(int32 fontIndex, const fstring& text, const GuiRect& rect, float minValue, float maxValue) override;
   Rococo::IRadioButton* AddRadioButton(int32 fontIndex, const fstring& text, const fstring& key, const fstring& value, const GuiRect& rect) override;
   Rococo::IScroller* AddScroller(const fstring& key, const GuiRect& rect, boolean32 isVertical) override;

   IPane* Base() override
   {
      return this;
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

void RenderLabel(IGuiRenderContext& grc, cstr text, const GuiRect& absRect, int horzAlign, int vertAlign, Vec2i padding, int fontIndex, const ColourScheme& scheme, bool enableHighlights)
{
   GuiMetrics metrics;
   grc.Renderer().GetGuiMetrics(metrics);
 
   using namespace Rococo::Fonts;
   struct : IDrawTextJob
   {
      cstr text;
      int fontIndex;
      RGBAb colour;

      virtual void OnDraw(IGlyphBuilder& builder)
      { 
         builder.SetTextColour((FontColour&)colour);
         builder.SetFontIndex(fontIndex);

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

class PanelRadioButton : public BasePanel, public IRadioButton, public IObserver
{
   int32 fontIndex = 1;
   rchar text[128];
   EventId id;
   rchar value[128];
   int32 horzAlign = 0;
   int32 vertAlign = 0;
   Vec2i padding{ 0,0 };
   IPublisher& publisher;

   int32 stateIndex = -1; // indeterminate
public:
   PanelRadioButton(IPublisher& _publisher, int _fontIndex, cstr _text, cstr _key, cstr _value) :
      id(CreateEventIdFromVolatileString(_key)),
      fontIndex(_fontIndex), publisher(_publisher)
   {
      SafeFormat(text, sizeof(text), "%s", _text);
      SafeFormat(value, sizeof(value), "%s", _value);

      publisher.Attach(this, id);
   }

   ~PanelRadioButton()
   {
      publisher.Detach(this);
   }

   void OnEvent(Event& ev) override
   {
      if (ev.id == id)
      {
         auto& toe = As<TextOutputEvent>(ev);
         if (!toe.isGetting)
         {
            stateIndex = (toe.text && Eq(toe.text, value)) ? 1 : 0;
         }
      }
   }

   void Free() override
   {
      delete this;
   }

   IPane* Base() override
   {
      return this;
   }

   void SetAlignment(int32 horz, int32 vert, int32 paddingX, int paddingY)
   {
      horzAlign = horz;
      vertAlign = vert;
      padding = { paddingX, paddingY };
   }

   bool AppendEvent(const KeyboardEvent& me, const Vec2i& focusPoint, const Vec2i& absTopLeft) override
   {
      return false;
   }

   void AppendEvent(const MouseEvent& me, const Vec2i& absTopLeft) override
   {
      if (stateIndex == 0 && me.HasFlag(me.LUp) || me.HasFlag(me.RUp))
      {
         TextOutputEvent toe(id);
         toe.isGetting = false;
         SecureFormat(toe.text, sizeof(toe.text), value);
         publisher.Publish(toe);
      }
   }

   void Render(IGuiRenderContext& grc, const Vec2i& topLeft, const Modality& modality) override
   {
      GuiMetrics metrics;
      grc.Renderer().GetGuiMetrics(metrics);

      if (stateIndex == -1)
      {
         TextOutputEvent toe(id);
         toe.isGetting = true;
         publisher.Publish(toe);
         stateIndex = (toe.text && Eq(toe.text, value)) ? 1 : 0;
      }

      auto p = metrics.cursorPosition;

      GuiRect absRect = GuiRect { 0, 0, Width(ClientRect()), Height(ClientRect()) } + topLeft;
      auto& scheme = Scheme();

      if (stateIndex != 0)
      {
         Graphics::DrawRectangle(grc, absRect, scheme.hi_topLeft, scheme.hi_bottomRight);
         Graphics::DrawBorderAround(grc, absRect, Vec2i{ 1, 1 }, scheme.hi_topLeftEdge, scheme.hi_bottomRightEdge);
      }
      else
      {
         Graphics::DrawRectangle(grc, absRect, scheme.topLeft, scheme.bottomRight);
         Graphics::DrawBorderAround(grc, absRect, Vec2i{ 1, 1 }, scheme.topLeftEdge, scheme.bottomRightEdge);
      }

      RenderLabel(grc, text, absRect, horzAlign, vertAlign, padding, fontIndex, scheme, !modality.isUnderModal);
   }
};

class PanelLabel : public BasePanel, public ILabelPane
{
   int32 fontIndex = 1;
   rchar text[128];
   int32 horzAlign = 0;
   int32 vertAlign = 0;
   Vec2i padding{ 0,0 };
   IPublisher& publisher;
public:
   PanelLabel(IPublisher& _publisher, int _fontIndex, cstr _text): fontIndex(_fontIndex), publisher(_publisher)
   {
      SafeFormat(text, sizeof(text), "%s", _text);
   }

   void Free() override
   {
      delete this;
   }

   IPane* Base() override
   {
      return this;
   }

   void SetAlignment(int32 horz, int32 vert, int32 paddingX, int paddingY)
   {
      horzAlign = horz;
      vertAlign = vert;
      padding = { paddingX, paddingY };
   }

   void AppendEvent(const MouseEvent& me, const Vec2i& absTopLeft) override
   {
      if (me.HasFlag(me.LUp) || me.HasFlag(me.RUp))
      {
         BasePanel::Invoke(publisher, 1);
      }
      else if (me.HasFlag(me.LDown) || me.HasFlag(me.RDown))
      {
         BasePanel::Invoke(publisher, 0);
      }
   }

   bool AppendEvent(const KeyboardEvent& me, const Vec2i& focusPoint, const Vec2i& absTopLeft) override
   {
      return false;
   }

   void Render(IGuiRenderContext& grc, const Vec2i& topLeft, const Modality& modality) override
   {
      RenderBackground(grc, topLeft, modality);

      auto span = Span(ClientRect());
      GuiRect absRect{ topLeft.x, topLeft.y, topLeft.x + span.x, topLeft.y + span.y};

      RenderLabel(grc, text, absRect, horzAlign, vertAlign, padding, fontIndex, BasePanel::Scheme(), !modality.isUnderModal);
   }
};

class PanelSlider : public BasePanel, public ISlider
{
   int32 fontIndex = 1;
   rchar text[128];
   IPublisher& publisher;
   IRenderer& renderer;
   float minValue;
   float maxValue;
   float value;
public:
   PanelSlider(IPublisher& _publisher, IRenderer& _renderer, int _fontIndex, cstr _text, float _minValue, float _maxValue) :
      publisher(_publisher), renderer(_renderer), fontIndex(_fontIndex), minValue(_minValue), maxValue(_maxValue)
   {
      SafeFormat(text, sizeof(text), "%s", _text);
      value = 0.5f * (maxValue + minValue);
   }

   void Free() override
   {
      delete this;
   }

   IPane* Base() override
   {
      return this;
   }

   bool AppendEvent(const KeyboardEvent& me, const Vec2i& focusPoint, const Vec2i& absTopLeft) override
   {
      return false;
   }

   void AppendEvent(const MouseEvent& me, const Vec2i& absTopLeft) override
   {
      if (me.HasFlag(me.LUp))
      {
         GuiMetrics metrics;
         renderer.GetGuiMetrics(metrics);

         auto ds = metrics.cursorPosition - absTopLeft;

         int32 width = Width(ClientRect());

         float delta = ds.x / (float) width;

         value = delta * (maxValue - minValue);
      }
   }

   void Render(IGuiRenderContext& grc, const Vec2i& topLeft, const Modality& modality) override
   {
      auto& controlRect = ClientRect();

      float fspan = maxValue - minValue;
      float delta = (value - minValue) / fspan;

      int32 right = (int32) (delta * (controlRect.right - controlRect.left));

      GuiRect sliderRect
      {
         topLeft.x,
         topLeft.y,
         topLeft.x + right,
         topLeft.y + Height(controlRect)
      };

      auto span = Span(ClientRect());
      GuiRect absRect{ topLeft.x, topLeft.y, topLeft.x + span.x, topLeft.y + span.y };

      GuiMetrics metrics;
      grc.Renderer().GetGuiMetrics(metrics);

      bool lit = !modality.isUnderModal && IsPointInRect(metrics.cursorPosition, absRect);
      Graphics::DrawRectangle(grc, sliderRect, lit ? Scheme().hi_topLeft : Scheme().topLeft, lit ? Scheme().hi_bottomRight : Scheme().bottomRight);
    
      char fullText[256];
      SafeFormat(fullText, sizeof(fullText), "%s - %0.0f%%", text, delta * 100.0f);

      RenderLabel(grc, fullText, absRect, 0, 0, { 0,0 }, fontIndex, BasePanel::Scheme(), !modality.isUnderModal);
       
      Graphics::DrawBorderAround(grc, absRect, { 1,1 }, lit ? Scheme().hi_topLeftEdge :  Scheme().topLeftEdge, lit ? Scheme().hi_bottomRightEdge : Scheme().bottomRightEdge);
   }
};

class PanelScrollbar : public BasePanel, public IScroller, IObserver
{
   std::string key;
   IPublisher& publisher;
   IRenderer& renderer;
   int32 minValue = 0;
   int32 maxValue = 0;
   int32 value = 0;
   int32 rowSize = 0;
   int32 pageSize = 0;
   boolean32 isVertical;
   EventId setScrollId;
   EventId getScrollId;
   EventId uiScrollId;
   IKeyboardSupervisor& keyboard;
public:
   PanelScrollbar(IPublisher& _publisher, IRenderer& _renderer, IKeyboardSupervisor& _keyboard, cstr _key, boolean32 _isVertical) :
      publisher(_publisher), renderer(_renderer), key(_key), isVertical(_isVertical), keyboard(_keyboard),
      setScrollId(""_event), getScrollId(""_event), uiScrollId(""_event)
   {
      char eventText[256];
     
      {
         SecureFormat(eventText, sizeof(eventText), "%s_set", _key);
         EventId id = CreateEventIdFromVolatileString(eventText);
         memcpy(&setScrollId, &id, sizeof(id));
      }

      {
         SecureFormat(eventText, sizeof(eventText), "%s_get", _key);
         EventId id = CreateEventIdFromVolatileString(eventText);
         memcpy(&getScrollId, &id, sizeof(id));
      }

      {
         SecureFormat(eventText, sizeof(eventText), "%s_ui", _key);
         EventId id = CreateEventIdFromVolatileString(eventText);
         memcpy(&uiScrollId, &id, sizeof(id));
      }

      publisher.Attach(this, getScrollId);
      publisher.Attach(this, setScrollId);
   }

   ~PanelScrollbar()
   {
      publisher.Detach(this);
   }

   void OnEvent(Event& ev) override
   {
      auto& s = As<ScrollEvent>(ev);

      if (ev.id == setScrollId)
      {     
         minValue = s.logicalMinValue;
         maxValue = s.logicalMaxValue;
         value = s.logicalValue;
         pageSize = s.logicalPageSize;
         rowSize = s.rowSize;
      }
      else if (ev.id == getScrollId)
      {
         s.logicalMinValue = minValue;
         s.logicalMaxValue = maxValue;
         s.logicalValue = value;
         s.logicalPageSize = pageSize;
         s.rowSize = rowSize;
      }
   }

   void Free() override
   {
      delete this;
   }

   IPane* Base() override
   {
      return this;
   }

   bool AppendEvent(const KeyboardEvent& k, const Vec2i& focusPoint, const Vec2i& absTopLeft) override
   {
      Key key = keyboard.GetKeyFromEvent(k);
      if (key.isPressed)
      {
         bool consume = true;

         if (Eq(key.KeyName, "HOME"))
         {
            value = minValue;
         }
         else if (Eq(key.KeyName, "END"))
         {
            value = maxValue - pageSize;
         }
         else if (Eq(key.KeyName, "PGUP"))
         {
            value -= pageSize;
         }
         else if (Eq(key.KeyName, "PGDOWN"))
         {
            value += pageSize;
         }
         else if (Eq(key.KeyName, "UP"))
         {
            value -= rowSize;
         }
         else if (Eq(key.KeyName, "DOWN"))
         {
            value += rowSize;
         }
         else
         {
            consume = false;
         }

         CapValue(value);

         if (consume)
         {
            ScrollEvent s(uiScrollId);
            s.logicalMinValue = minValue;
            s.logicalMaxValue = maxValue;
            s.logicalValue = value;
            s.logicalPageSize = pageSize;
            s.fromScrollbar = true;
            s.rowSize = rowSize;
            publisher.Publish(s);
         }

         return consume;
      }
      return false;
   }

   int32 PixelRange() const
   {
      auto span = Span(ClientRect());
      return isVertical ? span.y : span.x;
   }

   int32 LogicalRange() const
   {
      return maxValue - minValue;
   }

   int32 PixelSelectValue(const Vec2i& relPos) const
   {
      return (isVertical) ? relPos.y : relPos.x;
   }

   int32 LogicalValue(int32 pixelValue)
   {
      float pr = (float)PixelRange();
      return pr == 0 ? 0 : (int32)((pixelValue / pr * (float) LogicalRange())) + minValue;
   }

   void CapValue(int32 candidateValue)
   {
      if (candidateValue < minValue) value = minValue;
      else if (candidateValue + pageSize > maxValue) value = maxValue - pageSize;
      else
      {
         value = candidateValue;
      }
   }

   int32 PixelValue(int32 logicalValue)
   {
      float lr = (float)LogicalRange();
      return lr == 0 ? 0 : (int32)((logicalValue / lr) * (float)PixelRange());
   }

   int32 PageSizeInPixels()
   {
      float lr = (float)LogicalRange();
      return lr == 0 ? 0 : (int32)((pageSize / lr) * (float) PixelRange());
   }

   Vec2i grabPoint{ -1,-1 };
   int32 grabPixelValue = -1;

   void AppendEvent(const MouseEvent& me, const Vec2i& absTopLeft) override
   {
      int32 oldValue = value;

      if (me.HasFlag(me.LDown))
      {
         Vec2i ds = me.cursorPos - absTopLeft;
         int32 pixelValue = PixelSelectValue(ds);
         if (pixelValue < PixelValue(value))
         {
            value -= pageSize;
         }
         else if (pixelValue > PixelValue(value + pageSize))
         {
            value += pageSize;
         }
         else
         {
            grabPoint = me.cursorPos;
            grabPixelValue = PixelValue(value);
         }

         CapValue(value);
      }

      if (me.HasFlag(me.RDown))
      {
         Vec2i ds = me.cursorPos - absTopLeft;
         int32 pixelValue = PixelSelectValue(ds);
         if (pixelValue < PixelValue(value))
         {
            value -= rowSize;
         }
         else if (pixelValue > PixelValue(value + pageSize))
         {
            value += rowSize;
         }

         CapValue(value);
      }

      if (me.HasFlag(me.MouseWheel))
      {
         int32 delta = (int32)( ((short) me.buttonData) / 120 );
         value -= rowSize * delta;
         CapValue(value);
      }

      if (oldValue != value)
      {
         ScrollEvent s(uiScrollId);
         s.logicalMinValue = minValue;
         s.logicalMaxValue = maxValue;
         s.logicalValue = value;
         s.logicalPageSize = pageSize;
         s.fromScrollbar = true;
         s.rowSize = rowSize;
         publisher.Publish(s);
      }

      if (me.HasFlag(me.LUp))
      {
         grabPoint = { -1,-1 };
         grabPixelValue = -1;
      }
   }

   int trapCount = 0;

   void Render(IGuiRenderContext& grc, const Vec2i& topLeft, const Modality& modality) override
   {
      if (LogicalRange() == 0)
      {
         ScrollEvent ev(uiScrollId);
         ev.fromScrollbar = false;
         publisher.Publish(ev); 
         maxValue = ev.logicalMaxValue;
         minValue = ev.logicalMinValue;
         value = ev.logicalValue;
         pageSize = ev.logicalPageSize;
         rowSize = ev.rowSize;
      }

      auto span = Span(ClientRect());

      GuiRect absRect = GuiRect { 0, 0, span.x, span.y } + topLeft;

      GuiMetrics metrics;
      grc.Renderer().GetGuiMetrics(metrics);

      bool lit = !modality.isUnderModal && IsPointInRect(metrics.cursorPosition, absRect);

      if (lit)
      {
         trapCount = 0;

         if (grabPoint.x >= 0)
         {
            Vec2i delta = metrics.cursorPosition - grabPoint;
            int32 dPixels = isVertical ? delta.y : delta.x;
            int32 pixelPos = grabPixelValue + dPixels;
            int32 newValue = LogicalValue(pixelPos);

            if (dPixels != 0)
            {
               if (newValue < minValue)
               {
                  value = minValue;
                  grabPoint = metrics.cursorPosition;
                  grabPixelValue = 0;
               }
               else if (newValue + pageSize > maxValue)
               {
                  value = maxValue - pageSize;
                  grabPoint = metrics.cursorPosition;
                  grabPixelValue = PixelValue(value);
               }
               else 
               {
                  value = newValue;
               }

               ScrollEvent s(uiScrollId);
               s.logicalMinValue = minValue;
               s.logicalMaxValue = maxValue;
               s.logicalValue = value;
               s.logicalPageSize = pageSize;
               s.fromScrollbar = true;
               s.rowSize = rowSize;
               publisher.Publish(s);
            }
         }
      }
      else
      {
         trapCount++;
         if (trapCount > 50)
         {
            trapCount = 0;
            grabPoint = { -1,-1 };
         }
      }

      Graphics::DrawRectangle(grc, absRect, lit ? Scheme().hi_topLeft : Scheme().topLeft, lit ? Scheme().hi_bottomRight : Scheme().bottomRight);
      Graphics::DrawBorderAround(grc, absRect, { 1,1 }, lit ? Scheme().hi_topLeftEdge : Scheme().topLeftEdge, lit ? Scheme().hi_bottomRightEdge : Scheme().bottomRightEdge);

      GuiRect sliderRect;

      if (isVertical)
      {
         sliderRect = GuiRect{ 1, 1, span.x - 2, PageSizeInPixels() } +Vec2i{ 0, PixelValue(value) } + topLeft;
      }
      else
      {
         sliderRect = GuiRect{ 1, 1, PageSizeInPixels(), span.y - 2 } +Vec2i{ PixelValue(value), 0 } + topLeft;
      }

      Graphics::DrawRectangle(grc, sliderRect, lit ? Scheme().hi_fontColour : Scheme().fontColour, lit ? Scheme().hi_fontColour : Scheme().fontColour);
      Graphics::DrawBorderAround(grc, sliderRect, { 1,1 }, lit ? Scheme().hi_topLeftEdge : Scheme().topLeftEdge, lit ? Scheme().hi_bottomRightEdge : Scheme().bottomRightEdge);
   }
};

class PanelTextOutput : public BasePanel, public ITextOutputPane
{
   int32 fontIndex = 1;
   int32 horzAlign = 0;
   int32 vertAlign = 0;
   Vec2i padding{ 0,0 };
   IPublisher& publisher;
   EventId id;
public:
   PanelTextOutput(IPublisher& _publisher, int _fontIndex, cstr _key) :
      id(CreateEventIdFromVolatileString(_key)),
      publisher(_publisher), fontIndex(_fontIndex)
   {
   }

   ~PanelTextOutput()
   {
   }

   void Free() override
   {
      delete this;
   }

   IPane* Base() override
   {
      return this;
   }

   void SetAlignment(int32 horz, int32 vert, int32 paddingX, int paddingY)
   {
      horzAlign = horz;
      vertAlign = vert;
      padding = { paddingX, paddingY };
   }

   bool AppendEvent(const KeyboardEvent& me, const Vec2i& focusPoint, const Vec2i& absTopLeft) override
   {
      return false;
   }

   void AppendEvent(const MouseEvent& me, const Vec2i& absTopLeft) override
   {

   }

   void Render(IGuiRenderContext& grc, const Vec2i& topLeft, const Modality& modality) override
   {
      RenderBackground(grc, topLeft, modality);

      auto span = Span(ClientRect());
      GuiRect absRect{ topLeft.x, topLeft.y, topLeft.x + span.x, topLeft.y + span.y };

      TextOutputEvent event(id);
      event.isGetting = true;
      *event.text = 0;
      publisher.Publish(event);

      RenderLabel(grc, event.text, absRect, horzAlign, vertAlign, padding, fontIndex, Scheme(), !modality.isUnderModal);
   }
};

Rococo::ILabelPane* PanelContainer::AddLabel(int32 fontIndex, const fstring& text, const GuiRect& rect)
{
   auto* label = new PanelLabel(platform.publisher, fontIndex, text);
   AddChild(label);
   label->SetRect(rect);
   return label;
}

Rococo::ISlider* PanelContainer::AddSlider(int32 fontIndex, const fstring& text, const GuiRect& rect, float minValue, float maxValue)
{
   auto* s = new PanelSlider(platform.publisher, platform.renderer, fontIndex, text, minValue, maxValue);
   AddChild(s);
   s->SetRect(rect);
   return s;
}

Rococo::ITextOutputPane* PanelContainer::AddTextOutput(int32 fontIndex, const fstring& eventKey, const GuiRect& rect)
{
   auto* to = new PanelTextOutput(platform.publisher, fontIndex, eventKey);
   AddChild(to);
   to->SetRect(rect);
   return to;
}

Rococo::IRadioButton* PanelContainer::AddRadioButton(int32 fontIndex, const fstring& text, const fstring& key, const fstring& value, const GuiRect& rect)
{
   auto* radio = new PanelRadioButton(platform.publisher, fontIndex, text, key, value);
   AddChild(radio);
   radio->SetRect(rect);
   return radio;
}

Rococo::IScroller* PanelContainer::AddScroller(const fstring& key, const GuiRect& rect, boolean32 isVertical)
{
   auto* scroller = new PanelScrollbar(platform.publisher, platform.renderer, platform.keyboard, key, isVertical);
   AddChild(scroller);
   scroller->SetRect(rect);
   return scroller;
}

class ScriptedPanel : IEventCallback<ScriptCompileArgs>, IObserver, public IPaneBuilderSupervisor, public PanelContainer
{
   GuiRect lastRect{ 0, 0, 0, 0 };
   std::string scriptFilename;
   Platform& platform;
public:
   ScriptedPanel(Platform& _platform, cstr _scriptFilename) : PanelContainer(_platform),
      platform(_platform),
      scriptFilename(_scriptFilename)
   {
      platform.publisher.Attach(this, evFileUpdated);
   }

   ~ScriptedPanel()
   {
      platform.publisher.Detach(this);  
   }

   void OnEvent(Event& ev) override
   {
      if (ev.id == evFileUpdated)
      {
         auto& fue = As<FileUpdatedEvent>(ev);
         if (Rococo::Eq(fue.pingPath, scriptFilename.c_str()))
         {
            platform.sourceCache.Release(fue.pingPath);
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

   virtual void OnEvent(ScriptCompileArgs& args)
   {
      AddNativeCalls_RococoIPaneContainer(args.ss, nullptr);
      AddNativeCalls_RococoILabelPane(args.ss, nullptr);
      AddNativeCalls_RococoIPaneBuilder(args.ss, this);
      AddNativeCalls_RococoITextOutputPane(args.ss, nullptr);
      AddNativeCalls_RococoIRadioButton(args.ss, nullptr);
      AddNativeCalls_RococoIPane(args.ss, nullptr);
      AddNativeCalls_RococoISlider(args.ss, nullptr);
      AddNativeCalls_RococoIScroller(args.ss, nullptr);
   }

   void RefreshScript()
   {
      FreeAllChildren();
      platform.utilities.RunEnvironmentScript(platform, *this, scriptFilename.c_str(), false);
   }

   void Render(IGuiRenderContext& grc, const Vec2i& topLeft, const Modality& modality) override
   {
      if (IsVisible() && IsNormalized())
      {
         if (lastRect != BasePanel::ClientRect())
         {
            lastRect = BasePanel::ClientRect();
            RefreshScript();
         }
      }

      PanelContainer::Render(grc, topLeft, modality);
   }

   virtual IPanelSupervisor* Supervisor()
   {
      return this;
   }
};

IPaneBuilderSupervisor* GuiStack::BindPanelToScript(cstr scriptName)
{
   return new ScriptedPanel(*platform, scriptName);
}

namespace Rococo
{
   namespace Graphics
   {
      IMeshBuilderSupervisor* CreateMeshBuilder(IRenderer& renderer);
   }
}

void Main(HANDLE hInstanceLock, IAppFactory& appFactory, cstr title)
{
   AutoFree<IOSSupervisor> os = GetOS();
   AutoFree<IInstallationSupervisor> installation = CreateInstallation("content.indicator.txt", *os);
   AutoFree<Rococo::Events::IPublisherSupervisor> publisher(Events::CreatePublisher());
   os->Monitor(installation->Content());

   AutoFree<IDX11Window> mainWindow(CreateDX11Window(*installation));
   SetWindowTextA(mainWindow->Window(), title);

   AutoFree<ISourceCache> sourceCache(CreateSourceCache(*installation));
   AutoFree<IDebuggerWindow> debuggerWindow(Windows::IDE::CreateDebuggerWindow(mainWindow->Window()));

   Rococo::M::InitScriptSystem(*installation);

   AutoFree<Graphics::IMeshBuilderSupervisor> meshes = Graphics::CreateMeshBuilder(mainWindow->Renderer());
   AutoFree<Entities::IInstancesSupervisor> instances = Entities::CreateInstanceBuilder(*meshes, mainWindow->Renderer());
   AutoFree<Entities::IMobilesSupervisor> mobiles = Entities::CreateMobilesSupervisor(*instances);

   AutoFree<IMathsVisitorSupervisor> mathsVisitor = CreateMathsVisitor();

   AutoFree<Graphics::ICameraSupervisor> camera = Graphics::CreateCamera(*instances, *mobiles, mainWindow->Renderer());

   AutoFree<Graphics::ISceneSupervisor> scene = Graphics::CreateScene(*instances, *camera);

   AutoFree<IKeyboardSupervisor> keyboard = CreateKeyboardSupervisor();

   AutoFree<Graphics::ISpriteSupervisor> sprites = Graphics::CreateSpriteSupervisor(mainWindow->Renderer());

   AutoFree<IConfigSupervisor> config = CreateConfig();

   Utilities utils(*installation);
   GuiStack gui(*publisher, *sourceCache, mainWindow->Renderer(), utils);
   Platform platform{ *os, *installation, mainWindow->Renderer(), *sourceCache, *debuggerWindow, *publisher, utils, gui, *keyboard, *config, *meshes, *instances, *mobiles, *sprites, *camera, *scene, *mathsVisitor, title };
   gui.platform = &platform;

   AutoFree<IApp> app(appFactory.CreateApp(platform));

   platform.renderer.AddOverlay(1000, &mathsVisitor->Overlay());

   mainWindow->Run(hInstanceLock, *app);
}

namespace Rococo
{
   Events::EventId UIInvoke::EvId()
   {
      static EventId invokeEvent = "ui.invoke"_event;
      return invokeEvent;
   }

   UIInvoke::UIInvoke(): Event(EvId())
   {

   }

   UIPopulate::UIPopulate() : Event(EvId())
   {

   }

   Events::EventId UIPopulate::EvId()
   {
      static EventId invokeEvent = "ui.populate"_event;
      return invokeEvent;
   }

   int M_Platorm_Win64_Main(HINSTANCE hInstance, IAppFactory& factory, cstr title, HICON hLarge, HICON hSmall)
   {
      rchar filename[1024];
      GetModuleFileNameA(nullptr, filename, 1024);
      for (char* p = filename; *p != 0; p++)
      {
         if (*p == '\\') *p = '#';
      }

      HANDLE hInstanceLock = CreateEventA(nullptr, TRUE, FALSE, filename);

      int err = GetLastError();
      if (err == ERROR_ALREADY_EXISTS)
      {
         SetEvent(hInstanceLock);

         if (IsDebuggerPresent())
         {
            ShowMessageBox(Windows::NoParent(), "Application is already running", filename, MB_ICONEXCLAMATION);
         }

         return err;
      }

      int errCode = 0;

      try
      {
         InitRococoWindows(hInstance, hLarge, hSmall, nullptr, nullptr);
         Main(hInstanceLock, factory, title);
      }
      catch (IException& ex)
      {
         rchar text[256];
         SafeFormat(text, 256, "%s crashed", title);
         OS::ShowErrorBox(NoParent(), ex, text);
         errCode = ex.ErrorCode();
      }

      CloseHandle(hInstanceLock);

      return errCode;
   }
}