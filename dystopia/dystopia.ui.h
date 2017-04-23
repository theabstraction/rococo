#ifndef DYSTOPIA_UI_H
#define DYSTOPIA_UI_H

namespace Dystopia
{
	ROCOCOAPI IUIPane
	{
		virtual Relay OnTimestep(const TimestepEvent& timestep) = 0;
		virtual Relay OnKeyboardEvent(const KeyboardEvent& ke) = 0;
		virtual Relay OnMouseEvent(const MouseEvent& me) = 0;
		virtual void RenderGui(IGuiRenderContext& grc) = 0;
		virtual void RenderObjects(IRenderContext& rc) = 0;
	};

	ROCOCOAPI IUIPaneSupervisor : public IUIPane
	{
		virtual void Free() = 0;
	};

	ROCOCOAPI IUIControlPane : IUIPaneSupervisor
	{
		virtual IIntent* PlayerIntent() = 0;
	};

	IUIControlPane* CreatePaneIsometric(Environment& e);
	IUIPaneSupervisor* CreatePaneStats(Environment& e);
	IUIPaneSupervisor* CreatePersonalInfoPanel(Environment& e);
	IUIPaneSupervisor* CreateJournalPane(Environment& e);

	IUIPaneSupervisor* CreateDialogBox(Environment& e, IEventCallback<GuiEventArgs>& _handler,
		cstr _title,
		cstr _message,
		cstr _buttons,
		Vec2i _span,
		int32 _retzone,
		int32 _hypzone);

	IUIPaneSupervisor* CreateContextMenu(Environment& e, Vec2i topLeft, ContextMenuItem newMenu[], IEventCallback<ContextMenuItem>& onClick);
	IUIPaneSupervisor* CreateInventoryPane(Environment& e);
   IUIPaneSupervisor* CreateCVPane(Environment& e);

	enum ID_PANE;

	ROCOCOAPI IUIPaneFactory
	{
		virtual void FreeInstance(ID_PANE id, IUIPaneSupervisor* pane) = 0;
		virtual IUIPaneSupervisor* GetOrCreatePane(ID_PANE id) = 0; // Either construct or retrieve pane
	};

	struct PaneBind
	{
		IUIPaneSupervisor& pane;
		ID_PANE id;
	};

	ROCOCOAPI IUIStack
	{
		virtual PaneBind PopTop() = 0;
		virtual PaneBind PushTop(ID_PANE id) = 0;
		virtual PaneBind PushTop(IUIPaneSupervisor* pane, ID_PANE id) = 0;
		virtual PaneBind Top() = 0;
	};

	ROCOCOAPI IUIStackSupervisor : public IUIStack
	{
		virtual void Free() = 0;
		virtual void OnCreated() = 0;
		virtual void SetFactory(IUIPaneFactory& factory) = 0;
		virtual IScene& Scene() = 0;
	};

	IUIStackSupervisor* CreateUIStack(Post::IPostbox& postbox);

	enum ID_PANE
	{
		ID_PANE_ISOMETRIC_GAME_VIEW,
		ID_PANE_STATS,
		ID_PANE_GENERIC_DIALOG_BOX,
		ID_PANE_GENERIC_CONTEXT_MENU,
		ID_PANE_INVENTORY_SELF,
      ID_PANE_CV,
		ID_PANE_PERSONAL_INFO,
		ID_PANE_JOURNAL
	};

	enum ID_CONTEXT_COMMAND : int32
	{
		ID_CONTEXT_COMMAND_NONE,
		ID_CONTEXT_COMMAND_EXAMINE,
		ID_CONTEXT_COMMAND_OPEN,
		ID_CONTEXT_COMMAND_PICKUP
	};
}

#endif