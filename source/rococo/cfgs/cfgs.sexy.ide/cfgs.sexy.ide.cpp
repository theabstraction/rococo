#include <rococo.os.win32.h>
#include <rococo.cfgs.h>
#include <rococo.sexystudio.api.h>
#include <rococo.window.h>
#include <rococo.os.h>
#include <rococo.abstract.editor.h>
#include <rococo.visitors.h>
#include <rococo.variable.editor.h>
#include <rococo.strings.h>
#include <unordered_map>
#include <unordered_set>

using namespace Rococo;
using namespace Rococo::CFGS;
using namespace Rococo::SexyStudio;
using namespace Rococo::Windows;
using namespace Rococo::Visitors;
using namespace Rococo::Abedit;

namespace Rococo::CFGS
{
	ICFGSDesignerSpacePopupSupervisor* CreateWin32ContextPopup(IAbstractEditor& editor, ICFGSDatabase& cfgs, SexyStudio::ISexyDatabase& db);
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
		Visitors::TREE_NODE_ID parentId;
		Visitors::IUITree& tree;

		FNameValidator(IVariableEditor& _editor, Visitors::TREE_NODE_ID id, Visitors::IUITree& _tree): editor(_editor), parentId(id), tree(_tree)
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

			for (cstr p = textEditorContent + 1; *p != 0; p++)
			{
				if (!isalnum(*p))
				{
					editor.SetHintError(variableName, "The trailing characters of a function name must be alpha numerics. A-Z, a-z or 0-9");
					return false;
				}
			}

			if (0 != tree.FindFirstChild(parentId, textEditorContent).value)
			{
				editor.SetHintError(variableName, "The function name already exists in the tree");
				return false;
			}

			return true;
		}
	};

	struct NamespaceValidator : IStringValidator
	{
		IVariableEditor& editor;
		Visitors::TREE_NODE_ID parentId;
		Visitors::IUITree& tree;

		NamespaceValidator(IVariableEditor& _editor, Visitors::TREE_NODE_ID id, Visitors::IUITree& _tree) : editor(_editor), parentId(id), tree(_tree)
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
				editor.SetHintError(variableName, "The first character of a subspace name must be a capital A-Z");
				return false;
			}

			for (cstr p = textEditorContent + 1; *p != 0; p++)
			{
				if (!isalnum(*p))
				{
					editor.SetHintError(variableName, "The trailing characters of a subspace name must be alpha numerics. A-Z, a-z or 0-9");
					return false;
				}
			}

			if (0 != tree.FindFirstChild(parentId, textEditorContent).value)
			{
				char msg[256];

				char subspace[128];
				if (tree.TryGetText(subspace, sizeof subspace, parentId))
				{
					SafeFormat(msg, "The subspace name already exists in '%s'", subspace);
				}
				else
				{
					SafeFormat(msg, "The subspace name already exists in the root namespace");
				}

				editor.SetHintError(variableName, msg);
				return false;
			}

			return true;
		}
	};

	struct RenameNamespaceValidator : IStringValidator
	{
		IVariableEditor& editor;
		Visitors::TREE_NODE_ID id;
		Visitors::IUITree& tree;

		RenameNamespaceValidator(IVariableEditor& _editor, Visitors::TREE_NODE_ID _id, Visitors::IUITree& _tree) : editor(_editor), id(_id), tree(_tree)
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
				editor.SetHintError(variableName, "The first character of a subspace name must be a capital A-Z");
				return false;
			}

			for (cstr p = textEditorContent + 1; *p != 0; p++)
			{
				if (!isalnum(*p))
				{
					editor.SetHintError(variableName, "The trailing characters of a subspace name must be alpha numerics. A-Z, a-z or 0-9");
					return false;
				}
			}

			char text[128];
			if (!tree.TryGetText(text, sizeof text, id))
			{
				Throw(0, "Could not get text for the namespace item");
			}

			auto parentId = tree.GetParent(id);
			if (parentId.value == 0)
			{
				Throw(0, "Could not get parent for the namespace item");
			}

			auto match = tree.FindFirstChild(parentId, textEditorContent);

			if (match.value)
			{
				editor.SetHintError(variableName, "The subspace name already exists");
				return false;
			}

			return true;
		}
	};

	struct RenameFNameValidator : IStringValidator
	{
		IVariableEditor& editor;
		Visitors::TREE_NODE_ID id;
		Visitors::IUITree& tree;

		RenameFNameValidator(IVariableEditor& _editor, Visitors::TREE_NODE_ID _id, Visitors::IUITree& _tree) : editor(_editor), id(_id), tree(_tree)
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

			for (cstr p = textEditorContent + 1; *p != 0; p++)
			{
				if (!isalnum(*p))
				{
					editor.SetHintError(variableName, "The trailing characters of a function name must be alpha numerics. A-Z, a-z or 0-9");
					return false;
				}
			}

			char text[128];
			if (!tree.TryGetText(text, sizeof text, id))
			{
				Throw(0, "Could not get text for the function item");
			}

			auto parentId = tree.GetParent(id);
			if (parentId.value == 0)
			{
				Throw(0, "Could not get parent for the function item");
			}

			auto match = tree.FindFirstChild(parentId, textEditorContent);

			if (match.value)
			{
				editor.SetHintError(variableName, "The function name already exists");
				return false;
			}

			return true;
		}
	};

	struct NavigationHandler: Visitors::ITreeControlHandler
	{
		TREE_NODE_ID localFunctionsId = { 0 };
		TREE_NODE_ID namespacesId = { 0 };
		TREE_NODE_ID variablesId = { 0 };

		IAbstractEditor& editor;
		ICFGSDatabase& cfgs;

		std::unordered_set<TREE_NODE_ID, TREE_NODE_ID::Hasher> namespaceIdSet;
		std::unordered_map<TREE_NODE_ID, FunctionId, TREE_NODE_ID::Hasher> localFunctionMap;
		std::unordered_map<TREE_NODE_ID, FunctionId, TREE_NODE_ID::Hasher> publicFunctionMap;

		AutoFree<Rococo::Windows::IWin32Menu> contextMenu;

		enum
		{
			CONTEXT_MENU_ID_ADD_SUBSPACE = 4001,
			CONTEXT_MENU_ID_ADD_FUNCTION,
			CONTEXT_MENU_DELETE_FUNCTION,
			CONTEXT_MENU_ID_RENAME_NAMESPACE,
			CONTEXT_MENU_ID_RENAME_FUNCTION
		};

		HWND hWndMenuTarget{ nullptr };
		TREE_NODE_ID contextMenuTargetId = { 0 };


		NavigationHandler(IAbstractEditor& _editor, ICFGSDatabase& _cfgs) : editor(_editor), cfgs(_cfgs), hWndMenuTarget(_editor.ContainerWindow())
		{
			contextMenu = CreateMenu(true);
		}

		~NavigationHandler()
		{
			for (auto& i : localFunctionMap)
			{
				cfgs.DeleteFunction(i.second);
			}

			for (auto& i : publicFunctionMap)
			{
				cfgs.DeleteFunction(i.second);
			}
		}

		void ClearMenu()
		{
			while (GetMenuItemCount(*contextMenu))
			{
				DeleteMenu(*contextMenu, 0, MF_BYPOSITION);
			}
		}

		void OnItemSelected(TREE_NODE_ID id, IUITree& origin) override
		{
			UNUSED(origin);

			auto i = localFunctionMap.find(id);
			if (i != localFunctionMap.end())
			{
				cfgs.BuildFunction(i->second);
			}
			else
			{
				auto j = publicFunctionMap.find(id);
				if (j != publicFunctionMap.end())
				{
					cfgs.BuildFunction(j->second);
				}
			}

			editor.RefreshSlate();
		}

		void OnItemRightClicked(TREE_NODE_ID id, IUITree& tree) override
		{
			if (id == localFunctionsId)
			{
				Vec2i span{ 800, 120 };
				int labelWidth = 120;

				char fname[128] = { 0 };

				AutoFree<IVariableEditor> fnameEditor = CreateVariableEditor(editor.Window(), span, labelWidth, "CFGS Sexy IDE - Add a new local function...", nullptr, nullptr, &nullEventHandler, nullptr);

				FNameValidator fnameValidator(*fnameEditor, id, tree);
				fnameEditor->AddStringEditor("Local Name", nullptr, fname, sizeof fname, &fnameValidator);
				if (fnameEditor->IsModalDialogChoiceYes())
				{
					char fullname[256];
					GetFQName(fullname, sizeof fullname, fname, tree, { 0 });

					auto newLocalFunctionId = tree.AddChild(localFunctionsId, fname, Visitors::CheckState_NoCheckBox);

					FunctionId f = cfgs.CreateFunction();
					cfgs.FindFunction(f)->SetName(fullname);

					localFunctionMap.insert(std::make_pair(newLocalFunctionId, f));
					tree.Select(newLocalFunctionId);
				}
			}
			else if (id == namespacesId)
			{
				Vec2i span{ 800, 120 };
				int labelWidth = 120;

				char fname[128] = { 0 };

				AutoFree<IVariableEditor> nsEditor = CreateVariableEditor(editor.Window(), span, labelWidth, "CFGS Sexy IDE - Add a top level namespace...", nullptr, nullptr, &nullEventHandler, nullptr);

				NamespaceValidator fnameValidator(*nsEditor, id, tree);
				nsEditor->AddStringEditor("Root Namespace", nullptr, fname, sizeof fname, &fnameValidator);
				if (nsEditor->IsModalDialogChoiceYes())
				{
					auto newTopLevelNamespaceId = tree.AddChild(namespacesId, fname, Visitors::CheckState_NoCheckBox);
					namespaceIdSet.insert(newTopLevelNamespaceId);
					tree.Select(newTopLevelNamespaceId);
				}
			}
			else
			{
				ClearMenu();

				auto i = namespaceIdSet.find(id);
				if (i != namespaceIdSet.end())
				{
					contextMenu->AddString("Add subspace", CONTEXT_MENU_ID_ADD_SUBSPACE);
					contextMenu->AddString("Add public function", CONTEXT_MENU_ID_ADD_FUNCTION);
					contextMenu->AddString("Rename namespace", CONTEXT_MENU_ID_RENAME_NAMESPACE);
					contextMenuTargetId = id;
				}
				else
				{
					auto j = publicFunctionMap.find(id);
					if (j != publicFunctionMap.end())
					{
						contextMenu->AddString("Delete function", CONTEXT_MENU_DELETE_FUNCTION);
						contextMenu->AddString("Rename function", CONTEXT_MENU_ID_RENAME_FUNCTION);
						contextMenuTargetId = id;
					}
					else
					{
						auto k = localFunctionMap.find(id);
						if (k != localFunctionMap.end())
						{
							contextMenu->AddString("Delete function", CONTEXT_MENU_DELETE_FUNCTION);
							contextMenu->AddString("Rename function", CONTEXT_MENU_ID_RENAME_FUNCTION);
							contextMenuTargetId = id;
						}
						else
						{
							Beep(512, 200);
							return;
						}
					}
				}

				POINT screenPos = { 0,0 };
				GetCursorPos(&screenPos);
				TrackPopupMenu(*contextMenu, TPM_VERNEGANIMATION | TPM_TOPALIGN | TPM_LEFTALIGN, screenPos.x, screenPos.y, 0, hWndMenuTarget, NULL);
			}
		}

		void RefreshNavigationTree()
		{
			auto& t = editor.NavigationTree();
			t.ResetContent();
			localFunctionsId = t.AddRootItem("Local Functions", Visitors::CheckState_NoCheckBox);
			namespacesId = t.AddRootItem("Namespaces", Visitors::CheckState_NoCheckBox);
			variablesId = t.AddRootItem("Graph Variables", Visitors::CheckState_NoCheckBox);
		}

		struct NullVariableEditorEventHandler : IVariableEditorEventHandler
		{
			void OnButtonClicked(cstr variableName, IVariableEditor&) override
			{
				UNUSED(variableName);
			}

			void OnModal(IVariableEditor&)
			{

			}
		} nullEventHandler;

		void AddNamespaceAt(TREE_NODE_ID id, IUITree& tree)
		{
			auto i = namespaceIdSet.find(id);
			if (i == namespaceIdSet.end())
			{
				Rococo::Windows::ShowMessageBox(editor.ContainerWindow(), "Internal error. Could not identify the selected namespace", "CFGS Sexy IDE - Algorithmic Error", MB_ICONEXCLAMATION);
				return;
			}

			Vec2i span{ 800, 120 };
			int labelWidth = 120;

			char fname[128] = { 0 };

			char subspace[128];
			if (tree.TryGetText(subspace, sizeof subspace, id))
			{
				char ntitle[256];
				SafeFormat(ntitle, "%s - Add a subspace to %s...", title, subspace);

				AutoFree<IVariableEditor> nsEditor = CreateVariableEditor(editor.Window(), span, labelWidth, ntitle, nullptr, nullptr, &nullEventHandler, nullptr);

				NamespaceValidator fnameValidator(*nsEditor, id, tree);
				nsEditor->AddStringEditor("Subspace", nullptr, fname, sizeof fname, &fnameValidator);
				if (nsEditor->IsModalDialogChoiceYes())
				{
					auto newSubspaceId = tree.AddChild(id, fname, Visitors::CheckState_NoCheckBox);
					namespaceIdSet.insert(newSubspaceId);
					tree.Select(newSubspaceId);
				}
			}
		}

		void UpdateAllFunctionNames(TREE_NODE_ID namespaceId, IUITree& tree)
		{
			TREE_NODE_ID childId = tree.FindFirstChild(namespaceId, nullptr);
			while (childId.value != 0)
			{
				auto i = namespaceIdSet.find(childId);
				if (i == namespaceIdSet.end())
				{
					UpdateAllFunctionNames(childId, tree);
				}
				else
				{
					// We have a function
					auto j = publicFunctionMap.find(childId);
					if (j != publicFunctionMap.end())
					{
						FunctionId id = j->second;

						char fname[256];
						if (tree.TryGetText(fname, sizeof fname, childId))
						{
							char fqName[256];
							GetFQName(fqName, sizeof fqName, fname, tree, namespaceId);
							cfgs.FindFunction(id)->SetName(fqName);
						}
					}
				}

				childId = tree.FindNextChild(childId, nullptr);
			}
		}

		void RenameNamespace(TREE_NODE_ID namespaceId, IUITree& tree)
		{
			auto i = namespaceIdSet.find(namespaceId);
			if (i == namespaceIdSet.end())
			{
				Rococo::Windows::ShowMessageBox(editor.ContainerWindow(), "Internal error. Could not identify the selected namespace", "CFGS Sexy IDE - Algorithmic Error", MB_ICONEXCLAMATION);
				return;
			}

			Vec2i span{ 800, 120 };
			int labelWidth = 120;

			char newSubspace[128] = { 0 };

			char subspace[128];
			if (tree.TryGetText(subspace, sizeof subspace, namespaceId))
			{
				char ntitle[256];
				SafeFormat(ntitle, "%s - Rename subspace %s...", title, subspace);

				AutoFree<IVariableEditor> nsEditor = CreateVariableEditor(editor.Window(), span, labelWidth, ntitle, nullptr, nullptr, &nullEventHandler, nullptr);

				RenameNamespaceValidator fnameValidator(*nsEditor, namespaceId, tree);
				nsEditor->AddStringEditor("New Subspace", nullptr, newSubspace, sizeof newSubspace, &fnameValidator);
				if (nsEditor->IsModalDialogChoiceYes())
				{
					tree.SetText(namespaceId, newSubspace);
					tree.Select(namespaceId);

					UpdateAllFunctionNames(namespaceId, tree);
				}
			}
		}

		void RenameFunction(TREE_NODE_ID functionId, IUITree& tree)
		{
			auto i = publicFunctionMap.find(functionId);
			if (i == publicFunctionMap.end())
			{
				auto j = localFunctionMap.find(functionId);
				if (j == localFunctionMap.end())
				{
					Rococo::Windows::ShowMessageBox(editor.ContainerWindow(), "Internal error. Could not identify the selected function", "CFGS Sexy IDE - Algorithmic Error", MB_ICONEXCLAMATION);
					return;
				}
			}

			Vec2i span{ 800, 120 };
			int labelWidth = 140;

			char newFunctionName[128] = { 0 };

			char functionName[128];
			if (tree.TryGetText(functionName, sizeof functionName, functionId))
			{
				SafeFormat(newFunctionName, "%s", functionName);

				char ntitle[256];
				SafeFormat(ntitle, "%s - Rename function %s...", title, functionName);

				AutoFree<IVariableEditor> nsEditor = CreateVariableEditor(editor.Window(), span, labelWidth, ntitle, nullptr, nullptr, &nullEventHandler, nullptr);

				RenameFNameValidator fnameValidator(*nsEditor, functionId, tree);
				nsEditor->AddStringEditor("New Function Name", nullptr, newFunctionName, sizeof newFunctionName, &fnameValidator);
				if (nsEditor->IsModalDialogChoiceYes())
				{
					tree.SetText(functionId, newFunctionName);
					tree.Select(functionId);

					TREE_NODE_ID namespaceId{ 0 };

					FunctionId id;

					i = publicFunctionMap.find(functionId);
					if (i != publicFunctionMap.end())
					{
						id = i->second;
						namespaceId = tree.GetParent(functionId);
					}
					else
					{
						auto j = localFunctionMap.find(functionId);
						if (j != localFunctionMap.end())
						{
							id = j->second;
						}
					}

					if (id)
					{
						char fqName[256];
						GetFQName(fqName, sizeof fqName, newFunctionName, tree, namespaceId);
						cfgs.FindFunction(id)->SetName(fqName);
					}
				}
			}
		}

		void DeleteFunction(TREE_NODE_ID functionId, IUITree& tree)
		{
			auto i = publicFunctionMap.find(functionId);
			if (i == publicFunctionMap.end())
			{
				auto j = localFunctionMap.find(functionId);
				if (j == localFunctionMap.end())
				{
					Rococo::Windows::ShowMessageBox(editor.ContainerWindow(), "Internal error. Could not identify the selected function", "CFGS Sexy IDE - Algorithmic Error", MB_ICONEXCLAMATION);
					return;
				}
			}

			Vec2i span{ 800, 120 };
			int labelWidth = 140;

			struct VariableEventHandler : IVariableEditorEventHandler
			{
				void OnButtonClicked(cstr variableName, IVariableEditor& editor) override
				{
					bool isChecked = editor.GetBoolean(variableName);
					editor.SetEnabled(isChecked, (cstr) IDOK);
				}

				void OnModal(IVariableEditor& editor) override
				{
					editor.SetEnabled(false, (cstr)IDOK);
				}
			} evHandler;

			char functionName[128];
			if (tree.TryGetText(functionName, sizeof functionName, functionId))
			{
				char ntitle[256];
				SafeFormat(ntitle, "%s - Delete function %s...", title, functionName);

				AutoFree<IVariableEditor> deletionBox = CreateVariableEditor(editor.Window(), span, labelWidth, ntitle, nullptr, nullptr, &evHandler, nullptr);

				cstr confirmText = "Confirm deletion";

				deletionBox->AddBooleanEditor(confirmText, false);
				if (deletionBox->IsModalDialogChoiceYes())
				{
					bool isConfirmed = deletionBox->GetBoolean(confirmText);
					if (isConfirmed)
					{
						auto k = publicFunctionMap.find(functionId);
						if (k != publicFunctionMap.end())
						{
							cfgs.DeleteFunction(k->second);
							publicFunctionMap.erase(k);
						}
						else
						{
							auto l = localFunctionMap.find(functionId);
							if (l != localFunctionMap.end())
							{
								cfgs.DeleteFunction(l->second);
								publicFunctionMap.erase(l);
							}
						}
					}

					tree.Delete(functionId);
				}
			}
		}

		static char* WriteBackwardsAndReturnEndPos(char* startOfBuffer, char* endPos, cstr token)
		{
			size_t len = strlen(token);
			endPos -= len;
			if (endPos < startOfBuffer)
			{
				return nullptr;
			}

			memcpy_s(endPos, endPos - startOfBuffer, token, len);
			return endPos;
		}

		void GetFQName(char* buffer, size_t sizeOfBuffer, cstr typeName, IUITree& tree, TREE_NODE_ID tailNamespaceId) const
		{
			if (sizeOfBuffer < 8)
			{
				Throw(0, "Titchy sizeOfBuffer");
			}

			char* temp = (char*) _alloca(sizeOfBuffer);

			char* end = temp + sizeOfBuffer;

			end--;
			*end = 0;

			end = WriteBackwardsAndReturnEndPos(temp, end, typeName);

			TREE_NODE_ID namespaceId = tailNamespaceId;

			for (;;)
			{
				auto i = namespaceIdSet.find(namespaceId);
				if (i == namespaceIdSet.end())
				{
					// No more namespaces, so we fully qualified the name
					strcpy_s(buffer, sizeOfBuffer, end);
					return;
				}

				char subspace[128];
				if (tree.TryGetText(subspace, sizeof subspace, namespaceId))
				{
					end = WriteBackwardsAndReturnEndPos(temp, end, ".");
					end = WriteBackwardsAndReturnEndPos(temp, end, subspace);
				}

				namespaceId = tree.GetParent(namespaceId);
			}
		}

		void AddFunctionAt(TREE_NODE_ID namespaceId, IUITree& tree)
		{
			auto i = namespaceIdSet.find(namespaceId);
			if (i == namespaceIdSet.end())
			{
				Rococo::Windows::ShowMessageBox(editor.ContainerWindow(), "Internal error. Could not identify the selected namespace", "CFGS Sexy IDE - Algorithmic Error", MB_ICONEXCLAMATION);
				return;
			}

			Vec2i span{ 800, 120 };
			int labelWidth = 120;

			char fname[128] = { 0 };

			char subspace[128];
			if (tree.TryGetText(subspace, sizeof subspace, namespaceId))
			{
				char ntitle[256];
				SafeFormat(ntitle, "%s - Add a function to %s...", title, subspace);

				AutoFree<IVariableEditor> nsEditor = CreateVariableEditor(editor.Window(), span, labelWidth, ntitle, nullptr, nullptr, &nullEventHandler, nullptr);

				NamespaceValidator fnameValidator(*nsEditor, namespaceId, tree);
				nsEditor->AddStringEditor("Function", nullptr, fname, sizeof fname, &fnameValidator);
				if (nsEditor->IsModalDialogChoiceYes())
				{
					char fullname[256];
					GetFQName(fullname, sizeof fullname, fname, tree, namespaceId);

					auto newPublicFunctionId = tree.AddChild(namespaceId, fname, Visitors::CheckState_NoCheckBox);
					auto id = cfgs.CreateFunction();
					auto* f = cfgs.FindFunction(id);
					f->SetName(fullname);
					publicFunctionMap.insert(std::make_pair(newPublicFunctionId, id));
					tree.Select(newPublicFunctionId);
				}
			}
		}

		bool TryHandleContextMenuItem(uint16 menuCommandId)
		{
			switch (menuCommandId)
			{
			case CONTEXT_MENU_ID_ADD_FUNCTION:
				AddFunctionAt(contextMenuTargetId, editor.NavigationTree());
				return true;
			case CONTEXT_MENU_ID_ADD_SUBSPACE:
				AddNamespaceAt(contextMenuTargetId, editor.NavigationTree());
				return true;
			case CONTEXT_MENU_ID_RENAME_FUNCTION:
				RenameFunction(contextMenuTargetId, editor.NavigationTree());
				return true;
			case CONTEXT_MENU_ID_RENAME_NAMESPACE:
				RenameNamespace(contextMenuTargetId, editor.NavigationTree());
				return true;
			case CONTEXT_MENU_DELETE_FUNCTION:
				DeleteFunction(contextMenuTargetId, editor.NavigationTree());
				return true;
			}

			return false;
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

		NavigationHandler navHandler;

		Sexy_CFGS_IDE(HWND _hHostWindow, ICFGSDatabase& _cfgs, IAbstractEditor& _editor): hHostWindow(_hHostWindow), cfgs(_cfgs), editor(_editor), navHandler(_editor, _cfgs)
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

			designerSpacePopup = CreateWin32ContextPopup(editor, cfgs, ideWindow.ideInstance->GetDatabase());

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

		bool TryHandleContextMenuItem(uint16 id) override
		{
			return navHandler.TryHandleContextMenuItem(id);
		}
	};
}

extern "C" __declspec(dllexport) ICFGSIntegratedDevelopmentEnvironmentSupervisor* Create_CFGS_Win32_IDE(HWND hHostWindow, ICFGSDatabase& db, Rococo::Abedit::IAbstractEditor& editor)
{
	AutoFree<ANON::Sexy_CFGS_IDE> ide = new ANON::Sexy_CFGS_IDE(hHostWindow, db, editor);
	ide->Create();
	return ide.Detach();
}