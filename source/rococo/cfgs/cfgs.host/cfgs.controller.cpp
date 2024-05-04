#include <rococo.mvc.h>
#include <rococo.abstract.editor.h>
#include <rococo.strings.h>
#include <rococo.validators.h>
#include <rococo.properties.h>
#include <rococo.cfgs.h>
#include <rococo.sexml.h>
#include <rococo.io.h>
#include <rococo.os.h>
#include <rococo.functional.h>
#include <rococo.events.h>
#include <sexy.types.h>
#include <Sexy.S-Parser.h>
#include <stdio.h>

using namespace Rococo;
using namespace Rococo::Strings;
using namespace Rococo::Abedit;
using namespace Rococo::MVC;
using namespace Rococo::Validators;
using namespace Rococo::Reflection;
using namespace Rococo::Editors;
using namespace Rococo::Sex;
using namespace Rococo::Sex::SEXML;
using namespace Rococo::CFGS;
using namespace Rococo::Events;

namespace Rococo::CFGS
{
	IUI2DGridSlateSupervisor* Create2DGridControl(IAbstractEditorSupervisor& editor, Rococo::Editors::IUI2DGridEvents& eventHandler);
	ICFGSIntegratedDevelopmentEnvironmentSupervisor* Create_CFGS_IDE(IAbstractEditorSupervisor& editor, ICFGSDatabase& db, Rococo::Events::IPublisher& publisher, ICFGSControllerConfig& config);

	bool TryGetUserSelectedCFGSPath(OUT U8FilePath& path, IAbstractEditorSupervisor& editor);
	bool TryGetUserCFGSSavePath(OUT U8FilePath& path, Abedit::IAbstractEditorSupervisor& editor);
	void SetTitleWithFilename(IAbstractEditorSupervisor& editor, cstr filePath);
	void LoadDatabase(ICFGSDatabase& db, cstr filename, ICFGSLoader& loader);
	void SaveDatabase(ICFGSDatabase& db, Rococo::Sex::SEXML::ISEXMLBuilder& sb, ICFGArchiver& archiver);
}

namespace ANON
{
	enum class ELEMENT_CLASS
	{
		None,
		Light,
		Heavy,
		Transuranic
	};

	struct ELEMENT_CLASS_PAIR
	{
		ELEMENT_CLASS eValue;
		cstr sValue;
	};

	const ELEMENT_CLASS_PAIR ELEMENT_CLASS_pairs[] =
	{
		{ ELEMENT_CLASS::None, "None" },
		{ ELEMENT_CLASS::Light, "Light" },
		{ ELEMENT_CLASS::Heavy, "Heavy" },
		{ ELEMENT_CLASS::Transuranic, "Transuranic" },
	};

	ELEMENT_CLASS Parse(const OptionRef& ref)
	{
		for (auto i : ELEMENT_CLASS_pairs)
		{
			if (Eq(i.sValue, ref.value))
			{
				return i.eValue;
			}
		}

		return ELEMENT_CLASS::None;
	}

#define MARSHAL_OPTION(...)

	struct Element: IEstateAgent
	{
		HString name = "Uranium";
		int32 atomicNumber = 92;
		float atomicWeight = 238.0f;
		double valency = 6.01;
		bool isRadioactive = true;
		char fullDesc[256] = { 0 };
		ELEMENT_CLASS elementClass = ELEMENT_CLASS::Transuranic;

		void FormatDesc()
		{
			SafeFormat(fullDesc, "Element #%d '%s': weight %f, valency %g. %s", atomicNumber, name.c_str(), atomicWeight, valency, isRadioactive ? "<radioactive>" : "<stable>");
		}

		void AcceptVisit(IPropertyVisitor& visitor, IPropertyUIEvents& eventHandler) override
		{
			MARSHAL_PRIMITIVE(visitor, "an", "Atomic Number", eventHandler, REF atomicNumber, AllInt32sAreValid(), Int32Decimals());
			MARSHAL_STRING(visitor, "e1", "Element", eventHandler, REF name, 12);
			MARSHAL_PRIMITIVE(visitor, "aw", "Atomic Weight", eventHandler, REF atomicWeight, AllFloatsAreValid(), FloatDecimals());
			MARSHAL_PRIMITIVE(visitor, "va", "Valency", eventHandler, REF valency, AllDoublesAreValid(), DoubleDecimals());
			MARSHAL_PRIMITIVE(visitor, "ra", "Is Radioactive", eventHandler, REF isRadioactive, AllBoolsAreValid(), BoolFormatter());
			MARSHAL_OPTION(visitor, "ec", "Class", eventHandler, REF elementClass);

			visitor.VisitHeader("Desc", "Description", fullDesc);

			/*
				We have an example here, of a variable fullDesc that is dependent on the other variables, so when they are updated by the visitor
				we need to recompute it. We also need to signal that the editor/viewer for the dependent variable
			*/

			if (visitor.IsWritingToReferences())
			{
				FormatDesc();
				eventHandler.OnDependentVariableChanged("Desc", *this);
			}

			
			PropertyMarshallingStub stub { "cl", "Class", eventHandler };

			struct ElementClassEnumerator : IEnumDescriptor, IEnumVectorSupervisor
			{
				IEnumVectorSupervisor* CreateEnumList() override
				{
					return this;
				}

				// Returns the number of elements in the enumeration
				size_t Count() const override
				{
					return sizeof(ELEMENT_CLASS_pairs) / sizeof(ELEMENT_CLASS_PAIR);
				}

				// Populates the ith enum name. Returns true if i is within bounds
				bool GetEnumName(size_t i, Strings::IStringPopulator& populator) const override
				{
					if (i >= 0 && i < Count())
					{
						populator.Populate(ELEMENT_CLASS_pairs[i].sValue);
						return true;
					}

					return false;
				}

				// Populates the ith enum description or not if i is out of bounds. Returns true if i is within bounds
				bool GetEnumDescription(size_t i, Strings::IStringPopulator& populator) const override
				{
					if (i >= 0 && i < Count())
					{
						populator.Populate(ELEMENT_CLASS_pairs[i].sValue);
						return true;
					}

					return false;
				}

				void Free() override
				{

				}

				static IEnumDescriptor& Singleton()
				{
					static ElementClassEnumerator elementClassEnumerator;
					return elementClassEnumerator;
				}
			};
			
			if (!visitor.IsWritingToReferences())
			{
				HString classString;
				classString = ELEMENT_CLASS_pairs[(size_t)elementClass].sValue;
				OptionRef classRef{ classString };
				visitor.VisitOption(stub, IN REF classRef, 24, ElementClassEnumerator::Singleton());
			}
			else
			{
				HString classString;
				OptionRef classRef{ classString };
				visitor.VisitOption(stub, OUT REF classRef, 24, ElementClassEnumerator::Singleton());
				elementClass = Parse(classRef);
			}
		}
	};

	static auto evMenu = "AbeditMenuSelected"_event;

	struct CFGS_Controller: IMVC_ControllerSupervisor, IAbstractEditorMainWindowEventHandler, IPropertyVenue, IPropertyUIEvents, IUI2DGridEvents, ICFGSGuiEventHandler, ICFGArchiver, ICFGSLoader, ICFGSControllerConfig, Events::IObserver
	{
		AutoFree<Rococo::Events::IPublisherSupervisor> publisher;
		AutoFree<IAbstractEditorSupervisor> editor;
		AutoFree<ICFGSDatabaseSupervisor> db;
		AutoFree<IUI2DGridSlateSupervisor> gridSlate;
		AutoFree<ICFGSGuiSupervisor> gui;
		AutoFree<ICFGSIntegratedDevelopmentEnvironmentSupervisor> ide;
		
		bool terminateOnMainWindowClose = false;

		bool isRunning = true;

		Element element;

		enum class MenuItem : uint16
		{
			New = 3500,
			Load,
			Save,
			SaveAs,
			Compile,
			Exit
		};

		CFGS_Controller(IMVC_Host& _host, IMVC_View& view, cstr _commandLine): publisher(Events::CreatePublisher())
		{
			UNUSED(_commandLine);
			UNUSED(_host);

			Abedit::IAbstractEditorFactory* editorFactory = nullptr;
			view.Cast((void**)&editorFactory, "Rococo::Abedit::IAbstractEditorFactory");
			if (!editorFactory)
			{
				Throw(0, "%s: Expected an IAbstractEditorFactory to be non-NULL", __FUNCTION__);
			}

			EditorSessionConfig config;
			config.defaultPosLeft = -1;
			config.defaultPosTop = -1;
			config.defaultWidth = 1366;
			config.defaultHeight = 768;
			config.slateHasMenu = true;
			editor = editorFactory->CreateAbstractEditor(IN config, *this, *publisher);
			if (!editor)
			{
				Throw(0, "%s: Expected editorFactory->CreateAbstractEditor() to return a non-NULL pointer", __FUNCTION__);
			}

			SetTitleWithFilename(*editor, nullptr);

			db = CreateCFGSDatabase(*publisher);

			element.FormatDesc();

			gridSlate = Create2DGridControl(*editor, *this);
			gridSlate->ResizeToParent();

			gui = CreateCFGSGui(*db, gridSlate->DesignSpace(), *this);

			auto& props = editor->Properties();
			props.BuildEditorsForProperties(*this);

			ide = Create_CFGS_IDE(*editor, *db, *publisher, *this);

			auto& mainMenu = editor->Menu();
			auto& filePopup = mainMenu.AddPopup("&File");
			auto& buildPopup = mainMenu.AddPopup("&Build");
			filePopup.AddString("&New", (int32)MenuItem::New);
			filePopup.AddString("&Load...", (int32)MenuItem::Load);
			filePopup.AddString("&Save", (int32)MenuItem::Save);
			filePopup.AddString("&Save As...", (int32)MenuItem::SaveAs);
			filePopup.AddString("E&xit", (int32)MenuItem::Exit);
			buildPopup.AddString("C&ompile", (int32)MenuItem::Compile);

			editor->BringToFront();

			publisher->Subscribe(this, evMenu);
		}

		virtual ~CFGS_Controller()
		{
			publisher->Unsubscribe(this);
		}

		void OnEvent(Event& ev) override
		{
			if (ev == evMenu)
			{
				auto& args = As<Abedit::AbeditMenuEvent>(ev);

				auto item = static_cast<MenuItem>(args.menuId);

				switch (item)
				{
				case MenuItem::Load:
					OnSelectFileToLoad(*args.sender);
					break;
				case MenuItem::Save:
					OnSelectSave(*args.sender);
					break;
				case MenuItem::SaveAs:
					OnSelectFileToSave(*args.sender);
					break;
				case MenuItem::Exit:
					OnRequestToClose(*args.sender);
					break;
				case MenuItem::Compile:
					ide->Compile();
					break;
				default:
					OnContextMenuItemSelected(args.menuId, *args.sender);
					break;
				}
			}
		}

		void VisitVenue(IPropertyVisitor& visitor) override
		{
			element.AcceptVisit(visitor, *this);
		}

		void OnBooleanButtonChanged(IPropertyEditor& property) override
		{
			auto& props = editor->Properties();
			props.UpdateFromVisuals(property, *this);
		}

		void OnPropertyEditorLostFocus(Reflection::IPropertyEditor& property) override
		{
			auto& props = editor->Properties();
			props.UpdateFromVisuals(property, *this);
		}

		void CallArrayMethod(cstr arrayId, Function<void(IArrayProperty&)> callback) override
		{
			UNUSED(arrayId);
			UNUSED(callback);
		}

		void OnDeleteSection(cstr sectionId) override
		{
			UNUSED(sectionId);
		}

		void OnDependentVariableChanged(cstr propertyId, IEstateAgent& agent) override
		{
			editor->Properties().Refresh(propertyId, agent);
		}

		void Free() override
		{
			delete this;
		}

		bool IsRunning() const override
		{
			bool isVisible = editor->IsVisible();
			return isRunning && isVisible;
		}

		void OnExit() override
		{
			ide->OnExit();
		}

		void OnInitComplete() override
		{
			ide->OnInitComplete();
		}

		void GetErrorTitle(char* titleBuffer, size_t capacity) const
		{
			SafeFormat(titleBuffer, capacity, "%ls: Error!", GetCFGSAppTitle());
		}

		void OnRequestToClose(IAbeditMainWindow& sender) override
		{
			sender.Hide();	
			isRunning = false;
		}

		void OnSlateResized() override
		{
			if (gridSlate)
			{
				gridSlate->ResizeToParent();
			}
		}

		void TerminateOnMainWindowClose() override
		{
			terminateOnMainWindowClose = true;
		}

		void GridEvent_OnControlWheelRotated(int ticks, uint32 buttonFlags, Vec2i cursorPosition) override
		{
			UNUSED(cursorPosition);
			UNUSED(buttonFlags);

			double currentScale = gridSlate->ScaleFactor();
			double newScale = currentScale;

			const double multiplier_per_tick = 1.1;
			const double max_scale = 10.0;

			while (ticks > 0)
			{
				if (newScale < max_scale)
				{
					newScale *= multiplier_per_tick;
				}
				ticks--;
			}

			while (ticks < 0)
			{
				if (newScale > 1.0)
				{
					newScale /= multiplier_per_tick;
				}
				ticks++;
			}

			if (newScale < multiplier_per_tick)
			{
				newScale = 1.0;
			}

			gridSlate->SetScaleFactor(newScale);
		}

		void GridEvent_OnCursorMove(uint32 buttonFlags, Vec2i cursorPosition) override
		{
			UNUSED(buttonFlags);

			if (!gui->OnCursorMove(cursorPosition))
			{
				gridSlate->PreviewDrag(cursorPosition);
			}
		}

		void GridEvent_OnLeftButtonDown(uint32 buttonFlags, Vec2i cursorPosition) override
		{
			auto& popup = ide->DesignerSpacePopup();
			if (popup.IsVisible())
			{
				popup.Hide();
			}

			gridSlate->CaptureCursorInput();

			if (!gui->OnLeftButtonDown(buttonFlags, cursorPosition))
			{
				gridSlate->BeginDrag(cursorPosition);
			}
			
			gridSlate->QueueRedraw();
		}

		void GridEvent_OnLeftButtonUp(uint32 buttonFlags, Vec2i cursorPosition) override
		{
			gridSlate->ReleaseCapture();

			if (!gui->OnLeftButtonUp(buttonFlags, cursorPosition))
			{
				if (gridSlate->IsDragging())
				{
					gridSlate->EndDrag(cursorPosition);
				}
			}

			gridSlate->QueueRedraw();
		}

		void GridEvent_OnRightButtonUp(uint32 buttonFlags, Vec2i cursorPosition) override
		{
			gui->OnRightButtonUp(buttonFlags, cursorPosition);
		}

		void GridEvent_PaintForeground(IFlatGuiRenderer& gr) override
		{
			gui->Render(gr);
		}

		void GridEvent_PaintForegroundIndices(IFlatGuiRenderer& gr) override
		{
			gui->RenderIndices(gr);
		}

		void GridEvent_OnBackReleased() override
		{
			Events::EventArgs nullArgs;
			publisher->Post(nullArgs, "TryDeleteNode"_event);
		}

		void CFGSGuiEventHandler_OnCableLaying(const CableConnection& anchor)
		{
			UNUSED(anchor);
			gridSlate->QueueRedraw();
		}

		void CFGSGuiEventHandler_OnNodeHoverChanged(const NodeId& id) override
		{
			UNUSED(id);
			gridSlate->QueueRedraw();
		}

		void CFGSGuiEventHandler_OnNodeDragged(const NodeId& id) override
		{
			UNUSED(id);
			gridSlate->QueueRedraw();
		}

		void CFGSGuiEventHandler_OnNodeSelected(const NodeId& id) override
		{
			publisher->PostOneArg(id, "NodeSelected"_event);
		}

		void CFGSGuiEventHandler_PopupContextGUI(Vec2i cursorPosition) override
		{
			auto& popup = ide->DesignerSpacePopup();
			if (popup.IsVisible())
			{
				popup.Hide();
			}
			else
			{
				Vec2i desktopPosition = gridSlate->GetDesktopPositionFromGridPosition(cursorPosition);
				ide->DesignerSpacePopup().ShowAt(desktopPosition, gridSlate->DesignSpace().ScreenToWorld(cursorPosition));
			}
		}

		bool CFGSGuiEventHandler_IsConnectionPermitted(const CableConnection& anchor, const ICFGSSocket& target) const override
		{
			return ide->IsConnectionPermitted(anchor, target);
		}

		void CFGSGuiEventHandler_OnCableDropped(const CableDropped& crDropInfo) override
		{
			CableDropped dropInfo = crDropInfo;
			publisher->Post(dropInfo, "CableDropped"_event);
		}

		U8FilePath lastSavedSysPath;

		void Load(cstr filename)
		{
			try
			{
				ide->Clear();
				LoadDatabase(*db, filename, *this);
				gridSlate->QueueRedraw();
				CopyString(lastSavedSysPath.buf, U8FilePath::CAPACITY, filename);
				SetTitleWithFilename(*editor, lastSavedSysPath);
				editor->NavigationTree().RefreshGUI();
			}
			catch (Sex::ParseException& ex)
			{
				Rococo::Throw(ex.ErrorCode(), "Error loading %ls at line %d pos %d:\n\t%s", filename, ex.Start().y + 1, ex.Start().x + 1, ex.Message());
			}
			catch (IException& ex)
			{
				Rococo::Throw(ex.ErrorCode(), "Error loading %ls: %s", filename, ex.Message());
			}
		}

		void OnSelectFileToLoad(IAbeditMainWindow& sender)
		{
			UNUSED(sender);

			U8FilePath sysPath;
			if (TryGetUserSelectedCFGSPath(OUT sysPath, *editor))
			{
				Load(sysPath);
			}
		}

		void OnSelectFileToSave(IAbeditMainWindow& sender)
		{
			UNUSED(sender);

			UNUSED(sender);

			U8FilePath sysPath;
			if (TryGetUserCFGSSavePath(OUT sysPath, *editor))
			{
				lastSavedSysPath = sysPath;
				OnSelectSave(sender);
				SetTitleWithFilename(*editor, sysPath);		
			}
		}

		cstr ActiveFile() override
		{
			return lastSavedSysPath.buf[0] != 0 ? lastSavedSysPath.buf : nullptr;
		}

		bool TryLoadActiveFile(cstr filename) override
		{
			try
			{
				Load(filename);
				ide->OnLoaded(filename);
				return true;
			}
			catch (IException&)
			{
				return false;
			}
		}

		void OnSelectSave(IAbeditMainWindow& sender)
		{
			UNUSED(sender);

			if (lastSavedSysPath.buf[0] == 0)
			{
				return;
			}

			if (!EndsWith(lastSavedSysPath, ".cfgs.sexml"))
			{
				Rococo::Throw(0, "%s:\nOnly perimitted to save files with extension cfgs.sexml", lastSavedSysPath.buf);
			}

			U8FilePath backPath;
			Format(backPath, "%s.bak", lastSavedSysPath.buf);

			try
			{
				Rococo::OS::LoadBinaryFile(lastSavedSysPath,
					[&backPath](const uint8* fileData, size_t length)
					{
						Rococo::IO::SaveBinaryFile(backPath, fileData, length);
					}
				);
			}
			catch (IException& ex)
			{
				Throw(ex.ErrorCode(), "Error attempting to backup control flow graph to %s. %s", backPath.buf, ex.Message());
			}

			Rococo::OS::SaveSXMLBySysPath(lastSavedSysPath, [this](Rococo::Sex::SEXML::ISEXMLBuilder& sb)
				{
					SaveDatabase(*db, sb, *this);
				}
			);
		}

		void Archiver_OnSaveNavigation(Rococo::Sex::SEXML::ISEXMLBuilder& sb) override
		{
			ide->Navigation().SaveNavigation(sb);
		}

		void Loader_OnLoadNavigation(const Rococo::Sex::SEXML::ISEXMLDirective& directive) override
		{
			ide->Navigation().LoadNavigation(directive);
		}

		void OnContextMenuItemSelected(uint16 id, Rococo::Abedit::IAbeditMainWindow& sender)
		{
			UNUSED(sender);
			bool wasHandled = ide->Navigation().TryHandleContextMenuItem(id);
			UNUSED(wasHandled);
		}

		void DoHousekeeping(uint64 frameIndex) override
		{
			UNUSED(frameIndex);
			publisher->Deliver();
		}
	};
}

// Control-Flow Graph System
namespace Rococo::CFGS
{
	IMVC_ControllerSupervisor* CreateMVCControllerInternal(IMVC_Host& host, IMVC_View& view, cstr commandLine)
	{
		return new ANON::CFGS_Controller(host, view, commandLine);
	}

	cstr GetCFGSAppTitle()
	{
		return "Rococo Control-Graph Flow System Editor";
	}
}