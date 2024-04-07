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

namespace Rococo::CFGS
{
	IUI2DGridSlateSupervisor* Create2DGridControl(IAbstractEditorSupervisor& editor, Rococo::Editors::IUI2DGridEvents& eventHandler);
	ICFGSIntegratedDevelopmentEnvironmentSupervisor* Create_CFGS_IDE(IAbstractEditorSupervisor& editor, ICFGSDatabase& db);
	ICFGSMessagingSupervisor* CreateMessagingService(ICFGSPropertyChangeHandler& changeHandler);

	bool TryGetUserSelectedCFGSPath(OUT WideFilePath& path, IAbstractEditorSupervisor& editor);
	bool TryGetUserCFGSSavePath(OUT WideFilePath& path, Abedit::IAbstractEditorSupervisor& editor);
	void SetTitleWithFilename(IAbstractEditorSupervisor& editor, const wchar_t* filePath);
	void LoadDatabase(ICFGSDatabase& db, const wchar_t* filename, CFGS::ICFGSLoader& loader);
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

	struct CFGS_Controller: IMVC_ControllerSupervisor, IAbstractEditorMainWindowEventHandler, IPropertyVenue, IPropertyUIEvents, IUI2DGridEvents, CFGS::ICFGSGuiEventHandler, CFGS::ICFGArchiver, CFGS::ICFGSLoader
	{
		AutoFree<IAbstractEditorSupervisor> editor;
		AutoFree<CFGS::ICFGSMessagingSupervisor> messaging;
		AutoFree<CFGS::ICFGSDatabaseSupervisor> db;
		AutoFree<IUI2DGridSlateSupervisor> gridSlate;
		AutoFree<CFGS::ICFGSGuiSupervisor> gui;
		AutoFree<CFGS::ICFGSIntegratedDevelopmentEnvironmentSupervisor> ide;
		
		bool terminateOnMainWindowClose = false;

		bool isRunning = true;

		Element element;

		struct ChangeHandler : CFGS::ICFGSPropertyChangeHandler
		{
			CFGS::ICFGSIntegratedDevelopmentEnvironmentSupervisor* ide = nullptr;

			void OnPropertyChanged(Reflection::IPropertyEditor& property) override
			{
				if (ide)
				{
					ide->OnPropertyChanged(property);
				}
			}
		} changeHandler;

		CFGS_Controller(IMVC_Host& _host, IMVC_View& view, cstr _commandLine)
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
			editor = editorFactory->CreateAbstractEditor(IN config, *this);
			if (!editor)
			{
				Throw(0, "%s: Expected editorFactory->CreateAbstractEditor() to return a non-NULL pointer", __FUNCTION__);
			}

			CFGS::SetTitleWithFilename(*editor, nullptr);

			messaging = CFGS::CreateMessagingService(changeHandler);

			db = CFGS::CreateCFGSDatabase(*messaging);

			element.FormatDesc();

			gridSlate = CFGS::Create2DGridControl(*editor, *this);
			gridSlate->ResizeToParent();

			gui = CFGS::CreateCFGSGui(*db, gridSlate->DesignSpace(), *this);

			auto& props = editor->Properties();
			props.BuildEditorsForProperties(*this);

			ide = CFGS::Create_CFGS_IDE(*editor, *db);

			editor->BringToFront();

			changeHandler.ide = ide;
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

		void OnWindowlessMessage(uint32 messageId) override
		{
			if (messaging->IsDBHousekeeping(messageId))
			{
				db->DoHouseKeeping();
			}
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

		void GetErrorTitle(char* titleBuffer, size_t capacity) const
		{
			SafeFormat(titleBuffer, capacity, "%ls: Error!", CFGS::GetCFGSAppTitle());
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

		void CFGSGuiEventHandler_OnCableLaying(const CFGS::CableConnection& anchor)
		{
			UNUSED(anchor);
			gridSlate->QueueRedraw();
		}

		void CFGSGuiEventHandler_OnNodeHoverChanged(const CFGS::NodeId& id) override
		{
			UNUSED(id);
			gridSlate->QueueRedraw();
		}

		void CFGSGuiEventHandler_OnNodeDragged(const CFGS::NodeId& id) override
		{
			UNUSED(id);
			gridSlate->QueueRedraw();
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

		bool CFGSGuiEventHandler_IsConnectionPermitted(const Rococo::CFGS::CableConnection& anchor, const Rococo::CFGS::ICFGSSocket& target) const override
		{
			return ide->IsConnectionPermitted(anchor, target);
		}

		WideFilePath lastSavedSysPath;

		void OnSelectFileToLoad(IAbeditMainWindow& sender) override
		{
			UNUSED(sender);

			WideFilePath sysPath;
			if (CFGS::TryGetUserSelectedCFGSPath(OUT sysPath, *editor))
			{
				try
				{
					CFGS::LoadDatabase(*db, sysPath, *this);
					gridSlate->QueueRedraw();
					lastSavedSysPath = sysPath;
					CFGS::SetTitleWithFilename(*editor, sysPath);
					editor->NavigationTree().RefreshGUI();
				}
				catch (Sex::ParseException& ex)
				{
					Rococo::Throw(ex.ErrorCode(), "Error loading %ls at line %d pos %d:\n\t%s", sysPath.buf, ex.Start().y + 1, ex.Start().x + 1, ex.Message());
				}
				catch (IException& ex)
				{
					Rococo::Throw(ex.ErrorCode(), "Error loading %ls: %s", sysPath.buf, ex.Message());
				}
			}
		}

		void OnSelectFileToSave(IAbeditMainWindow& sender) override
		{
			UNUSED(sender);

			UNUSED(sender);

			WideFilePath sysPath;
			if (CFGS::TryGetUserCFGSSavePath(OUT sysPath, *editor))
			{
				lastSavedSysPath = sysPath;
				OnSelectSave(sender);
				CFGS::SetTitleWithFilename(*editor, sysPath);		
			}
		}

		void OnSelectSave(IAbeditMainWindow& sender) override
		{
			UNUSED(sender);

			if (lastSavedSysPath.buf[0] == 0)
			{
				return;
			}

			if (!EndsWith(lastSavedSysPath, L".cfgs.sxml"))
			{
				Rococo::Throw(0, "%ls:\nOnly perimitted to save files with extension cfgs.sxml", lastSavedSysPath.buf);
			}

			WideFilePath wBackPath;
			Format(wBackPath, L"%ls.bak", lastSavedSysPath.buf);

			try
			{
				Rococo::OS::LoadBinaryFile(lastSavedSysPath,
					[&wBackPath](const uint8* fileData, size_t length)
					{
						Rococo::IO::SaveBinaryFile(wBackPath, fileData, length);
					}
				);
			}
			catch (IException& ex)
			{
				Throw(ex.ErrorCode(), "Error attempting to backup control flow graph to %ls. %s", wBackPath.buf, ex.Message());
			}

			Rococo::OS::SaveSXMLBySysPath(lastSavedSysPath, [this](Rococo::Sex::SEXML::ISEXMLBuilder& sb)
				{
					CFGS::SaveDatabase(*db, sb, *this);
				}
			);
		}

		void Archiver_OnSaveNavigation(Rococo::Sex::SEXML::ISEXMLBuilder& sb) override
		{
			ide->SaveNavigation(sb);
		}

		void Loader_OnLoadNavigation(const Rococo::Sex::SEXML::ISEXMLDirective& directive) override
		{
			ide->LoadNavigation(directive);
		}

		void OnContextMenuItemSelected(uint16 id, Rococo::Abedit::IAbeditMainWindow& sender) override
		{
			UNUSED(sender);
			bool wasHandled = ide->TryHandleContextMenuItem(id);
			UNUSED(wasHandled);
		}

		void OnRegenerate(Rococo::Abedit::IAbeditMainWindow& sender) override
		{
			UNUSED(sender);
			ide->RegenerateProperties();
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

	const wchar_t* GetCFGSAppTitle()
	{
		return L"Rococo Control-Graph Flow System Editor";
	}
}