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
#include "cfgs.sexy.ide.h"

using namespace Rococo;
using namespace Rococo::CFGS;
using namespace Rococo::SexyStudio;
using namespace Rococo::Visitors;
using namespace Rococo::Abedit;
using namespace Rococo::Events;
using namespace Rococo::Reflection;
using namespace Rococo::Validators;
using namespace Rococo::Strings;
using namespace Rococo::Sex;

static const char* title = "CFGS SexyStudio IDE";

namespace Rococo::CFGS
{
	ICFGSSexyPopup* CreateWin32ContextPopup(IAbstractEditor& editor, ICFGSDatabase& cfgs, SexyStudio::ISexyDatabase& db, INamespaceValidator& namespaceValidator, ICFGSCosmetics& cosmetics, ICFGSDesignerSpacePopupPopulator& populator);

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
		virtual bool TryGetTypeFromSelectorBox(cstr text, cstr caption, cstr defaultType, char* resultBuffer,  size_t sizeofResultBuffer) = 0;
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

			if (*textEditorContent < 'A' || *textEditorContent > 'Z')
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

	struct VariableValidator : IStringValidator
	{
		IVariableEditor& editor;
		Visitors::TREE_NODE_ID parentId;
		Visitors::IUITree& tree;

		VariableValidator(IVariableEditor& _editor, Visitors::TREE_NODE_ID id, Visitors::IUITree& _tree) : editor(_editor), parentId(id), tree(_tree)
		{

		}

		bool ValidateAndReportErrors(cstr textEditorContent, cstr variableName) override
		{
			if (*textEditorContent == 0)
			{
				editor.SetHintError(variableName, "The name is blank");
				return false;
			}

			if (*textEditorContent < 'a' || *textEditorContent > 'z')
			{
				editor.SetHintError(variableName, "The first character of a subspace name must be a lower case a-z");
				return false;
			}

			for (cstr p = textEditorContent + 1; *p != 0; p++)
			{
				if (!isalnum(*p))
				{
					editor.SetHintError(variableName, "The trailing characters of a variable name must be alpha numerics. A-Z, a-z or 0-9");
					return false;
				}
			}

			if (0 != tree.FindFirstChild(parentId, textEditorContent).value)
			{
				char msg[256];

				char vname[128];
				if (tree.TryGetText(vname, sizeof vname, parentId))
				{
					SafeFormat(msg, "A variable with that name already exists");
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
		IPublisher& publisher;

		TargetNode(ISexyDatabase& _db, IPublisher& _publisher): db(_db), publisher(_publisher)
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
			publisher.PostOneArg(&property, "PropertyChanged"_event);
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

		bool TryVisitPrimitive(ICFGSSocket& socket, IPropertyVisitor& visitor, cstr type, cstr variableName, cstr displayTypeOverride)
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

		void VisitField(ICFGSSocket& socket, IPropertyVisitor& visitor, cstr typeString, cstr variableName, int depth, int index, cstr typeSource, cstr displayTypeOverride)
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

			auto* interfaceType = db.FindInterface(typeString);
			if (interfaceType)
			{
				if (Eq(typeString, "Sys.Type.IString") || Eq(typeString, "IString") || Eq(typeString, "Type.IString"))
				{
					char displaytext[128];
					SafeFormat(displaytext, "IString %s", variableName);

					HString defaultString;

					HStringPopulator populator(REF defaultString);
					if (!socket.TryGetField(variableName, populator))
					{
						Format(defaultString, "%s: %s has been defined as this crud", node->Type().Value, variableName);
					}

					MARSHAL_STRING(visitor, sectionId, displaytext, *this, defaultString, 4096);

					if (visitor.IsWritingToReferences())
					{
						socket.SetField(variableName, defaultString);
					}
				}
				else
				{
					char displaytext[128];
					SafeFormat(displaytext, "%s %s = <null %s>", typeString, variableName, interfaceType->PublicName());

					PropertyFormatSpec spec;
					spec.emphasize = false;
					spec.hideDisplayName = true;
					visitor.VisitHeader(sectionId, "", displaytext, spec);
				}
				return;
			}

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

			if (!localType && typeSource)
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

			SafeFormat(sectionId, "%s_%llx_%llx_blank", variableName, sid.iValues[0], sid.iValues[1]);
			visitor.VisitHeader(sectionId, "", "");
		}

		void VisitSocket(ICFGSSocket& socket, IPropertyVisitor& visitor, cstr variableName, int index)
		{
			VisitField(socket, visitor, socket.Type().Value, variableName, 0, index, nullptr, nullptr);
		}

		void VisitVenue(IPropertyVisitor& visitor) override
		{
			if (node == nullptr) Throw(0, "%s: node should have been assigned prior to the visitation", __ROCOCO_FUNCTION__);

			auto& n = *node;

			visitor.VisitHeader(FormatWithId("NodeHeader", n.Id()), "Callee", n.Type().Value);
			visitor.VisitHeader(FormatWithId("NodeHeaderBlank", n.Id()), "", "");

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
						visitor.VisitHeader(FormatWithId("NodeInputHeader", n.Id()), "", "Inputs", spec);
					}

					VisitSocket(s, visitor, s.Name(), inputCount);
					break;
				}
			}

			if (inputCount > 0)
			{
				visitor.VisitHeader(FormatWithId("NodeInputHeaderSpacer", n.Id()), "", "");
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
						visitor.VisitHeader(FormatWithId("NodeOutputHeader", n.Id()), "", "Outputs", spec);
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

	struct VariableDef
	{
		HString name;
		HString type;
		HString defaultValue;
	};

	struct NavigationHandler : ICFGSIDENavigation, ITreeControlHandler, ICFGSVariableEnumerator
	{
		MessageMap<NavigationHandler> messageMap;

		TREE_NODE_ID privateMethodsId = { 0 };
		TREE_NODE_ID publicMethodsId = { 0 };
		TREE_NODE_ID variablesId = { 0 };

		IAbstractEditor& editor;
		ICFGSDatabase& cfgs;
		ISexyDatabase& sexyDB;
		ICFGSSexyPopup& popup;
		ICFGSCosmetics& cosmetics;

		std::unordered_map<TREE_NODE_ID, FunctionId, TREE_NODE_ID::Hasher> privateMethodsMap;
		std::unordered_map<TREE_NODE_ID, FunctionId, TREE_NODE_ID::Hasher> publicMethodsMap;
		std::unordered_map<TREE_NODE_ID, VariableDef, TREE_NODE_ID::Hasher> variableIdSet;

		enum
		{
			CONTEXT_MENU_ID_ADD_SUBSPACE = 4001,
			CONTEXT_MENU_ID_ADD_FUNCTION,
			CONTEXT_MENU_DELETE_FUNCTION,
			CONTEXT_MENU_ID_RENAME_FUNCTION,
			CONTEXT_MENU_CHANGE_VARIABLE_TYPE,
			CONTEXT_MENU_CHANGE_VARIABLE_DEFAULT,
			CONTEXT_MENU_SET_TEMPLATE
		};

		TREE_NODE_ID contextMenuTargetId = { 0 };
		TREE_NODE_ID templateId = { 0 };

		TypeOptions inputTypes;
		TypeOptions outputTypes;

		IPublisher& publisher;
		ICFGSIDEGui& gui;

		NodeId selectedNode;

		TargetNode targetNode;

		NavigationHandler(IAbstractEditor& _editor, ICFGSDatabase& _cfgs, ISexyDatabase& _db, IPublisher& _publisher, ICFGSIDEGui& _gui, ICFGSSexyPopup& _popup, ICFGSCosmetics& _cosmetics) :
			editor(_editor), cfgs(_cfgs), sexyDB(_db), popup(_popup), inputTypes(_db, true), outputTypes(_db, false), publisher(_publisher), gui(_gui), messageMap(_publisher, *this), targetNode(_db, _publisher), cosmetics(_cosmetics)
		{
			messageMap.AddHandler("Regenerate"_event, &NavigationHandler::OnRegenerate);
			messageMap.AddHandler("PropertyChanged"_event, &NavigationHandler::OnPropertyChanged);
			messageMap.AddHandler("FunctionChanged"_event, &NavigationHandler::OnFunctionChanged);
			messageMap.AddHandler("NodeSelected"_event, &NavigationHandler::OnNodeSelected);
			messageMap.AddHandler("TryDeleteNode"_event, &NavigationHandler::OnTryDeleteNode);
			messageMap.AddHandler("CableDropped"_event, &NavigationHandler::OnCableDropped);
			messageMap.AddHandler("BeginNodeModified"_event, &NavigationHandler::OnBeginNodeModified);
			messageMap.AddHandler("ReturnNodeModified"_event, &NavigationHandler::OnReturnNodeModified);
		}

		~NavigationHandler()
		{
			for (auto& i : privateMethodsMap)
			{
				cfgs.DeleteFunction(i.second);
			}

			for (auto& i : publicMethodsMap)
			{
				cfgs.DeleteFunction(i.second);
			}
		}

		void Clear()
		{
			targetNode.node = nullptr;
			selectedNode = NodeId();
			RefreshNavigationTree();
		}

		void ForEachVariable(Function<void(cstr name, cstr type, cstr dfeaultValue)> callback) override
		{
			TREE_NODE_ID child = editor.NavigationTree().FindFirstChild(variablesId, nullptr);
			while (child)
			{
				auto i = variableIdSet.find(child);
				if (i != variableIdSet.end())
				{
					callback.Invoke(i->second.name, i->second.type, i->second.defaultValue);
				}

				child = editor.NavigationTree().FindNextChild(child, nullptr);
			}
		}

		ICFGSSocket* FindSocket(ICFGSNode& node, cstr name)
		{
			for (int i = 0; i < node.SocketCount(); ++i)
			{
				auto& s = node[i];
				if (Eq(s.Name(), name))
				{
					return &s;
				}
			}

			return nullptr;
		}

		// Assumes n is a begin node
		void SyncBeginNode(ICFGSNode& n, ICFGSFunction& f)
		{
			auto& requiredSpec = f.BeginNode();

			std::vector<SocketId> obsoleteSockets;

			for (int j = 0; j < n.SocketCount(); j++)
			{
				auto& s = n[j];
				cstr name = s.Name();

				if (IsOutputClass(s.SocketClassification()))
				{
					auto* requiredSocket = FindSocket(requiredSpec, name);
					if (!requiredSocket)
					{
						// Our template does not have the named socket, so we must remove it from our begin node
						obsoleteSockets.push_back(s.Id());
					}
					else
					{
						s.SetType(requiredSocket->Type());
						s.SetColours({ requiredSocket->GetSocketColour(false), requiredSocket->GetSocketColour(true) });
					}
				}
			}

			for (auto& id : obsoleteSockets)
			{
				n.DeleteSocket(id);
			}

			for (int j = 0; j < requiredSpec.SocketCount(); j++)
			{
				auto& requiredSocket = requiredSpec[j];
				if (IsInputClass(requiredSocket.SocketClassification()))
				{
					cstr name = requiredSocket.Name();

					auto* nodeSocket = FindSocket(n, name);
					if (!nodeSocket)
					{
						auto classification = FlipInputOutputClass(requiredSocket.SocketClassification());
						if (classification != SocketClass::None)
						{
							auto& socket = n.AddSocket(requiredSocket.Type().Value, classification, name, SocketId());
							auto colours = cosmetics.GetColoursForType(requiredSocket.Type().Value);
							socket.SetColours(colours);
						}
					}
				}
			}
		}

		void OnBeginNodeModified(TEventArgs<FunctionId>& functionId)
		{
			auto* f = cfgs.FindFunction(functionId);
			if (!f)
			{
				return;
			}

			for (int i = 0; i < f->Nodes().Count(); ++i)
			{
				auto& n = f->Nodes()[i];
				if (Eq(n.Type().Value, "<Begin>"))
				{
					// Update begin node
					SyncBeginNode(n, *f);

					// We are only expecting one begin node per graph, so we break rather than continue
					break;
				}
			}

			editor.RefreshSlate();
		}

		// Assumes n is a return node
		void SyncReturnNode(ICFGSNode& n, ICFGSFunction& f)
		{
			auto& requiredSpec = f.ReturnNode();

			std::vector<SocketId> obsoleteSockets;

			for (int j = 0; j < n.SocketCount(); j++)
			{
				auto& s = n[j];
				cstr name = s.Name();

				if (IsInputClass(s.SocketClassification()))
				{
					auto* requiredSocket = FindSocket(requiredSpec, name);
					if (!requiredSocket)
					{
						// Our template does not have the named socket, so we must remove it from our begin node
						obsoleteSockets.push_back(s.Id());
					}
					else
					{
						s.SetType(requiredSocket->Type());
						s.SetColours({ requiredSocket->GetSocketColour(false), requiredSocket->GetSocketColour(true) });
					}
				}
			}

			for (auto& id : obsoleteSockets)
			{
				n.DeleteSocket(id);
			}

			for (int j = 0; j < requiredSpec.SocketCount(); j++)
			{
				auto& requiredSocket = requiredSpec[j];
				if (IsOutputClass(requiredSocket.SocketClassification()))
				{
					cstr name = requiredSocket.Name();

					auto* nodeSocket = FindSocket(n, name);
					if (!nodeSocket)
					{
						auto classification = FlipInputOutputClass(requiredSocket.SocketClassification());
						if (classification != SocketClass::None)
						{
							auto& socket = n.AddSocket(requiredSocket.Type().Value, classification, name, SocketId());
							auto colours = cosmetics.GetColoursForType(requiredSocket.Type().Value);
							socket.SetColours(colours);
						}
					}
				}
			}
		}

		void OnReturnNodeModified(TEventArgs<FunctionId>& functionId)
		{
			auto* f = cfgs.FindFunction(functionId);
			if (!f)
			{
				return;
			}

			for (int i = 0; i < f->Nodes().Count(); ++i)
			{
				auto& n = f->Nodes()[i];
				if (Eq(n.Type().Value, "<Return>"))
				{
					SyncReturnNode(n, *f);

					// There may be more than one return node, so we continue rather than break
					continue;
				}
			}

			editor.RefreshSlate();
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

				if (targetNode.node != nullptr)
				{
					props.UpdateFromVisuals(*args, targetNode);
				}
				else
				{
					props.UpdateFromVisuals(*args, f->PropertyVenue());
					publisher.PostOneArg(f->Id(), "BeginNodeModified"_event);
					publisher.PostOneArg(f->Id(), "ReturnNodeModified"_event);
				}
			}
		}

		void OnCableDropped(CableDropped& args)
		{
			auto* f = cfgs.CurrentFunction();
			if (!f || f->Id() != args.functionId)
			{
				return;
			}

			auto* node = f->Nodes().FindNode(args.anchor.node);
			if (!node)
			{
				return;
			}

			auto* socket = node->FindSocket(args.anchor.socket);
			if (!socket)
			{
				return;
			}

			cstr sexyType = socket->Type().Value;

			const ISxyNamespace* pNamespace = nullptr;
			auto* pInterface = sexyDB.FindInterface(sexyType, &pNamespace);
			if (!pInterface || !pNamespace)
			{
				return;
			}

			popup.ShowInterface(args.dropPoint, args.designPoint, *pInterface, *pNamespace, args);
		}

		void OnFunctionChanged(TEventArgs<FunctionId>& arg)
		{
			auto* f = cfgs.CurrentFunction();
			if (f && f->Id() == arg)
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
					auto id = targetNode.node->Id();

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

		void AddBeginNodeIfMissing(ICFGSFunction& f)
		{
			Editors::DesignerVec2 topLeft{ 1.0e40, 1.0e40 };

			cstr beginNodeType = "<Begin>";

			if (f.Nodes().Count() == 0)
			{
				topLeft = { -100, -100 };
			}

			for (int i = 0; i < f.Nodes().Count(); i++)
			{
				auto& node = f.Nodes()[i];
				if (Eq(f.Nodes()[i].Type().Value, beginNodeType))
				{
					return;
				}

				Editors::DesignerRect rect = node.GetDesignRectangle();

				topLeft.x = min(rect.left, topLeft.x);
				topLeft.y = min(rect.top, topLeft.y);
			}

			// We are missing a Begin node
			topLeft.x = min(topLeft.x, 1.0e40);
			topLeft.y = min(topLeft.y, 1.0e40);

			topLeft.x -= 120.0;
			topLeft.y -= 120.0;

			auto& beginNode = f.Nodes().Builder().AddNode(beginNodeType, topLeft, NodeId());

			auto& flowOut = beginNode.AddSocket("Flow", SocketClass::Exit, "Execute", SocketId());
			flowOut.SetColours(cosmetics.GetColoursForType("__Flow"));

			// The input nodes for the function are the output nodes for the beginNode
			auto& outputSpec = f.BeginNode();
			for (int i = 0; i < outputSpec.SocketCount(); i++)
			{
				auto& output = outputSpec[i];
				cstr outputType = output.Type().Value;
				cstr outputName = output.Name();

				SocketClass outputClass = SocketClass::None;

				switch (output.SocketClassification())
				{
				case SocketClass::ConstInputRef:
					outputClass = SocketClass::ConstOutputRef;
					break;
				case SocketClass::InputVar:
					outputClass = SocketClass::OutputValue;
					break;
				case SocketClass::InputRef:
					outputClass = SocketClass::OutputRef;
					break;
				default:
					continue;
				}

				auto& outputSocket = beginNode.AddSocket(outputType, outputClass, outputName, SocketId());
				auto colours = cosmetics.GetColoursForType(outputType);
				outputSocket.SetColours(colours);
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

				AddBeginNodeIfMissing(*f);
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

			auto i = privateMethodsMap.find(id);
			if (i != privateMethodsMap.end())
			{
				cfgs.BuildFunction(i->second);
			}
			else
			{
				auto j = publicMethodsMap.find(id);
				if (j != publicMethodsMap.end())
				{
					cfgs.BuildFunction(j->second);
				}
			}

			RegenerateProperties();
		}

		void OnItemRightClicked(TREE_NODE_ID id, IUITree& tree) override
		{
			if (id == privateMethodsId)
			{
				Vec2i span{ 800, 120 };
				int labelWidth = 120;

				char fname[128] = { 0 };

				AutoFree<IVariableEditor> fnameEditor = gui.CreateVariableEditor(span, labelWidth, "CFGS Sexy IDE - Add a new private method...", &nullEventHandler);

				FNameValidator fnameValidator(*fnameEditor, id, tree);
				fnameEditor->AddStringEditor("Private Name", nullptr, fname, sizeof fname, &fnameValidator);
				if (fnameEditor->IsModalDialogChoiceYes())
				{
					char fullname[256] = "_";
					CopyString(fullname+1, sizeof fullname - 1, fname);

					auto newPrivateMethodId = tree.AddChild(privateMethodsId, fname, Visitors::CheckState_NoCheckBox);

					FunctionId f = cfgs.CreateFunction();
					cfgs.FindFunction(f)->SetName(fullname);

					privateMethodsMap.insert(std::make_pair(newPrivateMethodId, f));
					tree.Select(newPrivateMethodId);
				}
			}
			else if (id == publicMethodsId)
			{
				Vec2i span{ 800, 120 };
				int labelWidth = 120;

				char fname[128] = { 0 };

				AutoFree<IVariableEditor> nsEditor = gui.CreateVariableEditor(span, labelWidth, "CFGS Sexy IDE - Add a new public method...", &nullEventHandler);

				NamespaceValidator fnameValidator(*nsEditor, id, tree);
				nsEditor->AddStringEditor("Public Name", nullptr, fname, sizeof fname, &fnameValidator);
				if (nsEditor->IsModalDialogChoiceYes())
				{
					auto newFunctionId = tree.AddChild(publicMethodsId, fname, Visitors::CheckState_NoCheckBox);

					FunctionId f = cfgs.CreateFunction();
					cfgs.FindFunction(f)->SetName(fname);

					publicMethodsMap.insert(std::make_pair(newFunctionId, f));
					tree.Select(newFunctionId);
				}
			}
			else if (id == variablesId)
			{
				Vec2i span{ 800, 120 };
				int labelWidth = 120;

				char fname[128] = { 0 };

				AutoFree<IVariableEditor> nsEditor = gui.CreateVariableEditor(span, labelWidth, "CFGS Sexy IDE - Add a new variable...", &nullEventHandler);

				VariableValidator vnameValidator(*nsEditor, id, tree);

				nsEditor->AddStringEditor("Name", nullptr, fname, sizeof fname, &vnameValidator);
				if (nsEditor->IsModalDialogChoiceYes())
				{
					auto newVariableId = tree.AddChild(variablesId, fname, Visitors::CheckState_NoCheckBox);
					tree.AddChild(newVariableId, "Type: Int32", Visitors::CheckState_NoCheckBox);
					tree.AddChild(newVariableId, "Default: 0", Visitors::CheckState_NoCheckBox);
					variableIdSet.insert(std::make_pair(newVariableId, VariableDef { fname, "Int32", "0"}));
					tree.Select(newVariableId);
				}
			}
			else
			{
				gui.ContextMenu().ContextMenu_Clear();

				auto j = publicMethodsMap.find(id);
				if (j != publicMethodsMap.end())
				{
					gui.ContextMenu().ContextMenu_AddButton("Set template", CONTEXT_MENU_SET_TEMPLATE, nullptr);
					gui.ContextMenu().ContextMenu_AddButton("Delete function", CONTEXT_MENU_DELETE_FUNCTION, nullptr);
					gui.ContextMenu().ContextMenu_AddButton("Rename function", CONTEXT_MENU_ID_RENAME_FUNCTION, nullptr);
					contextMenuTargetId = id;
				}
				else
				{
					auto k = privateMethodsMap.find(id);
					if (k != privateMethodsMap.end())
					{
						gui.ContextMenu().ContextMenu_AddButton("Set template", CONTEXT_MENU_SET_TEMPLATE, nullptr);
						gui.ContextMenu().ContextMenu_AddButton("Delete function", CONTEXT_MENU_DELETE_FUNCTION, nullptr);
						gui.ContextMenu().ContextMenu_AddButton("Rename function", CONTEXT_MENU_ID_RENAME_FUNCTION, nullptr);
						contextMenuTargetId = id;
					}
					else
					{
						auto variableId = tree.GetParent(id);
						auto l = variableIdSet.find(variableId);
						if (l != variableIdSet.end())
						{
							char buffer[256];
							if (tree.TryGetText(buffer, sizeof buffer, id))
							{
								if (StartsWith(buffer, "Type: "))
								{
									gui.ContextMenu().ContextMenu_AddButton("Change type", CONTEXT_MENU_CHANGE_VARIABLE_TYPE, nullptr);
									contextMenuTargetId = variableId;
								}
								else if (StartsWith(buffer, "Default: "))
								{
									gui.ContextMenu().ContextMenu_AddButton("Change default value", CONTEXT_MENU_CHANGE_VARIABLE_DEFAULT, nullptr);
									contextMenuTargetId = variableId;
								}
							}
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
			privateMethodsId = t.AddRootItem("Private Methods", Visitors::CheckState_NoCheckBox);
			publicMethodsId = t.AddRootItem("Public Methods", Visitors::CheckState_NoCheckBox);
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

		void UpdateAllFunctionNames(TREE_NODE_ID namespaceId, IUITree& tree)
		{
			TREE_NODE_ID childId = tree.FindFirstChild(namespaceId, nullptr);
			while (childId)
			{
				// We have a function
				auto j = publicMethodsMap.find(childId);
				if (j != publicMethodsMap.end())
				{
					FunctionId id = j->second;

					char fname[256];
					if (tree.TryGetText(fname, sizeof fname, childId))
					{
						cfgs.FindFunction(id)->SetName(fname);
					}
				}

				childId = tree.FindNextChild(childId, nullptr);
			}
		}

		void RenameFunction(TREE_NODE_ID functionId, IUITree& tree)
		{
			auto i = publicMethodsMap.find(functionId);
			if (i == publicMethodsMap.end())
			{
				auto j = privateMethodsMap.find(functionId);
				if (j == privateMethodsMap.end())
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

					i = publicMethodsMap.find(functionId);
					if (i != publicMethodsMap.end())
					{
						id = i->second;
						namespaceId = tree.GetParent(functionId);
					}
					else
					{
						auto j = privateMethodsMap.find(functionId);
						if (j != privateMethodsMap.end())
						{
							id = j->second;
						}
					}

					if (id)
					{
						cfgs.FindFunction(id)->SetName(newFunctionName);
					}
				}
			}
		}

		void ChangeVariableType(TREE_NODE_ID variableId, IUITree& tree)
		{
			UNUSED(tree);

			auto i = variableIdSet.find(variableId);
			if (i == variableIdSet.end())
			{
				return;
			}

			auto& def = i->second;

			char caption[256];
			SafeFormat(caption, "Select a new type for %s", def.name.c_str());

			char newType[256];
			if (gui.TryGetTypeFromSelectorBox("Type: ", caption, def.type, newType, sizeof newType))
			{
				def.type = newType;

				auto typeId = tree.FindFirstChild(variableId, nullptr);

				char buffer[256];
				tree.TryGetText(buffer, sizeof buffer, typeId);

				if (StartsWith(buffer, "Type: "))
				{
					SafeFormat(buffer, "Type: %s", newType);
					tree.SetText(typeId, buffer);
				}
				else
				{
					Throw(0, "Could not find type. Unexpected!");
				}
			}
		}

		void DeleteFunction(TREE_NODE_ID functionId, IUITree& tree)
		{
			auto i = publicMethodsMap.find(functionId);
			if (i == publicMethodsMap.end())
			{
				auto j = privateMethodsMap.find(functionId);
				if (j == privateMethodsMap.end())
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
					auto k = publicMethodsMap.find(functionId);
					if (k != publicMethodsMap.end())
					{
						cfgs.DeleteFunction(k->second);
						publicMethodsMap.erase(k);
					}
					else
					{
						auto l = privateMethodsMap.find(functionId);
						if (l != privateMethodsMap.end())
						{
							cfgs.DeleteFunction(l->second);
							publicMethodsMap.erase(l);
						}
					}

					tree.Delete(functionId);
				}
			}
		}

		bool TryHandleContextMenuItem(uint16 menuCommandId)
		{
			switch (menuCommandId)
			{
			case CONTEXT_MENU_ID_RENAME_FUNCTION:
				RenameFunction(contextMenuTargetId, editor.NavigationTree());
				return true;
			case CONTEXT_MENU_DELETE_FUNCTION:
				DeleteFunction(contextMenuTargetId, editor.NavigationTree());
				return true;
			case CONTEXT_MENU_CHANGE_VARIABLE_TYPE:
				ChangeVariableType(contextMenuTargetId, editor.NavigationTree());
				return true;
			case CONTEXT_MENU_SET_TEMPLATE:
				templateId = contextMenuTargetId;
				{
					auto i = privateMethodsMap.find(templateId);
					if (i != privateMethodsMap.end())
					{
						popup.SetTemplate(i->second);
					}
					else
					{
						auto j = publicMethodsMap.find(templateId);
						if (j != publicMethodsMap.end())
						{
							popup.SetTemplate(j->second);
						}
					}
				}
				
				return true;
			}

			return false;
		}

		const char* const DIRECTIVE_LOCALFUNCTIONS = "LocalFunctions";
		const char* const DIRECTIVE_VARIABLES = "Variables";
		const char* const DIRECTIVE_VARIABLE = "Variable";

		void AddFunctionToTree(const ICFGSFunction& f)
		{
			auto& tree = editor.NavigationTree();

			cstr fname = f.Name();

			bool hasLeadingUnderscore = *fname == '_';
			if (hasLeadingUnderscore)
			{
				auto id = tree.AddChild(privateMethodsId, fname + 1, CheckState_NoCheckBox);
				privateMethodsMap.insert(std::make_pair(id, f.Id()));
			}
			else
			{
				auto id = tree.AddChild(publicMethodsId, fname, CheckState_NoCheckBox);
				publicMethodsMap.insert(std::make_pair(id, f.Id()));
			}
		}

		void LoadNavigation(const Rococo::Sex::SEXML::ISEXMLDirective& navDirective)
		{
			cfgs.ForEachFunction(
				[this](ICFGSFunction& f)
				{
					AddFunctionToTree(f);
				}
			);

			size_t startIndex = 0;
			const auto* varDirective = navDirective.FindFirstChild(REF startIndex, DIRECTIVE_VARIABLES);
			if (varDirective)
			{
				auto& tree = editor.NavigationTree();

				auto& variables = varDirective->Children();

				for (size_t i = 0; i < variables.NumberOfDirectives(); i++)
				{
					auto& variable = variables[i];

					cstr name = SEXML::AsAtomic(variable["Name"]).c_str();
					cstr type = SEXML::AsAtomic(variable["Type"]).c_str();
					cstr defaultValue = SEXML::AsAtomic(variable["Default"]).c_str();

					char typeDesc[256];
					SafeFormat(typeDesc, "Type: %s", type);

					char defaultDesc[256];
					SafeFormat(defaultDesc, "Default: %s", defaultValue);

					auto newVariableId = tree.AddChild(variablesId, name, Visitors::CheckState_NoCheckBox);
					tree.AddChild(newVariableId, typeDesc, Visitors::CheckState_NoCheckBox);
					tree.AddChild(newVariableId, defaultDesc, Visitors::CheckState_NoCheckBox);
					variableIdSet.insert(std::make_pair(newVariableId, VariableDef{ name, type, defaultValue }));
				}
			}
		}

		void SaveNavigation(Rococo::Sex::SEXML::ISEXMLBuilder& sb)
		{
			auto& tree = editor.NavigationTree();

			sb.AddDirective(DIRECTIVE_LOCALFUNCTIONS);

			TREE_NODE_ID childId = tree.FindFirstChild(privateMethodsId, nullptr);
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

			sb.AddDirective(DIRECTIVE_VARIABLES);

			ForEachVariable(
				[this, &sb](cstr name, cstr type, cstr defaultValue)
				{
					sb.AddDirective(DIRECTIVE_VARIABLE);
					sb.AddAtomicAttribute("Name", name);
					sb.AddAtomicAttribute("Type", type);
					sb.AddAtomicAttribute("Default", defaultValue);
					sb.CloseDirective();
				}
			);

			sb.CloseDirective();
		}

		void Free()
		{
			delete this;
		}
	};
}