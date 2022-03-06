#pragma once

// The SexyStudio Widget API. This file should be kept free of OS dependent data structures and functions
// Widgets ineract with OS windows via IWindow interface on their Window method

#include <rococo.api.h>
#include <rococo.events.h>

namespace Rococo
{
	namespace Events
	{
		struct EventIdRef;
		class IPublisher;
	}

	namespace Windows
	{
		struct IWindow;
	}

	namespace Sex
	{
		struct ISExpression;
		typedef const ISExpression& cr_sex;

		bool IsCompound(cr_sex s);
	}
}

namespace Rococo::SexyStudio
{
	struct IGuiWidget;
	struct IWidgetSet;
	struct ISexyStudioEventHandler;

	using namespace Rococo::Events;
	using namespace Rococo::Windows;
	using namespace Rococo::Sex;

	struct IOSFont;

	struct WidgetContext
	{
		IPublisher& publisher;
		IOSFont& fontSmallLabel;
	};


	struct AtomicArg
	{
		bool Matches(cr_sex s, int index) const;
		fstring operator()(cr_sex s, int index) const;
	};

	class ParseKeyword
	{
		fstring keyword;
	public:
		ParseKeyword(cstr _keyword);
		bool Matches(cr_sex s, int index) const;
		fstring operator()(cr_sex s, int index) const;
	};

	extern ParseKeyword keywordNamespace;
	extern ParseKeyword keywordInterface;
	extern ParseKeyword keywordStruct;
	extern ParseKeyword keywordFunction;
	extern ParseKeyword keywordMacro;
	extern ParseKeyword keywordAlias;
	extern AtomicArg ParseAtomic;

	/* Two functions to allow manipulation of ISExpression's without having to include sexy headers*/

	int Len(cr_sex s);

	template<class ACTION, class FIRSTARG, class SECONDARG>
	inline bool match_compound(cr_sex s, int nMaxArgs, FIRSTARG a, SECONDARG b, ACTION action)
	{
		if (!IsCompound(s)) return false;
		if (Len(s) < 2) return false;
		if (Len(s) > nMaxArgs) return false;

		if (!a.Matches(s, 0)) return false;
		if (!b.Matches(s, 1)) return false;

		action(s, a(s, 0), b(s, 1));

		return true;
	}

	template<class ACTION, class FIRSTARG, class SECONDARG, class THIRDARG>
	inline bool match_compound(cr_sex s, int nMaxArgs, FIRSTARG a, SECONDARG b, THIRDARG c, ACTION action)
	{
		if (!IsCompound(s)) return false;
		if (Len(s) < 3) return false;
		if (Len(s) > nMaxArgs) return false;

		if (!a.Matches(s, 0)) return false;
		if (!b.Matches(s, 1)) return false;
		if (!c.Matches(s, 2)) return false;

		action(s, a(s, 0), b(s, 1), c(s, 2));

		return true;
	}

	uint64 GetFileLength(cstr filename);

	void AppendAncestorsToString(IWindow& window, StringBuilder& sb);
	void AppendAncestorsAndRectsToString(IWindow& window, StringBuilder& sb);
	void AppendDescendantsAndRectsToString(IWindow& window, StringBuilder& sb);

	ROCOCOAPI ISXYFile
	{

	};

	ROCOCOAPI ISXYArchetype
	{
		virtual cstr PublicName() const = 0;
		virtual int InputCount() const = 0;
		virtual int OutputCount() const = 0;
		virtual cstr InputType(int index) const = 0;
		virtual cstr OutputType(int index) const = 0;
		virtual cstr InputName(int index) const = 0;
		virtual cstr OutputName(int index) const = 0;
		virtual cstr SourcePath() const = 0;
		virtual int LineNumber() const = 0;
	};

	ROCOCOAPI ISXYFunction : ISXYArchetype{};

	ROCOCOAPI ISXYFactory
	{
		virtual cstr PublicName() const = 0;
		virtual int InputCount() const = 0;
		virtual void GetDefinedInterface(char* buf, size_t capacity) const = 0;
		virtual cstr InputType(int index) const = 0;
		virtual cstr InputName(int index) const = 0;
		virtual int LineNumber() const = 0;
		virtual cstr SourcePath() const = 0;
	};

	ROCOCOAPI ISXYInterface
	{
		virtual cstr Base() const = 0;
		virtual cstr PublicName() const = 0;
		virtual int AttributeCount() const = 0;
		virtual int MethodCount() const = 0;
		virtual cstr GetAttribute(int index) const = 0;
		virtual ISXYFunction& GetMethod(int index) = 0;
		virtual cstr SourcePath() const = 0;
		virtual cr_sex GetDefinition() const = 0;
	};

	struct SXYMethodArgument
	{
		cstr type;
		cstr name;
	};

	struct SXYField
	{
		cstr type;
		cstr name;
	};

	ROCOCOAPI ISXYLocalType
	{
		virtual int FieldCount() const = 0;
		virtual SXYField GetField(int index) const = 0;
		virtual cstr LocalName() const = 0;
		virtual cstr SourcePath() const = 0;
		virtual int LineNumber() const = 0;
	};

	ROCOCOAPI ISXYType
	{
		virtual cstr PublicName() const = 0;
		virtual ISXYLocalType* LocalType() = 0;
	};

	ROCOCOAPI ISXYPublicFunction
	{
		virtual cstr PublicName() const = 0;
		virtual ISXYFunction* LocalFunction() = 0;
	};

	ROCOCOAPI ISxyNamespace
	{
		virtual int AliasCount() const = 0;
		virtual cstr GetNSAliasFrom(int index) const = 0;
		virtual	cstr GetNSAliasTo(int index) const = 0;
		virtual	cstr GetAliasSourcePath(int index) const = 0;
		virtual ISXYArchetype& GetArchetype(int index) = 0;
		virtual ISXYInterface & GetInterface(int index) = 0;
		virtual ISXYType& GetType(int index) = 0;
		virtual ISXYPublicFunction& GetFunction(int index) = 0;
		virtual ISXYFactory& GetFactory(int index) = 0;
		virtual ISxyNamespace* GetParent() = 0;
		virtual int ArchetypeCount() const = 0;
		virtual int FactoryCount() const = 0;
		virtual int FunctionCount() const = 0;
		virtual int InterfaceCount() const = 0;
		virtual int TypeCount() const = 0;
		virtual int SubspaceCount() const = 0;
		virtual ISxyNamespace& operator[] (int index) = 0;
		virtual cstr Name() = 0;
		virtual ISxyNamespace& Update(cstr subspace, cr_sex src) = 0;
		virtual void UpdateArchetype(cstr name, cr_sex sDef, ISXYFile& file) = 0;
		virtual void UpdateFactory(cstr name, cr_sex sFactoryDef, ISXYFile& file) = 0;
		virtual void UpdateInterface(cstr name, cr_sex sInterfaceDef, ISXYFile& file) = 0;
		virtual void UpdateInterfaceViaDefinition(cstr name, cr_sex sInterfaceDef, ISXYFile& file) = 0;
		virtual void UpdateMacro(cstr name, cr_sex sMacroDef, ISXYFile& file) = 0;
		virtual void SortRecursive() = 0;
		virtual void AliasFunction(cstr localName, ISXYFile& file, cstr publicName) = 0;
		virtual void AliasStruct(cstr localName, ISXYFile& file, cstr publicName) = 0;
		virtual void AliasNSREf(cstr publicName, cr_sex sAliasDef, ISXYFile& file) = 0;
		virtual int EnumCount() const = 0;
		virtual cstr GetEnumName(int index) const = 0;
		virtual cstr GetEnumValue(int index) const = 0;
		virtual cstr GetEnumSourcePath(int index) const = 0;
	};

	void AppendFullName(ISxyNamespace& ns, struct StringBuilder& sb);

	ROCOCOAPI ISexyFieldEnumerator
	{
		virtual void OnField(cstr fieldName) = 0;
		virtual void OnHintFound(cstr hint) = 0;
	};

	ROCOCOAPI ISolution
	{
		virtual cstr GetContentFolder() = 0;
		virtual cstr GetDeclarationPathForInclude(cstr includeName, int& priority) = 0;
		virtual cstr GetDeclarationPathForImport(cstr packageName, int& priority) = 0;
		virtual cstr GetPackagePingPath(cstr packageName) = 0;
		virtual cstr GetPackageRoot() = 0;
		virtual cstr GetPackageSourceFolder(cstr packagePath) = 0;
		virtual void SetContentFolder(cstr path) = 0;
	};

	ROCOCOAPI ISexyDatabase
	{
		virtual void Clear() = 0;
		virtual bool EnumerateVariableAndFieldList(cr_substring variable, cstr typeString, ISexyFieldEnumerator& fieldEnumerator) = 0;
		virtual ISXYInterface* FindInterface(cstr typeString) = 0;
		virtual void FocusProject(cstr projectFilePath) = 0;
		virtual void ForEachAutoCompleteCandidate(cr_substring prefix, ISexyFieldEnumerator& fieldEnumerator) = 0;
		virtual void GetHintForCandidate(cr_substring prefix, char args[1024]) = 0;
		virtual ISxyNamespace& GetRootNamespace() = 0;
		virtual void PingPathToSysPath(cstr pingPath, U8FilePath& sysPath) = 0;
		virtual void Sort() = 0;
		virtual void UpdateFile_SXY(cstr fullpathToSxy) = 0;
		virtual void UpdateFile_SXY_PackedItem(cstr data, int32 length, cstr path) = 0;

		virtual ISolution& Solution() = 0;
	};

	void BuildDatabaseFromProject(ISexyDatabase& database, cr_sex sProjectRoot, cstr projectPath);

	typedef int64 ID_TREE_ITEM;

	struct SourceAndLine
	{
		cstr SourcePath;
		int LineNumber;
	};

	ROCOCOAPI ISourceTree
	{
		virtual void Add(ID_TREE_ITEM item, cstr text, int lineNumber) = 0;
		virtual void Clear() = 0;
		virtual SourceAndLine Find(ID_TREE_ITEM item) const = 0;
		virtual void Free() = 0;
	};

	void PopulateTreeWithPackages(cstr packageFolder, ISexyDatabase& database);

	ROCOCOAPI ISexyDatabaseSupervisor : ISexyDatabase
	{
		virtual void Free() = 0;
		virtual void SetContentPath(cstr contentFolder) = 0;
	};

	ISexyDatabaseSupervisor* CreateSexyDatabase();

	struct WaitCursorSection
	{
		WaitCursorSection();
		~WaitCursorSection();
	};

	ROCOCOAPI ILayout
	{
		virtual void Layout(IGuiWidget& widget, GuiRect & rect) = 0;
		virtual void Free() = 0;
	};

	ROCOCOAPI ILayoutSet
	{
		virtual void Add(ILayout * d) = 0;
		virtual void Free() = 0;
		virtual void Layout(IGuiWidget& widget) = 0;
	};

	ROCOCOAPI IGuiWidget
	{
		/* Reshape this control and its children according to the layout controls.
		   parents should call this when they are resized 
		 */
		virtual void Layout() = 0;

		// add a modifier - to modify the way this widget is layed out
		virtual void AddLayoutModifier(ILayout* l) = 0;

		// Release the memory associated with this widget, invalidating it.
		virtual void Free() = 0;

		// Modify visibility of the widget
		virtual void SetVisible(bool isVisible) = 0;

		// returns the set of children if it can possess children, otherwise returns nullptr
		virtual IWidgetSet* Children() = 0;

		// Get the OS or other implementation of this widget
		virtual IWindow& Window() = 0;

		operator IWindow& () { return Window(); }
	};

	namespace Widgets
	{
		void AnchorToParentLeft(IGuiWidget& widget, int pixelBorder);
		void AnchorToParentRight(IGuiWidget& widget, int pixelBorder);
		void AnchorToParentTop(IGuiWidget& widget, int pixelBorder);
		void AnchorToParent(IGuiWidget& widget, int leftBorder, int topBorder, int rightBorder, int bottomBorder);
		void ExpandBottomFromTop(IGuiWidget& widget, int pixels);
		void ExpandLeftFromRight(IGuiWidget& widget, int pixels);

		void ExpandToFillParentSpace(IWindow& window);

		Vec2i GetParentSpan(IWindow& window);
		GuiRect GetScreenRect(IWindow& window);
		Vec2i GetSpan(IWindow& widget);
		GuiRect MapScreenToWindowRect(const GuiRect& rect, IWindow& window);
		void SetWidgetPosition(IWindow& widget, const GuiRect& rect);
		void Maximize(IWindow& window);
		void Minimize(IWindow& window);

		void SetSpan(IWindow& window, int32 dx, int32 dy);
		void SetText(IWindow& window, const char* text);
	}

	ROCOCOAPI IWidgetSet
	{
		// Add a widget to the widget set, when the set owner is done it will call Free() on the widget
		virtual void Add(IGuiWidget * widget) = 0;
		// the parent window to which this widget set belongs
		virtual IWindow& Parent() = 0;
		// IGuiWidget* iterator begin()
		virtual IGuiWidget** begin() = 0;
		// IGuiWidget* iterator end()
		virtual IGuiWidget** end() = 0;
		// Get the publisher associated with this widget set
		virtual WidgetContext& Context() = 0;
	};

	enum class EFolderIcon
	{
		FOLDER_CLOSED = 0,
		FOLDER_OPEN = 1
	};

	ROCOCOAPI IGuiTreeRenderer
	{
		virtual void RenderItem() = 0;
	};

	struct TreeItemInfo
	{
		ID_TREE_ITEM idItem;
	};

	ROCOCOAPI IPopupMenuBuilder
	{
		virtual void AppendMenuItem(uint16 id, cstr text) = 0;
	};

	ROCOCOAPI IPopupMenu: IPopupMenuBuilder
	{
		virtual void ClearPopupMenu() = 0;
		virtual void ShowPopupMenu(Vec2i pos) = 0;
	};

	ROCOCOAPI IGuiTree : IGuiWidget
	{
		virtual ID_TREE_ITEM AppendItem(ID_TREE_ITEM branch) = 0;
		virtual void Clear() = 0;
		virtual void Collapse() = 0;
		virtual void EnableExpansionIcons(bool enable) = 0;
		virtual void ExpandAt(ID_TREE_ITEM idItem) = 0;
		virtual void SetContext(ID_TREE_ITEM idItem, uint64 contextId) = 0;
		virtual void SetItemExpandedImage(ID_TREE_ITEM hItem, int imageIndex) = 0;
		virtual void SetItemText(ID_TREE_ITEM hItem, cstr text) = 0;
		virtual void SetItemImage(ID_TREE_ITEM hItem, int imageIndex) = 0;
		virtual void EnumerateChildren(ID_TREE_ITEM id, IEventCallback<TreeItemInfo>& cb) const = 0;
		virtual void GetText(char* buffer, size_t capacity, ID_TREE_ITEM id) = 0;

		// Define the list of image identifiers, each is an int32, example SetImageList(2, ID_FOLDER_CLOSED, ID_FOLDER_OPEN, ID_FILE)
		virtual void SetImageList(uint32 nItems, ...) = 0;
		virtual Windows::IWindow& TreeWindow() = 0;
		virtual IPopupMenu& PopupMenu() = 0;
	};

	ROCOCOAPI IGuiTreeEvents
	{
		virtual void OnItemContextClick(IGuiTree& tree, ID_TREE_ITEM hItem, Vec2i pos) = 0;
		virtual void OnCommand(uint16 id) = 0;
	};

	struct TreeStyle
	{
		bool hasCheckBoxes = false;
		bool hasButtons = false;
		bool hasLines = false;
	};

	IGuiTree* CreateTree(IWidgetSet& widgets, const TreeStyle& style, IGuiTreeEvents& eventHandler, IGuiTreeRenderer* customRenderer = nullptr);

	ISourceTree* CreateSourceTree();

	ROCOCOAPI IGuiWidgetEditor : IGuiWidget
	{
		virtual cstr Name() const = 0;
		virtual void SetName(cstr name) = 0;
	};

	ROCOCOAPI IAsciiStringEditor : IGuiWidgetEditor
	{
		virtual void Bind(char* buffer, size_t capacityBytes) = 0;
		virtual IWindow& OSEditor() = 0;
		virtual void SetCharacterUpdateEvent(EventIdRef id) = 0;
		virtual void SetMouseMoveEvent(EventIdRef id) = 0;
		virtual void SetText(cstr text) = 0;
		virtual void SetUpdateEvent(EventIdRef id) = 0;
		virtual cstr Text() const = 0;
	};

	ROCOCOAPI IDropDownList : IGuiWidgetEditor
	{
		virtual IWindow& OSDropDown() = 0;
		virtual void AppendItem(cstr text) = 0;	
		virtual void ClearItems() = 0;
	};

	ROCOCOAPI IListWidget : IGuiWidgetEditor
	{
		virtual IWindow & OSList() = 0;
		virtual void AppendItem(cstr text) = 0;
		virtual void ClearItems() = 0;
	};

	ROCOCOAPI IFloatingListWidget : IGuiWidget
	{
		virtual IWindow & OSList() = 0;
		virtual void AppendItem(cstr text) = 0;
		virtual void ClearItems() = 0;
		virtual void RenderWhileMouseInEditorOrList(IWindow& editorWindow) = 0;
		virtual void SetDoubleClickEvent(EventIdRef id) = 0;
	};

	cstr FindDot(cstr s);

	IFloatingListWidget* CreateFloatingListWidget(IWindow& window, WidgetContext& wc);

	ROCOCOAPI IFilePathEditor : IGuiWidgetEditor
	{
		virtual void Bind(U8FilePath& path, uint32 maxChars) = 0;
		virtual void SetUpdateEvent(EventIdRef id) = 0;
	};

	ROCOCOAPI IVariableList : IGuiWidget
	{
		virtual IAsciiStringEditor* AddAsciiEditor() = 0;
		virtual IDropDownList* AddDropDownList(bool addTextEditor) = 0;
		virtual IFilePathEditor* AddFilePathEditor() = 0;
		virtual IListWidget* AddListWidget() = 0;

		// Gives number of pixels from LHS of the list to the editor column
		virtual int NameSpan() const = 0;
	};

	IVariableList* CreateVariableList(IWidgetSet& widgets);

	ROCOCOAPI IToolbar : public IGuiWidget
	{
		// Tells the toolbar that the specified widget will manage its own layout
		// Otherwise the toolbar lays out the widget to the right of its predecessor
		virtual void SetManualLayout(IGuiWidget * widget) = 0;

		// The spacing between each widget in the toolbar
		virtual void SetSpacing(int32 firstBorder, int32 widgetSpacing) = 0;
	};

	ROCOCOAPI IIDEFrame
	{
		virtual IWindow & Window() = 0;
		virtual void SetVisible(bool isVisible) = 0;
		virtual IWidgetSet& Children() = 0;
		operator IWindow& () { return Window(); };
		// Update child geometry. This is issued when the control is resized and also by calling SetVisible
		virtual void LayoutChildren() = 0;
		virtual void SetProgress(float progressPercent, cstr bannerText) = 0;
		virtual ISexyStudioEventHandler& Events() = 0;
	};

	ROCOCOAPI IIDEFrameSupervisor : IIDEFrame
	{
		virtual void Free() = 0;
		virtual void SetCloseEvent(const EventIdRef& evClose) = 0;
		virtual void SetResizeEvent(const EventIdRef& evResize) = 0;
	};

	IIDEFrameSupervisor* CreateMainIDEFrame(WidgetContext& context, IWindow& topLevelWindow, ISexyStudioEventHandler& evHandler);

	ROCOCOAPI IButtonWidget : IGuiWidget
	{
	};

	ILayoutSet* CreateLayoutSet();

	IButtonWidget* CreateButtonByResource(WidgetContext& context, IWidgetSet& widgets, int16 resourceId, Rococo::Events::EventIdRef evOnClick);

	struct ButtonClickContext
	{
		IButtonWidget* sourceWidget;
	};

	ROCOCOAPI IWidgetSetSupervisor : IWidgetSet
	{
		virtual void Free() = 0;
	};

	IWidgetSetSupervisor* CreateDefaultWidgetSet(Rococo::Windows::IWindow& parent, WidgetContext& context);

	ROCOCOAPI ISplitScreen : public IGuiWidget
	{
		virtual ISplitScreen* GetFirstHalf() = 0;
		virtual ISplitScreen* GetSecondHalf() = 0;
		virtual GuiRect GetRect() = 0;
		virtual void SplitIntoColumns(int32 firstSpan) = 0;
		virtual void SplitIntoRows(int32 firstSpan) = 0;
		virtual void Merge() = 0;
		virtual void SetBackgroundColour(RGBAb colour) = 0;
	};

	struct TooltipArgs: Rococo::Events::EventArgs
	{
		cstr text;
		OS::ticks hoverTime;
		bool useSingleLine;
		RGBAb textColour;
		RGBAb backColour;
		RGBAb borderColour;
	};

	ISplitScreen* CreateSplitScreen(IWidgetSet& widgets);

	ROCOCOAPI ITab
	{
		virtual int64 AddRef() = 0;
		virtual int64 Release() = 0;
		virtual cstr Name() const = 0;
		virtual cstr Tooltip() const = 0;
		virtual void SetName(cstr name) = 0;
		virtual void SetTooltip(cstr tooltip) = 0;
		virtual void Activate() = 0;
		virtual void Deactivate() = 0;
		virtual void Layout() = 0;
		virtual IWidgetSet& Children() = 0;
	};

	ROCOCOAPI ITabSplitter : IGuiWidget
	{
		virtual ITab& AddTab() = 0;
	};

	ITabSplitter* CreateTabSplitter(IWidgetSet& widgets);

	struct ColourSet
	{
		RGBAb bkColor;
		RGBAb edgeColor;
		RGBAb txColor;
	};

	struct Theme
	{
		ColourSet normal;
		ColourSet lit;
	};

	ROCOCOAPI ITheme
	{
		// Get a mutable ref to the theme, allowing modification of a theme
		virtual Theme & GetTheme() = 0;
		virtual void Free() = 0;
	};

	// Use a theme, if the name is unknown a default theme is used
	// call free to cancel application of the theme
	ITheme* UseNamedTheme(cstr name, IPublisher& publisher);

	Theme GetTheme(IPublisher& publisher);

	struct ThemeInfo
	{
		cstr name;
		const Theme& theme;
	};

	void EnumerateThemes(IEventCallback<const ThemeInfo>& cb);

	// uses TEventArg<Theme> 
	extern EventIdRef evGetTheme;
}
