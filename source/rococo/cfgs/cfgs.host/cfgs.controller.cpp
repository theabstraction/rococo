#include <rococo.mvc.h>
#include <rococo.abstract.editor.h>
#include <rococo.strings.h>
#include <rococo.validators.h>

using namespace Rococo;
using namespace Rococo::Strings;
using namespace Rococo::Abedit;
using namespace Rococo::MVC;
using namespace Rococo::Validators;

namespace ANON
{
	struct Element: IEstateAgent
	{
		HString name = "Uranium";
		int32 atomicNumber = 92;
		float atomicWeight = 238.0f;
		double valency = 6.01;
		bool isRadioactive = true;
		char fullDesc[256] = { 0 };

		void FormatDesc()
		{
			SafeFormat(fullDesc, "Element #%d '%s': weight %f, valency %g. %s", atomicNumber, name.c_str(), atomicWeight, valency, isRadioactive ? "<radioactive>" : "<stable>");
		}

		void AcceptVisit(IPropertyVisitor& visitor, IPropertyUIEvents& eventHandler) override
		{
			MARSHAL_STRING(visitor, "e1", "Element", eventHandler, REF name, 12);
			MARSHAL_PRIMITIVE(visitor, "an", "Atomic Number", eventHandler, REF atomicNumber, AllInt32sAreValid(), Int32Decimals());
			MARSHAL_PRIMITIVE(visitor, "aw", "Atomic Weight", eventHandler, REF atomicWeight, AllFloatsAreValid(), FloatDecimals());
			MARSHAL_PRIMITIVE(visitor, "va", "Valency", eventHandler, REF valency, AllDoublesAreValid(), DoubleDecimals());
			MARSHAL_PRIMITIVE(visitor, "ra", "Is Radioactive", eventHandler, REF isRadioactive, AllBoolsAreValid(), BoolFormatter());

			/* 
				We have an example here, of a variable fullDesc that is dependent on the other variables, so when they are updated by the visitor
				we need to recompute it. We also need to signal that the editor/viewer for the dependent variable 
			*/

			visitor.VisitHeader("Desc", "Description", fullDesc);

			if (visitor.IsWritingToReferences())
			{
				FormatDesc();
				eventHandler.OnDependentVariableChanged("Desc", *this);
			}
		}
	};

	struct CFGS_Controller: IMVC_ControllerSupervisor, IAbstractEditorMainWindowEventHandler, IPropertyVenue, IPropertyUIEvents
	{
		AutoFree<IAbstractEditorSupervisor> editor;

		bool terminateOnMainWindowClose = false;

		bool isRunning = true;

		Element element;

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
			editor = editorFactory->CreateAbstractEditor(IN config, *this);
			if (!editor)
			{
				Throw(0, "%s: Expected editorFactory->CreateAbstractEditor() to return a non-NULL pointer", __FUNCTION__);
			}

			element.FormatDesc();

			auto& props = editor->Properties();
			props.BuildEditorsForProperties(*this);
		}

		void VisitVenue(IPropertyVisitor& visitor) override
		{
			element.AcceptVisit(visitor, *this);
		}

		void OnBooleanButtonChanged(IProperty& property) override
		{
			auto& props = editor->Properties();
			props.UpdateFromVisuals(property, *this);
		}

		void OnPropertyEditorLostFocus(Rococo::Abedit::IProperty& property) override
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

		void TerminateOnMainWindowClose() override
		{
			terminateOnMainWindowClose = true;
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
}