#include "cfgs.sexy.navigation.inl"

using namespace Rococo::CFGS::IDE::Sexy;

namespace ANON
{
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

	struct Sexy_CFGS_IDE : ICFGSIntegratedDevelopmentEnvironmentSupervisor
	{
		HWND hHostWindow;
		ICFGSDatabase& cfgs;
		SexyIDEWindow ideWindow;

		AutoFree<ISexyStudioFactory1> ssFactory;
		AutoFree<ICFGSDesignerSpacePopupSupervisor> designerSpacePopup;
		AutoFree<Sexy_CFGS_Core> core;

		IAbstractEditor& editor;

		AutoFree<NavigationHandler> navHandler;

		Rococo::Events::IPublisher& publisher;

		Sexy_CFGS_IDE(HWND _hHostWindow, ICFGSDatabase& _cfgs, IAbstractEditor& _editor, Rococo::Events::IPublisher& _publisher):
			hHostWindow(_hHostWindow), cfgs(_cfgs), editor(_editor), publisher(_publisher)
		{
			_cfgs.ListenForChange(
				[this](FunctionId id) 
				{
					auto* f = cfgs.CurrentFunction();
					if (f && f->Id() == id)
					{
						editor.Properties().Clear();
						editor.Properties().BuildEditorsForProperties(f->PropertyVenue());
						editor.RefreshSlate();
					}
				}
			);
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

			navHandler = new NavigationHandler(editor, cfgs, core->db, publisher);

			designerSpacePopup = CreateWin32ContextPopup(editor, cfgs, ideWindow.ideInstance->GetDatabase());

			editor.SetNavigationHandler(navHandler);

			navHandler->RefreshNavigationTree();
		}

		ICFGSIDENavigation& Navigation() override
		{
			return *navHandler;
		}

		void Free() override
		{
			delete this;
		}

		bool IsConnectionPermitted(const CableConnection& anchor, const ICFGSSocket& target) const override
		{
			return Rococo::CFGS::IDE::Sexy::IsConnectionPermitted(anchor, target, cfgs, core->db);
		}

		ICFGSDesignerSpacePopup& DesignerSpacePopup() override
		{
			return *designerSpacePopup;
		}
	};
}

extern "C" __declspec(dllexport) ICFGSIntegratedDevelopmentEnvironmentSupervisor* Create_CFGS_Win32_IDE(HWND hHostWindow, ICFGSDatabase& db, Rococo::Abedit::IAbstractEditor& editor, Rococo::Events::IPublisher& publisher)
{
	AutoFree<ANON::Sexy_CFGS_IDE> ide = new ANON::Sexy_CFGS_IDE(hHostWindow, db, editor, publisher);
	ide->Create();
	return ide.Detach();
}