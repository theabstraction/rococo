#include <rococo.types.h>
#include "cfgs.sexy.navigation.inl"
#include <rococo.os.win32.h>
#include <rococo.window.h>
#include <rococo.sexml.h>
#include <rococo.maths.i32.h>
#include <rococo.io.h>
#include <rococo.hashtable.h>
#include <sexy.types.h>
#include <Sexy.S-Parser.h>

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

	static const char* const CONFIG_SECTION = "cfgs.sexy.ide";
	static const char* const CONFIG_VERSION = "1.0.0.0";

	struct Sexy_CFGS_IDE : ICFGSIntegratedDevelopmentEnvironmentSupervisor, ICFGSIDEGui, ICFGSIDEContextMenu, INamespaceValidator
	{
		HWND hHostWindow;
		ICFGSDatabase& cfgs;
		SexyIDEWindow ideWindow;

		AutoFree<ISexyStudioFactory1> ssFactory;
		AutoFree<ICFGSDesignerSpacePopupSupervisor> designerSpacePopup;
		AutoFree<Sexy_CFGS_Core> core;
		AutoFree<ICFGSCosmeticsSupervisor> cosmetics;

		IAbstractEditor& editor;

		AutoFree<NavigationHandler> navHandler;

		Rococo::Events::IPublisher& publisher;

		AutoFree<Rococo::Windows::IWin32Menu> contextMenu;
		HWND hWndMenuTarget{ nullptr };

		ICFGSControllerConfig& config;

		MessageMap<Sexy_CFGS_IDE> messageMap;

		GuiRect lastWindowRect;

		Sexy_CFGS_IDE(HWND _hHostWindow, ICFGSDatabase& _cfgs, IAbstractEditor& _editor, Rococo::Events::IPublisher& _publisher, ICFGSControllerConfig& _config):
			hHostWindow(_hHostWindow), cfgs(_cfgs), editor(_editor), publisher(_publisher), contextMenu(CreateMenu(true)), hWndMenuTarget(editor.ContainerWindow()), config(_config), messageMap(_publisher, *this), lastWindowRect{ 0,0,0,0 }
		{
			messageMap.AddHandler("EvWindowResized"_event, &Sexy_CFGS_IDE::OnWindowResized);
		}

		void OnWindowResized(Abedit::WindowResizedArgs& args)
		{
			if (args.source == Abedit::WindowResizedArgs::SourceId::MainWindow)
			{
				lastWindowRect = args.screenPosition;
			}
		}

		ICFGSIDEContextMenu& ContextMenu() override
		{
			return *this;
		}

		void ContextMenu_Clear() override
		{
			while (GetMenuItemCount(*contextMenu))
			{
				DeleteMenu(*contextMenu, 0, MF_BYPOSITION);
			}
		}

		void ContextMenu_AddButton(cstr text, uint64 menuId, cstr keyCommand) override
		{
			contextMenu->AddString(text, menuId, keyCommand);
		}

		void ContextMenu_Show() override
		{
			POINT screenPos = { 0,0 };
			GetCursorPos(&screenPos);
			TrackPopupMenu(*contextMenu, TPM_VERNEGANIMATION | TPM_TOPALIGN | TPM_LEFTALIGN, screenPos.x, screenPos.y, 0, hWndMenuTarget, NULL);
		}

		bool GetUserConfirmation(cstr text, cstr caption) override
		{
			Vec2i span{ 800, 120 };
			int labelWidth = 140;

			struct VariableEventHandler : IVariableEditorEventHandler
			{
				void OnButtonClicked(cstr variableName, IVariableEditor& editor) override
				{
					bool isChecked = editor.GetBoolean(variableName);
					editor.SetEnabled(isChecked, (cstr)IDOK);
				}

				void OnModal(IVariableEditor& editor) override
				{
					editor.SetEnabled(false, (cstr)IDOK);
				}
			} evHandler;

			AutoFree<IVariableEditor> deletionBox = CreateVariableEditor(span, labelWidth, caption, &evHandler);

			deletionBox->AddBooleanEditor(text, false);
			if (deletionBox->IsModalDialogChoiceYes())
			{
				bool isConfirmed = deletionBox->GetBoolean(text);
				if (isConfirmed)
				{
					return true;
				}
			}

			return false;
		}

		IVariableEditor* CreateVariableEditor(const Vec2i& span, int32 labelWidth, cstr appQueryName, IVariableEditorEventHandler* eventHandler) override
		{
			return Rococo::CreateVariableEditor(editor.Window(), span, labelWidth, appQueryName, nullptr, nullptr, eventHandler, nullptr);
		}

		void ShowAlertBox(cstr text, cstr caption) override
		{
			Rococo::Windows::ShowMessageBox(editor.ContainerWindow(), text, caption, MB_ICONEXCLAMATION);
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

			cosmetics = CreateCosmetics(core->db);

			navHandler = new NavigationHandler(editor, cfgs, core->db, publisher, *this);

			designerSpacePopup = CreateWin32ContextPopup(editor, cfgs, ideWindow.ideInstance->GetDatabase(), *this, *cosmetics);

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
			return Rococo::CFGS::IsConnectionPermitted(anchor, target, cfgs, core->db);
		}

		ICFGSDesignerSpacePopup& DesignerSpacePopup() override
		{
			return *designerSpacePopup;
		}

		void SaveRect(const GuiRect& rect, Rococo::Sex::SEXML::ISEXMLBuilder& builder)
		{
			builder.AddAtomicAttribute("Left", rect.left);
			builder.AddAtomicAttribute("Top", rect.top);
			builder.AddAtomicAttribute("Right", rect.right);
			builder.AddAtomicAttribute("Bottom", rect.bottom);
		}

		GuiRect AsRect(const Rococo::Sex::SEXML::ISEXMLDirective& directive)
		{
			int32 left = AsAtomicInt32(directive["Left"]);
			int32 top = AsAtomicInt32(directive["Top"]);
			int32 right = AsAtomicInt32(directive["Right"]);
			int32 bottom = AsAtomicInt32(directive["Bottom"]);
			return GuiRect{ left, top, right, bottom };
		}

		void SaveConfig(Rococo::Sex::SEXML::ISEXMLBuilder& builder)
		{
			builder.AddDirective("Header");
				 builder.AddAtomicAttribute("Version", CONFIG_VERSION);
				 builder.AddAtomicAttribute("App", "Sexy_CFGS_IDE");
			builder.CloseDirective();

			builder.AddDirective("Recent");
				cstr activeFileName = config.ActiveFile();
				builder.AddStringLiteral("ActiveFile", activeFileName ? activeFileName : "");

				auto* f = cfgs.CurrentFunction();
				if (!f)
				{
					builder.AddStringLiteral("ActiveFunction", "");
				}
				else
				{
					builder.AddAtomicAttribute("ActiveFunction", f->Name());
				}

			 builder.CloseDirective();

			 builder.AddDirective("MainWindow");

			 if (lastWindowRect.right > lastWindowRect.left)
			 {
				 SaveRect(lastWindowRect, builder);
			 }
			 else
			 {
				 GuiRect nullRect = { 0,0,0,0 };
				 SaveRect(nullRect, builder);
			 }

			 builder.CloseDirective();
		}

		void OnExit() override
		{
			Rococo::OS::SaveUserSEXML(nullptr, CONFIG_SECTION, [this]
				(Rococo::Sex::SEXML::ISEXMLBuilder& builder)
				{
					SaveConfig(builder);
				}
			);
		}

		bool TryGetAssociatedFilterFile(OUT U8FilePath& filterFile, cstr filenameAsKey)
		{
			U8FilePath key;
			Assign(key, filenameAsKey);
			while (IO::MakeContainerDirectory(key.buf))
			{
				Format(filterFile, "%scfgs.filter.sexml", key.buf);
				
				if (IO::IsFileExistant(filterFile))
				{
					return true;
				}
			}

			return false;
		}

		void LoadConfig(const Rococo::Sex::SEXML::ISEXMLDirectiveList& topLevelDirectives)
		{
			using namespace Rococo::Sex::SEXML;

			size_t startIndex = 0;
			auto& header = GetDirective(topLevelDirectives, "Header", IN OUT startIndex);

			cstr version = AsString(header["Version"]).c_str();
			if (!Eq(version, CONFIG_VERSION))
			{
				Throw(0, "Bad version %s. Expecting %s", version, CONFIG_VERSION);
			}

			cstr app = AsString(header["App"]).c_str();
			if (!Eq(app, "Sexy_CFGS_IDE"))
			{
				Throw(0, "Bad app %s. Expecting Sexy_CFGS_IDE", app);
			}

			auto& recent = GetDirective(topLevelDirectives, "Recent", IN OUT startIndex);

			cstr activeFilename = AsString(recent["ActiveFile"]).c_str();
			if (activeFilename && *activeFilename)
			{
				if (config.TryLoadActiveFile(activeFilename))
				{
					cstr fqName = AsString(recent["ActiveFunction"]).c_str();
					navHandler->SelectFunction(fqName);
				}
			}

			auto& mainWindow = GetDirective(topLevelDirectives, "MainWindow", IN OUT startIndex);

			GuiRect rect = AsRect(mainWindow);

			Vec2i span = Span(rect);

			Vec2i maxSpan = GetDesktopSpan();
			Vec2i minSpan = { 128, 128 };

			if (span.x > 0 && span.y > 0)
			{
				span.x = clamp(span.x, minSpan.x, maxSpan.x);
				span.y = clamp(span.y, minSpan.y, maxSpan.y);

				if (GetParent(editor.ContainerWindow()) == nullptr)
				{
					MoveWindow(editor.ContainerWindow(), rect.left, rect.top, span.x, span.y, TRUE);
				}
			}
		}

		stringmap<int> prohibitedNamespaces;

		void ClearFilters()
		{
			prohibitedNamespaces.clear();
		}

		bool IsLegalNamespace(cstr ns) const override
		{
			if (prohibitedNamespaces.find(ns) != prohibitedNamespaces.end())
			{
				return false;
			}

			for (auto& pns : prohibitedNamespaces)
			{
				if (StartsWith(ns, pns.first))
				{
					return false;
				}
			}

			return true;
		}

		void LoadFilter(const Rococo::Sex::SEXML::ISEXMLDirectiveList& topLevelDirectives)
		{
			size_t startIndex = 0;
			auto& header = GetDirective(topLevelDirectives, "CFGSFilter", IN OUT startIndex);

			auto& version = AsString(header["Version"]);
			if (!Eq(version.c_str(), CONFIG_VERSION))
			{
				Throw(version.S(), "Bad version %s. Expecting %s", version.c_str(), CONFIG_VERSION);
			}

			auto& prohibitions = GetDirective(topLevelDirectives, "Prohibitions", IN OUT startIndex);
			auto& items = AsStringList(prohibitions["Namespaces"]);

			ClearFilters();

			for (size_t i = 0; i < items.NumberOfElements(); i++)
			{
				cstr item = items[i];
				int dummy = 0;
				prohibitedNamespaces.insert(item, dummy);
			}
		}

		void Clear() override
		{
			navHandler->Clear();
		}

		void OnLoaded(cstr filename) override
		{
			U8FilePath filterFile;
			if (TryGetAssociatedFilterFile(OUT filterFile, filename))
			{
				try
				{
					Rococo::OS::LoadSXMLBySysPath(filterFile, [this]
					(const Rococo::Sex::SEXML::ISEXMLDirectiveList& topLevelDirectives)
						{
							LoadFilter(topLevelDirectives);
						}
					);
				}
				catch (ParseException& pex)
				{
					char caption[256];
					SafeFormat(caption, "Error loading filter file", filterFile.buf);

					char line[256];
					SafeFormat(line, "%s\nLine %d char %d to line %d char %d\n%s", filterFile.buf, pex.Start().y, pex.Start().x, pex.End().y, pex.End().x, pex.Message());
					ShowMessageBox(editor.Window(), line, caption, MB_ICONEXCLAMATION);
				}
				catch (IException& ex)
				{
					char caption[256];
					SafeFormat(caption, "Error loading %s", filterFile.buf);
					ShowErrorBox(editor.Window(), ex, caption);
				}
			}
		}

		void OnInitComplete() override
		{
			RECT editorRect;
			GetWindowRect(editor.ContainerWindow(), &editorRect);
			lastWindowRect = FromRECT(editorRect);

			if (!Rococo::OS::IsUserSEXMLExistant(nullptr, CONFIG_SECTION))
			{
				return;
			}

			Rococo::OS::LoadUserSEXML(nullptr, CONFIG_SECTION, [this]
				(const Rococo::Sex::SEXML::ISEXMLDirectiveList& topLevelDirectives)
				{
					LoadConfig(topLevelDirectives);
				}
			);

			cosmetics->ConfigCFGSCosmetics(cfgs);
		}
	};
}

extern "C" __declspec(dllexport) ICFGSIntegratedDevelopmentEnvironmentSupervisor* Create_CFGS_Win32_IDE(HWND hHostWindow, ICFGSDatabase& db, Rococo::Abedit::IAbstractEditor& editor, Rococo::Events::IPublisher& publisher, ICFGSControllerConfig& config)
{
	AutoFree<ANON::Sexy_CFGS_IDE> ide = new ANON::Sexy_CFGS_IDE(hHostWindow, db, editor, publisher, config);
	ide->Create();
	return ide.Detach();
}