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
	struct CFGS_Controller: IMVC_ControllerSupervisor, IAbstractEditorMainWindowEventHandler, IPropertyManager
	{
		AutoFree<IAbstractEditorSupervisor> editor;

		bool terminateOnMainWindowClose = false;

		bool isRunning = true;

		HString element = "Uranium";
		int32 atomicNumber = 92;
		float atomicWeight = 238.0f;
		double valency = 6.01;
		bool isRadioactive = true;

		char fullDesc[256] = { 0 };

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

			FormatDesc();

			auto& props = editor->Properties();
			props.Build(*this);
		}

		void FormatDesc()
		{
			SafeFormat(fullDesc, "Element #%d '%s': weight %f, valency %g. %s", atomicNumber, element.c_str(), atomicWeight, valency, isRadioactive ? "<radioactive>" : "<stable>");
		}

		void SerializeProperties(IPropertySerializer& serializer) override
		{
			serializer.Target("e1", "Element", element, 12);
			serializer.Target("an", "Atomic Number", atomicNumber, AllInt32sAreValid(), Int32Decimals());
			serializer.Target("aw", "Atomic Weight", atomicWeight, AllFloatsAreValid(), FloatDecimals());
			serializer.Target("va", "Valency", valency, AllDoublesAreValid(), DoubleDecimals());
			serializer.Target("ra", "Is RadioActive", isRadioactive, AllBoolsAreValid(), BoolFormatter());
			serializer.AddHeader("Desc", fullDesc);
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