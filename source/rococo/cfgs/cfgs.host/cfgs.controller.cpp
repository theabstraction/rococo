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
	bool TryGetUserSelectedCFGSPath(OUT WideFilePath& path, IAbstractEditorSupervisor& editor);
	void SetTitleWithFilename(IAbstractEditorSupervisor& editor, const wchar_t* filePath);
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

	struct CFGS_Controller: IMVC_ControllerSupervisor, IAbstractEditorMainWindowEventHandler, IPropertyVenue, IPropertyUIEvents, IUI2DGridEvents, CFGS::ICFGSGuiEventHandler
	{
		AutoFree<IAbstractEditorSupervisor> editor;
		AutoFree<IUI2DGridSlateSupervisor> gridSlate;
		AutoFree<CFGS::ICFGSGui> gui;
		AutoFree<CFGS::ICFGSDatabaseSupervisor> db;

		bool terminateOnMainWindowClose = false;

		bool isRunning = true;

		Element element;

		CFGS_Controller(IMVC_Host& _host, IMVC_View& view, cstr _commandLine)
		{
			UNUSED(_commandLine);
			UNUSED(_host);

			db = CFGS::CreateCFGSTestSystem();

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

			element.FormatDesc();

			gridSlate = CFGS::Create2DGridControl(*editor, *this);
			gridSlate->ResizeToParent();

			gui = CFGS::CreateCFGSGui(*db, gridSlate->DesignSpace(), *this);

			auto& props = editor->Properties();
			props.BuildEditorsForProperties(*this);
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

		void GridEvent_OnControlWheelRotated(int ticks, uint32 gridEventWheelFlags, Vec2i cursorPosition) override
		{
			UNUSED(cursorPosition);
			UNUSED(gridEventWheelFlags);

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

		void GridEvent_OnCursorMove(uint32 gridEventWheelFlags, Vec2i cursorPosition) override
		{
			UNUSED(gridEventWheelFlags);

			if (!gui->OnCursorMove(cursorPosition))
			{
				gridSlate->PreviewDrag(cursorPosition);
			}
		}

		void GridEvent_OnLeftButtonDown(uint32 gridEventWheelFlags, Vec2i cursorPosition) override
		{
			UNUSED(gridEventWheelFlags);

			gridSlate->CaptureCursorInput();

			if (!gui->OnLeftButtonDown(gridEventWheelFlags, cursorPosition))
			{
				gridSlate->BeginDrag(cursorPosition);
			}
			else
			{
				gridSlate->QueueRedraw();
			}
		}

		void GridEvent_OnLeftButtonUp(uint32 gridEventWheelFlags, Vec2i cursorPosition) override
		{
			UNUSED(gridEventWheelFlags);
			gridSlate->ReleaseCapture();

			if (!gui->OnLeftButtonUp(gridEventWheelFlags, cursorPosition))
			{
				gridSlate->EndDrag(cursorPosition);
			}
		}

		void GridEvent_PaintForeground(IFlatGuiRenderer& gr) override
		{
			gui->Render(gr);
		}

		void GridEvent_PaintForegroundIndices(IFlatGuiRenderer& gr) override
		{
			gui->RenderIndices(gr);
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

		UniqueIdHolder AsUniqueId(const ISEXMLAttributeValue& a)
		{
			fstring id = AsString(a).ToFString();
			
			UniqueIdHolder uniqueId;
			if (sscanf_s(id, "%llx %llx", &uniqueId.iValues[0], &uniqueId.iValues[1]) != 2)
			{
				Throw(a.S(), "Expecting two 64-bit hexademical strings. Format is (Id \"<int64-hex> <int64-hex>\")");
			}

			return uniqueId;
		}

		template<class T>
		T AsId(const ISEXMLAttributeValue& a)
		{
			UniqueIdHolder id = AsUniqueId(a);
			return T{ id };
		}

		void OnLoadGraphNode(const ISEXMLDirective& node)
		{
			auto& type = AsString(node["Type"]);
			double xPos = AsAtomicDouble(node["XPos"]);
			double yPos = AsAtomicDouble(node["YPos"]);
			auto nodeId = AsId<CFGS::NodeId>(node["Id"]);

			auto& nb = db->Nodes().Builder().AddNode(type.c_str(), {xPos, yPos}, nodeId );

			auto nChildren = node.NumberOfChildren();
			for (size_t i = 0; i < nChildren; i++)
			{
				auto& socket = node[i];
				if (!Strings::Eq(socket.FQName(), "Socket"))
				{
					Throw(node.S(), "Expecting (Socket ...)");
				}

				auto& socketType = SEXML::AsString(socket["Type"]);
				auto& socketClass = SEXML::AsString(socket["Class"]);
				auto& socketLabel = SEXML::AsString(socket["Label"]);
				auto socketId = AsId<CFGS::SocketId>(socket["Id"]);

				CFGS::SocketClass sclass;
				if (CFGS::TryParse(OUT sclass, socketClass.c_str()))
				{
					nb.AddSocket(socketType.c_str(), sclass, socketLabel.c_str(), socketId);
				}
				else
				{
					Throw(socketClass.S(), "Could not parse %s as a SocketClass enum.", socketClass.c_str());
				}
			}
		}

		void OnLoadGraphCable(const ISEXMLDirective& cable)
		{
			auto startNodeId   = AsId<CFGS::NodeId>(cable["StartNode"]);
			auto startSocketId = AsId<CFGS::SocketId>(cable["StartSocket"]);
			auto endNodeId     = AsId<CFGS::NodeId>(cable["EndNode"]);
			auto endSocketId   = AsId<CFGS::SocketId>(cable["EndSocket"]);

			db->Cables().Add(startNodeId, startSocketId, endNodeId, endSocketId);
		}

		void OnLoadGraphSXML(const ISEXMLDirectiveList& topLevelDirectives)
		{
			size_t startIndex = 0;
			auto& header = GetDirective(topLevelDirectives, "ControlFlowGraphSystem", IN OUT startIndex);		
			auto& format = SEXML::AsString(header["FileFormat"]);
			if (!Strings::Eq(format.c_str(), "SXML"))
			{
				Throw(format.S(), "Expecting (FileFormat SEXML) in the ControlFlowGraphSystem directive");
			}

			auto& version = SEXML::AsString(header["Version"]);
			if (!Strings::Eq(version.c_str(), "1.0"))
			{
				Throw(version.S(), "Expecting (Version 1.0) in the ControlFlowGraphSystem directive. Value was '%s'", version.c_str());
			}

			startIndex = 0;
			auto& nodes = GetDirective(topLevelDirectives, "Nodes", IN OUT startIndex);
			
			size_t nChildren = nodes.NumberOfChildren();
			for (size_t i = 0; i < nChildren; i++)
			{
				auto& node = nodes[i];
				if (!Strings::Eq(node.FQName(), "Node"))
				{
					Throw(node.S(), "Expecting (Node ...)");
				}

				OnLoadGraphNode(node);
			}

			startIndex = 0;
			auto& cables = GetDirective(topLevelDirectives, "Cables", IN OUT startIndex);
			size_t nCables = cables.NumberOfChildren();
			for (size_t i = 0; i < nCables; i++)
			{
				auto& cable = cables[i];
				if (!Strings::Eq(cable.FQName(), "Cable"))
				{
					Throw(cable.S(), "Expecting (Cable ...)");
				}

				OnLoadGraphCable(cable);
			}

			db->ConnectCablesToSockets();

			gridSlate->QueueRedraw();
		}

		void LoadGraph(const wchar_t* filename)
		{
			auto lambda = [this](const ISEXMLDirectiveList& directives)
			{
				this->db->Nodes().Builder().DeleteAllNodes();
				this->OnLoadGraphSXML(directives);
			};

			Rococo::OS::LoadSXMLBySysPath(filename, lambda);
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
					LoadGraph(sysPath);
					lastSavedSysPath = sysPath;
					CFGS::SetTitleWithFilename(*editor, sysPath);
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