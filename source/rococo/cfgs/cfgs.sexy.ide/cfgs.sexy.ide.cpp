#include <rococo.os.win32.h>
#include <rococo.cfgs.h>
#include <rococo.sexystudio.api.h>
#include <rococo.window.h>
#include <rococo.os.h>
#include <rococo.abstract.editor.h>
#include <rococo.visitors.h>
#include <rococo.variable.editor.h>

using namespace Rococo;
using namespace Rococo::CFGS;
using namespace Rococo::SexyStudio;
using namespace Rococo::Windows;

namespace Rococo::CFGS
{
	ICFGSDesignerSpacePopupSupervisor* CreateWin32ContextPopup(HWND hHostWindow, ICFGSDatabase& cfgs, SexyStudio::ISexyDatabase& db);
}

namespace ANON
{
	static const char* title = "CFGS SexyStudio IDE";

	struct SexyIDEWindow : ISexyStudioEventHandler
	{
		AutoFree<Rococo::SexyStudio::ISexyStudioInstance1> ideInstance;

		SexyIDEWindow()
		{

		}

		void Create(HWND hHostWindow, ISexyStudioFactory1& factory)
		{
			UNUSED(hHostWindow);
			ideInstance = factory.CreateSexyIDE(Windows::NoParent(), *this);
			ShowWindow(ideInstance->GetIDEFrame(), SW_SHOW);
		}

		bool TryOpenEditor(cstr filePath, int lineNumber) override
		{
			Rococo::OS::ShellOpenDocument(ideInstance->GetIDEFrame(), "CFGS SexyStudio IDE. Open file... ", filePath, lineNumber);
			return true;
		}

		EIDECloseResponse OnIDEClose(IWindow& topLevelParent) override
		{
			UNUSED(topLevelParent);
			ShowWindow(ideInstance->GetIDEFrame(), SW_HIDE);
			return EIDECloseResponse::Continue;
		}
	};

	struct Sexy_CFGS_Core
	{
		SexyStudio::ISexyDatabase& db;
		ICFGSDatabase& cfgs;

		Sexy_CFGS_Core(SexyStudio::ISexyDatabase& _db, ICFGSDatabase& _cfgs):
			db(_db), cfgs(_cfgs)
		{

		}

		void Free()
		{
			delete this;
		}
	};

	struct FNameValidator : IStringValidator
	{
		IVariableEditor& editor;

		FNameValidator(IVariableEditor& _editor): editor(_editor)
		{

		}

		bool ValidateAndReportErrors(cstr textEditorContent, cstr variableName) override
		{
			if (*textEditorContent == 0)
			{
				editor.SetHintError(variableName, "The name is blank");
				return false;
			}
			if (!isupper(*textEditorContent))
			{
				editor.SetHintError(variableName, "The first character of a function name must be a capital A-Z");
				return false;
			}

			return true;
		}
	};

	struct NavigationHandler: Visitors::ITreeControlHandler
	{
		Rococo::Visitors::TREE_NODE_ID functionsId = { 0 };
		Rococo::Visitors::TREE_NODE_ID variablesId = { 0 };

		Rococo::Abedit::IAbstractEditor& editor;

		NavigationHandler(Rococo::Abedit::IAbstractEditor& _editor) : editor(_editor)
		{

		}

		void OnItemSelected(Visitors::TREE_NODE_ID id, Visitors::IUITree& origin) override
		{

		}

		void OnItemRightClicked(Visitors::TREE_NODE_ID id, Visitors::IUITree& origin) override
		{
			if (id == functionsId)
			{
				Vec2i span{ 640, 120 };
				int labelWidth = 120;

				char fname[128] = { 0 };

				struct VariableEventHandler : IVariableEditorEventHandler
				{
					void OnButtonClicked(cstr variableName) override
					{

					}
				} evHandler;

				AutoFree<IVariableEditor> fnameEditor = CreateVariableEditor(editor.Window(), span, labelWidth, "CFGS Sexy IDE - Add a new function...", nullptr, nullptr, &evHandler, nullptr);

				FNameValidator fnameValidator(*fnameEditor);
				fnameEditor->AddStringEditor("Function Name", nullptr, fname, sizeof fname, &fnameValidator);
				if (fnameEditor->IsModalDialogChoiceYes())
				{
					origin.AddChild(functionsId, fname, Visitors::CheckState_NoCheckBox);
				}
			}
		}

		void RefreshNavigationTree()
		{
			auto& t = editor.NavigationTree();
			t.ResetContent();
			functionsId = t.AddRootItem("Function Graphs", Visitors::CheckState_NoCheckBox);
			variablesId = t.AddRootItem("Graph Variables", Visitors::CheckState_NoCheckBox);
		}
	};

	struct Sexy_CFGS_IDE : ICFGSIntegratedDevelopmentEnvironmentSupervisor
	{
		HWND hHostWindow;
		ICFGSDatabase& cfgs;
		SexyIDEWindow ideWindow;

		AutoFree<ICFGSDesignerSpacePopupSupervisor> designerSpacePopup;
		AutoFree<ISexyStudioFactory1> ssFactory;
		AutoFree<Sexy_CFGS_Core> core;

		Rococo::Abedit::IAbstractEditor& editor;

		NavigationHandler navHandler;

		Sexy_CFGS_IDE(HWND _hHostWindow, ICFGSDatabase& _cfgs, Rococo::Abedit::IAbstractEditor& _editor): hHostWindow(_hHostWindow), cfgs(_cfgs), editor(_editor), navHandler(_editor)
		{
		}

		void Create()
		{
			HMODULE hSexyStudio = LoadLibraryA("sexystudio.dll");
			if (!hSexyStudio)
			{
				Throw(GetLastError(), "%s: failed to load sexystudio.dll", __FUNCTION__);
			}

			auto CreateSexyStudioFactory = (FN_CreateSexyStudioFactory) GetProcAddress(hSexyStudio, "CreateSexyStudioFactory");
			if (!CreateSexyStudioFactory)
			{	
				Throw(GetLastError(), "%s: failed to find proc CreateSexyStudioFactory in sexystudio.dll", __FUNCTION__);
			}

			cstr interfaceURL = "Rococo.SexyStudio.ISexyStudioFactory1";

			int nErr = CreateSexyStudioFactory((void**)&ssFactory, interfaceURL);
			if FAILED(nErr)
			{
				Throw(nErr, "CreateSexyStudioFactory with URL %s failed", interfaceURL);
			}	

			ideWindow.Create(hHostWindow, *ssFactory);	

			core = new Sexy_CFGS_Core(ideWindow.ideInstance->GetDatabase(), cfgs);

			designerSpacePopup = CreateWin32ContextPopup(hHostWindow, cfgs, ideWindow.ideInstance->GetDatabase());

			editor.SetNavigationHandler(&navHandler);

			navHandler.RefreshNavigationTree();
		}

		void Free() override
		{
			delete this;
		}

		ICFGSDesignerSpacePopup& DesignerSpacePopup() override
		{
			return *designerSpacePopup;
		}
	};
}

extern "C" __declspec(dllexport) ICFGSIntegratedDevelopmentEnvironmentSupervisor* Create_CFGS_Win32_IDE(HWND hHostWindow, ICFGSDatabase& db, Rococo::Abedit::IAbstractEditor& editor)
{
	AutoFree<ANON::Sexy_CFGS_IDE> ide = new ANON::Sexy_CFGS_IDE(hHostWindow, db, editor);
	ide->Create();
	return ide.Detach();
}