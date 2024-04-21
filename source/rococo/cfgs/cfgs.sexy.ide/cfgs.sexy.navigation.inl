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
#include <rococo.validators.h>

using namespace Rococo;
using namespace Rococo::CFGS;
using namespace Rococo::SexyStudio;
using namespace Rococo::Visitors;
using namespace Rococo::Abedit;
using namespace Rococo::Events;
using namespace Rococo::Reflection;
using namespace Rococo::Validators;

static const char* title = "CFGS SexyStudio IDE";

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

		RenameFNameValidator(IVariableEditor& _editor, TREE_NODE_ID _id, IUITree& _tree) : editor(_editor), id(_id), tree(_tree)
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

		void AppendNamespaceStructsRecursive(ISxyNamespace& subspace)
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

		bool GetEnumName(size_t i, IStringPopulator& populator) const override
		{
			if (i >= typeList.size())
			{
				return false;
			}

			populator.Populate(typeList[i].enumName);

			return true;
		}

		bool GetEnumDescription(size_t i, IStringPopulator& populator) const override
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

	struct VisitorId
	{
		char buf[128];
		operator cstr() { return buf; }
	};

	VisitorId FormatWithId(cstr prefix, UniqueIdHolder idHolder)
	{
		VisitorId id;
		SecureFormat(id.buf, "%s_%llx_%llx", prefix, idHolder.iValues[0], idHolder.iValues[1]);
		return id;
	}

	VisitorId FormatWithId(cstr prefix, cstr subprefix, UniqueIdHolder idHolder)
	{
		VisitorId id;
		SecureFormat(id.buf, "%s_%s_%llx_%llx", prefix, subprefix, idHolder.iValues[0], idHolder.iValues[1]);
		return id;
	}

	struct TargetNode : IPropertyVenue, IPropertyUIEvents
	{
		ICFGSNode* node = nullptr;
		ISexyDatabase& db;

		TargetNode(ISexyDatabase& _db): db(_db)
		{

		}

		void CallArrayMethod(cstr arrayId, Function<void(IArrayProperty&)> callback) override
		{
			UNUSED(arrayId);
			UNUSED(callback);
		}

		void OnBooleanButtonChanged(IPropertyEditor& property) override
		{
			UNUSED(property);
		}

		void OnPropertyEditorLostFocus(IPropertyEditor& property) override
		{
			UNUSED(property);
		}

		void OnDeleteSection(cstr sectionId) override
		{
			UNUSED(sectionId);
		}

		void OnDependentVariableChanged(cstr propertyId, IEstateAgent& agent) override
		{
			UNUSED(propertyId);
			UNUSED(agent);
		}

		bool IsPrimitive(cstr type) const
		{
			if (Eq(type, "Float32") || Eq(type, "Sys.Type.Float32"))
			{
				return true;
			}
			else if (Eq(type, "Float64") || Eq(type, "Sys.Type.Float64"))
			{
				return true;
			}
			else if (Eq(type, "Int32") || Eq(type, "Sys.Type.Int32"))
			{
				return true;
			}
			else if (Eq(type, "Int64") || Eq(type, "Sys.Type.Int64"))
			{
				return true;
			}
			else if (Eq(type, "Bool") || Eq(type, "Sys.Type.Bool"))
			{
				return true;
			}
			else
			{
				return false;
			}
		}

		VisitorId FieldId(cstr type, cstr variableName, cstr overrideType)
		{
			cstr trueType = overrideType ? overrideType : type;

			Substring fqTypeName = Substring::ToSubstring(trueType);
			cstr publicName = ReverseFind('.', fqTypeName);
			if (!publicName)
			{
				publicName = trueType;
			}
			else
			{
				publicName++;
			}

			VisitorId id;
			SafeFormat(id.buf, "%s %s", publicName, variableName);
			return id;
		}

		bool TryVisitPrimitive(const ICFGSSocket& socket, IPropertyVisitor& visitor, cstr type, cstr variableName, cstr displayTypeOverride)
		{
			if (Eq(type, "Float32") || Eq(type, "Sys.Type.Float32"))
			{
				float fValue = 0.0f;

				PopulationBuffer<64> fBuffer;
				if (socket.TryGetField(variableName, fBuffer))
				{
					_CRT_FLOAT f;
					if (0 == _atoflt(&f, fBuffer))
					{
						fValue = f.f;
					}
				}
				MARSHAL_PRIMITIVE(visitor, FormatWithId("Field", variableName, socket.Id()), FieldId("Float32", variableName, displayTypeOverride), *this, REF fValue, AllFloatsAreValid(), FloatDecimals());

				if (visitor.IsWritingToReferences())
				{
					SafeFormat(fBuffer.data, "%f", fValue);
					socket.SetField(variableName, fBuffer);
				}
			}
			else if (Eq(type, "Float64") || Eq(type, "Sys.Type.Float64"))
			{
				double dValue = 0.0;

				PopulationBuffer<64> dBuffer;
				if (socket.TryGetField(variableName, dBuffer))
				{
					_CRT_DOUBLE f;
					if (0 == _atodbl(&f, dBuffer.data))
					{
						dValue = f.x;
					}
				}

				MARSHAL_PRIMITIVE(visitor, FormatWithId("Field", variableName, socket.Id()), FieldId("Float64", variableName, displayTypeOverride), *this, REF dValue, AllDoublesAreValid(), DoubleDecimals());

				if (visitor.IsWritingToReferences())
				{
					SafeFormat(dBuffer.data, "%llf", dValue);
					socket.SetField(variableName, dBuffer);
				}
			}
			else if (Eq(type, "Int32") || Eq(type, "Sys.Type.Int32"))
			{
				int32 iValue = 0;

				PopulationBuffer<64> iBuffer;
				if (socket.TryGetField(variableName, iBuffer))
				{
					iValue = atoi(iBuffer);
				}

				MARSHAL_PRIMITIVE(visitor, FormatWithId("Field", variableName, socket.Id()), FieldId("Int32", variableName, displayTypeOverride), *this, REF iValue, AllInt32sAreValid(), Int32Decimals());

				if (visitor.IsWritingToReferences())
				{
					SafeFormat(iBuffer.data, "%d", iValue);
					socket.SetField(variableName, iBuffer);
				}
			}
			else if (Eq(type, "Int64") || Eq(type, "Sys.Type.Int64"))
			{
				int64 iValue = 0;

				PopulationBuffer<64> iBuffer;
				if (socket.TryGetField(variableName, iBuffer))
				{
					iValue = _atoi64(iBuffer);
				}

				MARSHAL_PRIMITIVE(visitor, FormatWithId("Field", variableName, socket.Id()), FieldId("Int64", variableName, displayTypeOverride), *this, REF iValue, AllInt64sAreValid(), Int64Decimals());

				if (visitor.IsWritingToReferences())
				{
					SafeFormat(iBuffer.data, "%d", iValue);
					socket.SetField(variableName, iBuffer);
				}
			}
			else if (Eq(type, "Bool") || Eq(type, "Sys.Type.Bool"))
			{
				bool truthValue = 0;

				PopulationBuffer<64> buffer;
				if (socket.TryGetField(variableName, buffer))
				{
					truthValue = Eq(buffer, "true");
				}

				MARSHAL_PRIMITIVE(visitor, FormatWithId("Field", variableName, socket.Id()), FieldId("Bool", variableName, displayTypeOverride), *this, REF truthValue, AllBoolsAreValid(), BoolFormatter());

				if (visitor.IsWritingToReferences())
				{
					SafeFormat(buffer.data, "%s", truthValue ? "true" : "false");
					socket.SetField(variableName, buffer);
				}
			}
			else
			{
				return false;
			}

			return true;
		}

		void VisitField(const ICFGSSocket& socket, IPropertyVisitor& visitor, cstr typeString, cstr variableName, int depth, int index, cstr typeSource, cstr displayTypeOverride)
		{
			UNUSED(index);

			if (TryVisitPrimitive(socket, visitor, typeString, variableName, displayTypeOverride))
			{
				return;
			}

			enum { MAX_VISIT_FIELD_DEPTH = 5};

			if (depth >= MAX_VISIT_FIELD_DEPTH)
			{
				return;
			}

			auto sid = socket.Id().id;

			char sectionId[256];
			SafeFormat(sectionId, "%s_%llx_%llx", variableName, sid.iValues[0], sid.iValues[1]);

			const ISXYLocalType* localType = nullptr;

			auto* publicType = db.FindPrimitiveOrFQType(typeString);

			if (publicType)
			{
				localType = publicType->LocalType();
				if (localType == nullptr)
				{
					VisitField(socket, visitor, publicType->PublicName(), variableName, depth + 1, index, typeSource, typeString);
					return;
				}
			}

			if (!localType)
			{
				localType = db.ResolveLocalType(typeSource, typeString);
			}

			if (!localType)
			{
				return;
			}

			visitor.VisitHeader(sectionId, typeString, variableName);

			for (int i = 0; i < localType->FieldCount(); i++)
			{
				auto field = localType->GetField(i);

				char extFieldName[128];
				SafeFormat(extFieldName, "%s.%s", variableName, field.name);

				if (IsPrimitive(field.type))
				{
					VisitField(socket, visitor, field.type, extFieldName, depth + 1, index, typeSource, nullptr);
				}
				else
				{
					auto* resolvedType = db.ResolveLocalType(localType->SourcePath(), field.type);

					if (!resolvedType)
					{
						continue;
					}

					VisitField(socket, visitor, resolvedType->LocalName(), extFieldName, depth + 1, index, resolvedType->SourcePath(), nullptr);
				}
			}
		}

		void VisitSocket(const ICFGSSocket& socket, IPropertyVisitor& visitor, cstr variableName, int index)
		{
			VisitField(socket, visitor, socket.Type().Value, variableName, 0, index, nullptr, nullptr);
		}

		void VisitVenue(IPropertyVisitor& visitor) override
		{
			if (node == nullptr) Throw(0, "%s: node should have assigned prior to the visitation", __FUNCTION__);

			auto& n = *node;

			visitor.VisitHeader(FormatWithId("NodeHeader", n.UniqueId()), "Callee", n.Type().Value);
			visitor.VisitHeader(FormatWithId("NodeHeaderBlank", n.UniqueId()), "", "");

			int inputCount = 0;
			int outputCount = 0;

			for (int32 i = 0; i < n.SocketCount(); i++)
			{
				auto& s = n[i];

				switch (s.SocketClassification())
				{
				case SocketClass::ConstInputRef:
				case SocketClass::InputRef:
				case SocketClass::InputVar:
					inputCount++;
					if (inputCount == 1)
					{
						PropertyFormatSpec spec;
						spec.hideDisplayName = true;
						spec.emphasize = true;
						visitor.VisitHeader(FormatWithId("NodeInputHeader", n.UniqueId()), "", "Inputs", spec);
					}

					VisitSocket(s, visitor, s.Name(), inputCount);
					break;
				}
			}

			if (inputCount > 0)
			{
				visitor.VisitHeader(FormatWithId("NodeInputHeaderSpacer", n.UniqueId()), "", "");
			}

			for (int32 i = 0; i < n.SocketCount(); i++)
			{
				auto& s = n[i];
				char desc[256];
				switch (s.SocketClassification())
				{
				case SocketClass::ConstOutputRef:
				case SocketClass::OutputRef:
				case SocketClass::OutputValue:
					outputCount++;
					if (outputCount == 1)
					{
						PropertyFormatSpec spec;
						spec.hideDisplayName = true;
						spec.emphasize = true;
						visitor.VisitHeader(FormatWithId("NodeOutputHeader", n.UniqueId()), "", "Outputs", spec);
					}

					SafeFormat(desc, "%s %s", s.Type().Value, s.Name());

					PropertyFormatSpec spec;
					spec.hideDisplayName = true;
					spec.emphasize = false;
					visitor.VisitHeader(FormatWithId("SocketOutHeader", s.Id()), "", desc, spec);
					break;
				}
			}
		}
	};

	struct NavigationHandler : ICFGSIDENavigation, ITreeControlHandler
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

		NodeId selectedNode;

		TargetNode targetNode;

		NavigationHandler(IAbstractEditor& _editor, ICFGSDatabase& _cfgs, ISexyDatabase& _db, IPublisher& _publisher, ICFGSIDEGui& _gui) :
			editor(_editor), cfgs(_cfgs), sexyDB(_db), inputTypes(_db, true), outputTypes(_db, false), publisher(_publisher), gui(_gui), messageMap(_publisher, *this), targetNode(_db)
		{
			messageMap.AddHandler("Regenerate"_event, &NavigationHandler::OnRegenerate);
			messageMap.AddHandler("PropertyChanged"_event, &NavigationHandler::OnPropertyChanged);
			messageMap.AddHandler("FunctionChanged"_event, &NavigationHandler::OnFunctionChanged);
			messageMap.AddHandler("NodeSelected"_event, &NavigationHandler::OnNodeSelected);
			messageMap.AddHandler("TryDeleteNode"_event, &NavigationHandler::OnTryDeleteNode);
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

		void OnNodeSelected(TEventArgs<NodeId>& args)
		{
			selectedNode = args;
			RegenerateProperties();
		}

		void OnPropertyChanged(TEventArgs<IPropertyEditor*>& args)
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

		void OnTryDeleteNode(EventArgs&)
		{
			if (targetNode.node != nullptr)
			{
				auto* f = cfgs.CurrentFunction();
				if (f)
				{
					auto id = targetNode.node->UniqueId();

					cstr nodeType = targetNode.node->Type().Value;

					char caption[256];
					SafeFormat(caption, "%s - Delete node %s?", title, nodeType);
					if (gui.GetUserConfirmation("Confirm deletion", caption))
					{
						targetNode.node = nullptr;
						f->Nodes().Builder().DeleteNode(id);
						editor.RefreshSlate();
					}
				}
			}
		}

		void RegenerateProperties()
		{
			auto* f = cfgs.CurrentFunction();
			if (f)
			{
				if (!selectedNode)
				{
					targetNode.node = nullptr;
					editor.Properties().Clear();
					f->SetInputTypeOptions(&inputTypes);
					f->SetOutputTypeOptions(&outputTypes);
					editor.Properties().BuildEditorsForProperties(f->PropertyVenue());
				}
				else
				{
					auto* node = f->Nodes().FindNode(selectedNode);
					if (!node)
					{
						targetNode.node = nullptr;
						editor.Properties().Clear();
						selectedNode = NodeId();
						RegenerateProperties();
						return;
					}
					else
					{
						if (targetNode.node != node)
						{
							targetNode.node = node;
							editor.Properties().Clear();
							editor.Properties().BuildEditorsForProperties(targetNode);
						}
					}
				}
			}

			editor.RefreshSlate();
		}

		void SelectFunction(cstr fqName)
		{
			FunctionId id;
			cfgs.ForEachFunction([&id, fqName](ICFGSFunction& f)
				{
					if (Eq(f.Name(), fqName))
					{
						id = f.Id();
					}				
				}
			);

			if (id)
			{
				cfgs.BuildFunction(id);
				RegenerateProperties();
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
				SafeFormat(caption, "%s - Delete function %s?", title, functionName);
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