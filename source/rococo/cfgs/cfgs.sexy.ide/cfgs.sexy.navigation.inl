#pragma once

#include <rococo.cfgs.h>
#include <rococo.sexystudio.api.h>
#include <rococo.os.h>
#include <rococo.abstract.editor.h>
#include <rococo.visitors.h>
#include <rococo.strings.h>
#include <rococo.sexml.h>
#include <rococo.functional.h>
#include <rococo.properties.h>
#include <..\sexystudio\sexystudio.api.h>
#include <unordered_map>
#include <unordered_set>
#include <ctype.h>
#include <rococo.variable.editor.h>
#include <rococo.events.map.h>

using namespace Rococo;
using namespace Rococo::CFGS;
using namespace Rococo::SexyStudio;
using namespace Rococo::Windows;
using namespace Rococo::Visitors;
using namespace Rococo::Abedit;
using namespace Rococo::Events;

static const char* title = "CFGS SexyStudio IDE";
static auto evRegenerate = "EvRegenerate"_event;
static auto evFunctionChanged = "EvFunctionChanged"_event;
static auto evPropertyChanged = "EvPropertyChanged"_event;

namespace Rococo::CFGS
{
	ICFGSDesignerSpacePopupSupervisor* CreateWin32ContextPopup(IAbstractEditor& editor, ICFGSDatabase& cfgs, SexyStudio::ISexyDatabase& db);

	ROCOCO_INTERFACE ICFGSIDEContextMenu
	{
		virtual void ContextMenu_AddButton(cstr text, uint64 menuId, cstr keyCommand) = 0;
		virtual void ContextMenu_Clear() = 0;
		virtual void ContextMenu_Show() = 0;
	};

	ROCOCO_INTERFACE ICFGSIDEGui
	{
		virtual ICFGSIDEContextMenu& ContextMenu() = 0;	
		virtual void ShowAlertBox(cstr text, cstr caption) = 0;
		virtual bool GetUserConfirmation(cstr text, cstr caption) = 0;
		virtual IVariableEditor* CreateVariableEditor(const Vec2i& span, int32 labelWidth, cstr appQueryName, IVariableEditorEventHandler* eventHandler) = 0;
	};
}

namespace Rococo::CFGS::IDE::Sexy
{
	struct Sexy_CFGS_Core
	{
		SexyStudio::ISexyDatabase& db;
		ICFGSDatabase& cfgs;

		Sexy_CFGS_Core(SexyStudio::ISexyDatabase& _db, ICFGSDatabase& _cfgs) :
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

		FNameValidator(IVariableEditor& _editor, Visitors::TREE_NODE_ID id, Visitors::IUITree& _tree) : editor(_editor), parentId(id), tree(_tree)
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
			if (!parentId)
			{
				Throw(0, "Could not get parent for the namespace item");
			}

			auto match = tree.FindFirstChild(parentId, textEditorContent);

			if (match)
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

	struct TypeVector : Rococo::Reflection::IEnumVectorSupervisor
	{
		struct TypeBinding
		{
			HString enumName;
			HString enumDescription;
		};

		ISexyDatabase& sexyDB;

		std::vector<TypeBinding> typeList;

		TypeVector(ISexyDatabase& _sexyDB) : sexyDB(_sexyDB)
		{
			if (typeList.empty())
			{
				auto& root = sexyDB.GetRootNamespace();
				AppendNamespaceStructsRecursive(root);
			}
		}

		void AppendNamespaceStructsRecursive(Rococo::SexyStudio::ISxyNamespace& subspace)
		{
			char structName[256] = { 0 };
			StackStringBuilder sb(structName, sizeof structName, StringBuilder::CursorState::BUILD_EXISTING);

			int nTypes = subspace.TypeCount();
			for (int i = 0; i < nTypes; i++)
			{
				auto& type = subspace.GetType(i);

				subspace.AppendFullNameToStringBuilder(sb);
				sb.AppendChar('.');
				sb << type.PublicName();
				typeList.push_back({ structName, structName });

				sb.Clear();
			}

			int nSubspaces = subspace.SubspaceCount();
			for (int i = 0; i < nSubspaces; i++)
			{
				auto& subspaceChild = subspace[i];
				AppendNamespaceStructsRecursive(subspaceChild);
			}
		}

		void Free() override
		{
			delete this;
		}

		size_t Count() const override
		{
			return typeList.size();
		}

		bool GetEnumName(size_t i, Strings::IStringPopulator& populator) const override
		{
			if (i >= typeList.size())
			{
				return false;
			}

			populator.Populate(typeList[i].enumName);

			return true;
		}

		bool GetEnumDescription(size_t i, Strings::IStringPopulator& populator) const override
		{
			if (i >= typeList.size())
			{
				return false;
			}

			populator.Populate(typeList[i].enumDescription);

			return true;
		}
	};

	struct TypeOptions : Rococo::Reflection::IEnumDescriptor
	{
		ISexyDatabase& sexyDB;
		bool isInputElseOutput;

		TypeOptions(ISexyDatabase& _sexyDB, bool _isInputElseOutput) :
			sexyDB(_sexyDB),
			isInputElseOutput(_isInputElseOutput)
		{

		}

		Rococo::Reflection::IEnumVectorSupervisor* CreateEnumList() override
		{
			return new TypeVector(sexyDB);
		}
	};

	struct NavigationHandler : ICFGSIDENavigation, Visitors::ITreeControlHandler
	{
		MessageMap<NavigationHandler> messageMap;

		TREE_NODE_ID localFunctionsId = { 0 };
		TREE_NODE_ID namespacesId = { 0 };
		TREE_NODE_ID variablesId = { 0 };

		IAbstractEditor& editor;
		ICFGSDatabase& cfgs;
		ISexyDatabase& sexyDB;

		std::unordered_set<TREE_NODE_ID, TREE_NODE_ID::Hasher> namespaceIdSet;
		std::unordered_map<TREE_NODE_ID, FunctionId, TREE_NODE_ID::Hasher> localFunctionMap;
		std::unordered_map<TREE_NODE_ID, FunctionId, TREE_NODE_ID::Hasher> publicFunctionMap;

		enum
		{
			CONTEXT_MENU_ID_ADD_SUBSPACE = 4001,
			CONTEXT_MENU_ID_ADD_FUNCTION,
			CONTEXT_MENU_DELETE_FUNCTION,
			CONTEXT_MENU_ID_RENAME_NAMESPACE,
			CONTEXT_MENU_ID_RENAME_FUNCTION
		};

		TREE_NODE_ID contextMenuTargetId = { 0 };

		TypeOptions inputTypes;
		TypeOptions outputTypes;

		IPublisher& publisher;
		ICFGSIDEGui& gui;

		NavigationHandler(IAbstractEditor& _editor, ICFGSDatabase& _cfgs, ISexyDatabase& _db, IPublisher& _publisher, ICFGSIDEGui& _gui) :
			editor(_editor), cfgs(_cfgs), sexyDB(_db), inputTypes(_db, true), outputTypes(_db, false), publisher(_publisher), gui(_gui), messageMap(_publisher, *this)
		{
			messageMap.AddHandler(evRegenerate, &NavigationHandler::OnRegenerate);
			messageMap.AddHandler(evPropertyChanged, &NavigationHandler::OnPropertyChanged);
			messageMap.AddHandler(evFunctionChanged, &NavigationHandler::OnFunctionChanged);
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

		void OnPropertyChanged(TEventArgs<Rococo::Reflection::IPropertyEditor*>& args)
		{
			auto* f = cfgs.CurrentFunction();
			if (f)
			{
				auto& props = editor.Properties();
				props.UpdateFromVisuals(*args, f->PropertyVenue());
			}
		}

		void OnFunctionChanged(FunctionIdArg& arg)
		{
			auto* f = cfgs.CurrentFunction();
			if (f && f->Id() == arg.functionId)
			{
				RegenerateProperties();
			}
		}

		void OnRegenerate(EventArgs&)
		{
			RegenerateProperties();
		}

		void RegenerateProperties()
		{
			auto* f = cfgs.CurrentFunction();
			if (f)
			{
				f->SetInputTypeOptions(&inputTypes);
				f->SetOutputTypeOptions(&outputTypes);
				editor.Properties().Clear();
				editor.Properties().BuildEditorsForProperties(f->PropertyVenue());
			}

			editor.RefreshSlate();
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

			RegenerateProperties();
		}

		void OnItemRightClicked(TREE_NODE_ID id, IUITree& tree) override
		{
			if (id == localFunctionsId)
			{
				Vec2i span{ 800, 120 };
				int labelWidth = 120;

				char fname[128] = { 0 };

				AutoFree<IVariableEditor> fnameEditor = gui.CreateVariableEditor(span, labelWidth, "CFGS Sexy IDE - Add a new local function...", &nullEventHandler);

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

				AutoFree<IVariableEditor> nsEditor = gui.CreateVariableEditor(span, labelWidth, "CFGS Sexy IDE - Add a top level namespace...", &nullEventHandler);

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
				gui.ContextMenu().ContextMenu_Clear();

				auto i = namespaceIdSet.find(id);
				if (i != namespaceIdSet.end())
				{
					gui.ContextMenu().ContextMenu_AddButton("Add subspace", CONTEXT_MENU_ID_ADD_SUBSPACE, nullptr);
					gui.ContextMenu().ContextMenu_AddButton("Add public function", CONTEXT_MENU_ID_ADD_FUNCTION, nullptr);
					gui.ContextMenu().ContextMenu_AddButton("Rename namespace", CONTEXT_MENU_ID_RENAME_NAMESPACE, nullptr);
					contextMenuTargetId = id;
				}
				else
				{
					auto j = publicFunctionMap.find(id);
					if (j != publicFunctionMap.end())
					{
						gui.ContextMenu().ContextMenu_AddButton("Delete function", CONTEXT_MENU_DELETE_FUNCTION, nullptr);
						gui.ContextMenu().ContextMenu_AddButton("Rename function", CONTEXT_MENU_ID_RENAME_FUNCTION, nullptr);
						contextMenuTargetId = id;
					}
					else
					{
						auto k = localFunctionMap.find(id);
						if (k != localFunctionMap.end())
						{
							gui.ContextMenu().ContextMenu_AddButton("Delete function", CONTEXT_MENU_DELETE_FUNCTION, nullptr);
							gui.ContextMenu().ContextMenu_AddButton("Rename function", CONTEXT_MENU_ID_RENAME_FUNCTION, nullptr);
							contextMenuTargetId = id;
						}
						else
						{
							return;
						}
					}
				}

				gui.ContextMenu().ContextMenu_Show();
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
				gui.ShowAlertBox("Internal error. Could not identify the selected namespace", "CFGS Sexy IDE - Algorithmic Error");
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

				AutoFree<IVariableEditor> nsEditor = gui.CreateVariableEditor(span, labelWidth, ntitle, &nullEventHandler);

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
			while (childId)
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
				gui.ShowAlertBox("Internal error. Could not identify the selected namespace", "CFGS Sexy IDE - Algorithmic Error");
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

				AutoFree<IVariableEditor> nsEditor = gui.CreateVariableEditor(span, labelWidth, ntitle, &nullEventHandler);

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
					gui.ShowAlertBox("Internal error. Could not identify the selected function", "CFGS Sexy IDE - Algorithmic Error");
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

				AutoFree<IVariableEditor> nsEditor = gui.CreateVariableEditor(span, labelWidth, ntitle, &nullEventHandler);

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
					gui.ShowAlertBox("Internal error. Could not identify the selected function", "CFGS Sexy IDE - Algorithmic Error");
					return;
				}
			}

			char functionName[128];
			if (tree.TryGetText(functionName, sizeof functionName, functionId))
			{
				char caption[256];
				SafeFormat(caption, "%s - Delete function %s...", title, functionName);
				if (gui.GetUserConfirmation("Confirm deletion", caption))
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

			char* temp = (char*)_alloca(sizeOfBuffer);

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
				gui.ShowAlertBox("Internal error. Could not identify the selected namespace", "CFGS Sexy IDE - Algorithmic Error");
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

				AutoFree<IVariableEditor> nsEditor = gui.CreateVariableEditor(span, labelWidth, ntitle, &nullEventHandler);

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

		void SaveNamespace(Rococo::Sex::SEXML::ISEXMLBuilder& sb, TREE_NODE_ID namespaceId)
		{
			auto& tree = editor.NavigationTree();

			TREE_NODE_ID childId = tree.FindFirstChild(namespaceId, nullptr);
			while (childId.value)
			{
				if (namespaceIdSet.find(childId) != namespaceIdSet.end())
				{
					char subspace[128];
					if (tree.TryGetText(subspace, sizeof subspace, childId))
					{
						sb.AddDirective("Subspace");
						sb.AddAtomicAttribute("name", subspace);

						SaveNamespace(sb, childId);

						sb.CloseDirective();
					}
				}

				if (publicFunctionMap.find(childId) != publicFunctionMap.end())
				{
					char name[128];
					if (tree.TryGetText(name, sizeof name, childId))
					{
						sb.AddDirective("Function");
						sb.AddAtomicAttribute("name", name);
						sb.CloseDirective();
					}
				}

				childId = tree.FindNextChild(childId, nullptr);
			}
		}

		const char* const DIRECTIVE_LOCALFUNCTIONS = "LocalFunctions";
		const char* const DIRECTIVE_NAMESPACES = "Namespaces";
		const char* const DIRECTIVE_SUBSPACE = "Subspace";

		void LoadNamespace(const Rococo::Sex::SEXML::ISEXMLDirective& parentNamespace, TREE_NODE_ID parentSpaceId)
		{
			auto& tree = editor.NavigationTree();

			size_t startIndex = 0;
			for (;;)
			{
				auto* subspace = parentNamespace.FindFirstChild(IN OUT startIndex, DIRECTIVE_SUBSPACE);
				if (!subspace)
				{
					return;
				}
				startIndex++;

				auto& subspaceName = Sex::SEXML::AsString((*subspace)["name"]);
				auto subspaceId = tree.AddChild(parentSpaceId, subspaceName.c_str(), Visitors::CheckState_NoCheckBox);

				LoadNamespace(*subspace, subspaceId);
			}
		}

		void AddFunctionToTree(const ICFGSFunction& f)
		{
			auto& tree = editor.NavigationTree();

			cstr fname = f.Name();
			cstr tokenStart = fname;

			cstr dotPos = Strings::FindChar(fname, '.');
			if (!dotPos)
			{
				auto localId = tree.AddChild(localFunctionsId, fname, CheckState_NoCheckBox);
				localFunctionMap.insert(std::make_pair(localId, f.Id()));
				return;
			}

			TREE_NODE_ID parentId = namespacesId;

			for (;;)
			{
				Substring subspaceRange{ tokenStart, dotPos };

				char subspace[128];
				subspaceRange.CopyWithTruncate(subspace, sizeof subspace);

				auto subspaceId = tree.FindFirstChild(parentId, subspace);
				if (!subspaceId)
				{
					subspaceId = tree.AddChild(parentId, subspace, Visitors::CheckState_NoCheckBox);
				}

				parentId = subspaceId;

				cstr nextToken = dotPos + 1;

				cstr nextDotPos = Strings::FindChar(nextToken, '.');

				if (nextDotPos == nullptr)
				{
					auto publicId = tree.AddChild(parentId, nextToken, Visitors::CheckState_NoCheckBox);
					publicFunctionMap.insert(std::make_pair(publicId, f.Id()));
					return;
				}

				tokenStart = nextToken;
				dotPos = nextDotPos;
			}
		}

		void LoadNavigation(const Rococo::Sex::SEXML::ISEXMLDirective& directive)
		{
			size_t startIndex = 0;
			auto& namespaces = directive.GetDirectivesFirstChild(IN OUT startIndex, DIRECTIVE_NAMESPACES);
			LoadNamespace(namespaces, namespacesId);

			cfgs.ForEachFunction(
				[this](ICFGSFunction& f)
				{
					AddFunctionToTree(f);
				}
			);
		}

		void SaveNavigation(Rococo::Sex::SEXML::ISEXMLBuilder& sb)
		{
			auto& tree = editor.NavigationTree();

			sb.AddDirective(DIRECTIVE_LOCALFUNCTIONS);

			TREE_NODE_ID childId = tree.FindFirstChild(localFunctionsId, nullptr);
			while (childId.value)
			{
				char name[128];
				if (tree.TryGetText(name, sizeof name, childId))
				{
					sb.AddDirective("Function");
					sb.AddAtomicAttribute("name", name);
					sb.CloseDirective();
				}

				childId = tree.FindNextChild(childId, nullptr);
			}

			sb.CloseDirective();

			sb.AddDirective(DIRECTIVE_NAMESPACES);

			SaveNamespace(sb, namespacesId);

			sb.CloseDirective();
		}

		void Free()
		{
			delete this;
		}
	};

	bool IsConnectionPermitted(const CableConnection& anchor, const ICFGSSocket& target, ICFGSDatabase& cfgs, const ISexyDatabase& db)
	{
		auto* f = cfgs.CurrentFunction();
		if (!f)
		{
			return false;
		}

		auto* srcNode = f->Nodes().FindNode(anchor.node);
		if (!srcNode)
		{
			return false;
		}

		auto* srcSocket = srcNode->FindSocket(anchor.socket);
		if (!srcSocket)
		{
			return false;
		}

		cstr srcType = srcSocket->Type().Value;
		cstr trgType = target.Type().Value;

		if (!db.AreTypesEquivalent(srcType, trgType))
		{
			return false;
		}

		return true;
	}
}