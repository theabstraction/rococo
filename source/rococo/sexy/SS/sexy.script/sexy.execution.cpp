#include "sexy.script.stdafx.h"
#include <Sexy.S-Parser.h>
#include <rococo.sexy.ide.h>
#include <rococo.try-catch.native.h>
#include <rococo.ide.h>
#include <rococo.visitors.h>
#include <rococo.maths.h>
#include <rococo.sexy.map.expert.h>
#include <rococo.os.h>
#include <rococo.time.h>

using namespace Rococo::Sex;
using namespace Rococo::Script;
using namespace Rococo::Strings;
using namespace Rococo::Compiler;
using namespace Rococo::VM;
using namespace Rococo::Visitors;
using namespace Rococo::Debugger;

namespace Rococo
{
	const char* SanitizeStringNames(const char* s)
	{
		if (strcmp(s, "_Null_Sys_Type_IString") == 0)
		{
			return "IString";
		}
		else if (strcmp(s, "_Null_Sys_Type_IStringBuilder") == 0)
		{
			return "IStringBuilder";
		}
		return s;
	}

	void AddArguments(const IArchetype& f, Visitors::IUITree& tree, Visitors::TREE_NODE_ID typeId)
	{
		char desc[256];

		if (f.NumberOfInputs() > (f.IsVirtualMethod() ? 1 : 0))
		{
			auto inputId = tree.AddChild(typeId, "[Inputs]", Visitors::CheckState_Clear);
			for (int i = 0; i < f.NumberOfInputs(); ++i)
			{
				if (i == f.NumberOfInputs() - 1 && f.IsVirtualMethod())
				{
					// Skip the instance reference
				}
				else
				{
					auto& arg = f.GetArgument(i + f.NumberOfOutputs());
					auto argName = f.GetArgName(i + f.NumberOfOutputs());

					if (&arg != nullptr)
					{
						// Hack, some build error created a partially initialized structure.
						// => We skip it.
						SafeFormat(desc, sizeof(desc), "%s %s", SanitizeStringNames(arg.Name()), argName);
						tree.AddChild(inputId, desc, Visitors::CheckState_Clear);
					}
				}
			}
		}

		if (f.NumberOfOutputs() > 0)
		{
			auto outputId = tree.AddChild(typeId, "[Outputs]", Visitors::CheckState_Clear);

			if (f.NumberOfOutputs() == 1 && (f.NumberOfInputs() == 0 || (f.NumberOfInputs() == 1 && f.IsVirtualMethod())))
			{
				auto& arg = f.GetArgument(0);
				auto argName = f.GetArgName(0);

				SafeFormat(desc, sizeof(desc), "%s %s - Get Accessor", SanitizeStringNames(arg.Name()), argName);
				tree.AddChild(outputId, desc, Visitors::CheckState_Clear);
			}
			else
			{
				for (int i = 0; i < f.NumberOfOutputs(); ++i)
				{
					auto& arg = f.GetArgument(i);
					auto argName = f.GetArgName(i);

					SafeFormat(desc, sizeof(desc), "%s %s", SanitizeStringNames(arg.Name()), argName);
					tree.AddChild(outputId, desc, Visitors::CheckState_Clear);
				}
			}
		}
	}

	struct Reg
	{
		char key[8];
		char value[48];
	};

	void PopulateRegisterWindow(Script::IPublicScriptSystem& ss, Visitors::IUIList& registerListView)
	{
		auto& vm = ss.PublicProgramObject().VirtualMachine();

		struct : public IRegisterEnumerationCallback
		{
			enum { MAX_REGISTER_COUNT = 16 };
			Reg registers[MAX_REGISTER_COUNT];
			int count = 0;

			void OnRegister(const char* name, const char* value) override
			{
				if (count < MAX_REGISTER_COUNT)
				{
					CopyString(registers[count].key, sizeof registers[count].key, name);
					CopyString(registers[count].value, sizeof registers[count].value, value);

					count++;
				}
			}

			IUIList* uiList;
		} addToList;

		addToList.uiList = &registerListView;

		registerListView.ClearRows();

		Script::EnumerateRegisters(vm.Cpu(), addToList);

		for (int i = 0; i < addToList.count;)
		{
			Reg& reg1 = addToList.registers[i++];
			Reg reg2 = { " ", " " };
			Reg reg3 = { " ", " " };
			Reg reg4 = { " ", " " };
			if (i < addToList.count)
			{
				reg2 = addToList.registers[i++];
			}
			if (i < addToList.count)
			{
				reg3 = addToList.registers[i++];
			}
			if (i < addToList.count)
			{
				reg4 = addToList.registers[i++];
			}

			cstr row[] = { reg1.key, reg1.value, reg2.key, reg2.value, reg3.key, reg3.value, reg4.key, reg4.value, nullptr };
			registerListView.AddRow(row);
		}
	}

	const IFunction* DisassembleCallStackAndAppendToView(IDisassembler& disassembler, IDebuggerWindow& debugger, Rococo::Script::IPublicScriptSystem& ss, CPU& cpu, size_t callDepth, const ISExpression** ppExpr, const uint8** ppSF, size_t populateAtDepth)
	{
		const Rococo::uint8* sf = nullptr;
		const Rococo::uint8* pc = nullptr;
		const IFunction* f = nullptr;
		*ppExpr = nullptr;
		*ppSF = nullptr;

		size_t fnOffset;
		size_t pcOffset;
		if (!Rococo::Script::GetCallDescription(sf, pc, f, fnOffset, ss, callDepth, pcOffset) || !f)
		{
			return nullptr;
		}

		*ppSF = sf;

		if (callDepth != populateAtDepth || populateAtDepth == (size_t)-1)
		{
			return f;
		}

		CodeSection section;
		f->Code().GetCodeSection(section);

		IPublicProgramObject& po = ss.PublicProgramObject();
		IVirtualMachine& vm = po.VirtualMachine();

		size_t functionLength = po.ProgramMemory().GetFunctionLength(section.Id);

		debugger.InitDisassembly(section.Id);

		char metaData[256];
		SafeFormat(metaData, sizeof(metaData), "%s %s (Id #%d) - %d bytes\n\n", f->Name(), f->Module().Name(), (int32)section.Id, (int32)functionLength);
		debugger.AddDisassembly(DISASSEMBLY_TEXT_TYPE::HEADER, metaData);

		int lineCount = 1;

		const Rococo::uint8* fstart = vm.Cpu().ProgramStart + po.ProgramMemory().GetFunctionAddress(section.Id);

		size_t hilightIndex = pc - fstart;
		// size_t dissassembleLength = min(64_kilobytes, functionLength);
		size_t i = 0;
		while (i < functionLength)
		{
			SymbolValue symbol = f->Code().GetSymbol(i);

			IDisassembler::Rep rep;
			disassembler.Disassemble(fstart + i, rep);

			if (fstart + i <= pc)
			{
				auto* s = (const ISExpression*)symbol.SourceExpression;
				if (s)
				{
					*ppExpr = s;
				}
			}

			bool isHighlight = (fstart + i == pc);
			if (isHighlight)
			{
				/*
				if (*ppExpr == nullptr)
				{
					*ppExpr = (const ISExpression*)symbol.SourceExpression;
				}
				*/
			}

			char assemblyLine[256];

			if (isHighlight)
			{
				debugger.AddDisassembly(DISASSEMBLY_TEXT_TYPE::HILIGHT, "*", true);
				SafeFormat(assemblyLine, sizeof(assemblyLine), "%p", fstart + i);
				debugger.AddDisassembly(DISASSEMBLY_TEXT_TYPE::HILIGHT, assemblyLine);
				SafeFormat(assemblyLine, sizeof(assemblyLine), " %s %s ", rep.OpcodeText, rep.ArgText);
				debugger.AddDisassembly(DISASSEMBLY_TEXT_TYPE::HILIGHT, assemblyLine);

				if (symbol.Text[0] != 0)
				{
					SafeFormat(assemblyLine, sizeof(assemblyLine), "// %s", symbol.Text);
					debugger.AddDisassembly(DISASSEMBLY_TEXT_TYPE::COMMENT, assemblyLine);
				}

				debugger.AddDisassembly(DISASSEMBLY_TEXT_TYPE::MAIN, "\n");
			}
			else if ((i < hilightIndex && (hilightIndex - i) < 1024) || (i > hilightIndex && (i - hilightIndex) < 1024))
			{
				SafeFormat(assemblyLine, sizeof(assemblyLine), " %p", fstart + i);
				debugger.AddDisassembly(DISASSEMBLY_TEXT_TYPE::MAIN, assemblyLine);
				SafeFormat(assemblyLine, sizeof(assemblyLine), " %s %s ", rep.OpcodeText, rep.ArgText);
				debugger.AddDisassembly(DISASSEMBLY_TEXT_TYPE::MAIN, assemblyLine);

				if (symbol.Text[0] != 0)
				{
					SafeFormat(assemblyLine, sizeof(assemblyLine), "// %s", symbol.Text);
					debugger.AddDisassembly(DISASSEMBLY_TEXT_TYPE::COMMENT, assemblyLine);
				}

				debugger.AddDisassembly(DISASSEMBLY_TEXT_TYPE::MAIN, "\n");
			}

			if (rep.ByteCount == 0)
			{
				debugger.AddDisassembly(DISASSEMBLY_TEXT_TYPE::MAIN, "Bad disassembly");
				break;
			}

			i += rep.ByteCount;
			lineCount++;
		}

		debugger.AddDisassembly(DISASSEMBLY_TEXT_TYPE::MAIN, nullptr);

		return f;
	}

	struct MemberEnumeratorPopulator : MemberEnumeratorCallback
	{
		TREE_NODE_ID parentId;
		IUITree* tree;
		int depth;
		const uint8* instance;
		const IStructure* parentStruct = nullptr;
		const IStructure* concreteStruct = nullptr;
		ObjectStub* parentHeader = nullptr;
		int index = 0;
		int firstUnkIndex = 1;

		void OnMember(IPublicScriptSystem& ss, cstr childName, const Rococo::Compiler::IMember& member, const uint8* sfItem, int offset, int recurseDepth) override
		{
			index++;

			if (parentStruct)
			{
				if (index == 1)
				{
					auto* header = (ObjectStub*)sfItem;
					auto* t = header->Desc;

					concreteStruct = t->TypeInfo;
					parentHeader = header;

					char classDesc[256];
					if (header->refCount == ObjectStub::NO_REF_COUNT)
					{
						SafeFormat(classDesc, "[%+d] %s DestructorId: %lld. Refcount: n/a", (sfItem - instance), concreteStruct->Name(), header->Desc->DestructorId);
					}
					else
					{
						SafeFormat(classDesc, "[%+d] %s DestructorId: %lld. Refcount: %lld", (sfItem - instance), concreteStruct->Name(), header->Desc->DestructorId, header->refCount);
					}


					auto node = tree->AddChild(parentId, classDesc, CheckState_NoCheckBox);

					firstUnkIndex = 3 + concreteStruct->InterfaceCount();

					for (int i = 0; i < concreteStruct->InterfaceCount(); ++i)
					{
						auto& interface = concreteStruct->GetInterface(i);
						auto* vTable = concreteStruct->GetVirtualTable(i + 1);
						char interfaceDesc[256];
						SafeFormat(interfaceDesc, "Implements %s. vTable %p", interface.Name(), vTable);

						auto inode = tree->AddChild(node, interfaceDesc, CheckState_NoCheckBox);

						auto* instanceVTable = header->pVTables[i];
						if (instanceVTable != (VirtualTable*)vTable)
						{
							char vTableDesc[256];
							SafeFormat(vTableDesc, "vTable mismatch. Expecting %p, but found %p", vTable, instanceVTable);
							auto vnode = tree->AddChild(inode, vTableDesc, CheckState_NoCheckBox);
						}

						if (header->Desc->TypeInfo == &interface.NullObjectType())
						{
							// The object is a null object so there are no other valid interface pointers
							break;
						}
					}
					return;
				}
				else if (index < firstUnkIndex)
				{
					return;
				}
			}

			char value[256];

			auto* name = member.UnderlyingType()->Name();
			if (Eq(name, "_Null_Sys_Type_IStringBuilder") || (Eq(name, "_Null_Sys_Type_IString")))
			{
				TRY_PROTECTED
				{
					// In the event that we have an IString, or IString derived interface, such as IStringBuilder
					// Then we can use the pointer member of string constant to determine the text buffer.
					// This is guaranteed to work for all implementations of IString, including a null IString;
					InterfacePointer p = InterfacePointer(sfItem);
					auto* s = (CStringConstant*)(sfItem + (*p)->OffsetToInstance);
					SafeFormat(value, "%s", s->pointer);
				}
					CATCH_PROTECTED
				{
					SafeFormat(value, "IString: <Bad pointer>");
				}
			}
			else if (Eq(name, "_Null_Sys_Reflection_IExpression"))
			{
				TRY_PROTECTED
				{
					// In the event that we have an Expression
					// Then we can use the pointer member of the object to determine the s-expression.
					InterfacePointer p = InterfacePointer(sfItem);
					auto* object = InterfaceToInstance(InterfacePointer(sfItem));
					auto* objWithHandle = (ObjectStubWithHandle*)object;
					const ISExpression* s = (const ISExpression*)objWithHandle->handle;
					if (object->Desc->TypeInfo == ss.GetExpressionType() && s)
					{
						if (s->Parent() == nullptr)
						{
							SafeFormat(value, "Expression: <root>");
						}
						else
						{
							switch (s->Type())
							{
							case EXPRESSION_TYPE_ATOMIC:
							case EXPRESSION_TYPE_STRING_LITERAL:
								{
									size_t len = strlen(s->c_str());
									if (len > 64)
									{
										SafeFormat(value, "From line %d pos %d : %.64s...", s->c_str(), s->Start().y, s->Start().x);
									}
									else
									{
										SafeFormat(value, "From line %d pos %d:  %s", s->c_str(), s->Start().y, s->Start().x);
									}

									break;
								}
							case EXPRESSION_TYPE_COMPOUND:
								SafeFormat(value, "<compound %d elements>. From line %d pos %d to line %d pos %d", s->NumberOfElements(), s->Start().y, s->Start().x, s->End().y, s->End().x);
								break;
							case EXPRESSION_TYPE_NULL:
								SafeFormat(value, "<null expression>. From line %d pos %d to line %d pos %d", s->NumberOfElements(), s->Start().y, s->Start().x, s->End().y, s->End().x);
								break;
							default:
								SafeFormat(value, "Expression: <Bad pointer>");
								break;
							}
						}
					}
				}
					CATCH_PROTECTED
				{
					SafeFormat(value, "Expression: <Bad pointer>");
				}
			}
			else
			{
				FormatValue(ss, value, sizeof value, member.UnderlyingType()->VarType(), sfItem);
			}

			char memberDesc[256];

			if (member.UnderlyingType()->InterfaceCount() != 0)
			{
				SafeFormat(memberDesc, "[%+d] ->  %p %s: %s", offset, sfItem, member.Name(), value);
			}
			else
			{
				SafeFormat(memberDesc, "[%+d] %p %s: %s", (sfItem - instance), sfItem, member.Name(), value);
			}

			auto node = tree->AddChild(parentId, memberDesc, CheckState_NoCheckBox);
			tree->SetId(node, (int64)(depth + 1));

			if (member.UnderlyingType()->VarType() == SexyVarType_Derivative)
			{
				MemberEnumeratorPopulator subMember;

				if (member.UnderlyingType()->InterfaceCount() != 0)
				{
					subMember.parentStruct = member.UnderlyingType();
				}

				auto* subInstance = sfItem;
				// auto* subInstance = sfItem && member.UnderlyingType() ? (IsNullType(*member.UnderlyingType()) ? *(const uint8**)sfItem : sfItem) : nullptr;
				if (member.UnderlyingType() && IsNullType(*member.UnderlyingType()))
				{
					subInstance = sfItem;
				}
				subMember.instance = subInstance;
				subMember.parentId = node;
				subMember.tree = tree;
				subMember.depth = depth;

				GetMembers(ss, *member.UnderlyingType(), member.Name(), subInstance, 0, subMember, recurseDepth);
			}
			else if (member.UnderlyingType()->VarType() == SexyVarType_Array)
			{
				MemberEnumeratorPopulator subMember;

				if (member.UnderlyingType()->InterfaceCount() != 0)
				{
					subMember.parentStruct = member.UnderlyingType();
				}

				auto* subInstance = *(const uint8**)sfItem;

				char arrayDesc[256];

				if (subInstance == nullptr)
				{
					tree->AddChild(parentId, "null reference", CheckState_NoCheckBox);
					return;
				}

				ArrayImage& a = *(ArrayImage*)subInstance;

				SafeFormat(arrayDesc, "%d of %d elements of type %s and %d bytes each (total %d KB)", a.NumberOfElements, a.ElementCapacity, GetFriendlyName(*a.ElementType), a.ElementLength, (a.ElementLength * a.NumberOfElements) / 1024);

				tree->AddChild(node, arrayDesc, CheckState_NoCheckBox);

				SafeFormat(arrayDesc, "RefCount: %d. Modification %s", a.RefCount, a.LockNumber > 0 ? "prohibited" : "allowed");

				tree->AddChild(node, arrayDesc, CheckState_NoCheckBox);

				if (a.Start == nullptr)
				{
					tree->AddChild(node, "item pointer is null", CheckState_NoCheckBox);
				}
				else
				{
					const uint8* pInstance = (const uint8*)a.Start;

					char itemDesc[256];
					SafeFormat(itemDesc, "C-array start 0x%llX", pInstance);
					auto indexNode = tree->AddChild(node, itemDesc, CheckState_NoCheckBox);
					for (int i = 0; i < a.NumberOfElements; ++i)
					{
						enum { MAX_ITEMS_DEBUGGED_PER_ARRAY = 20 };
						if (i > MAX_ITEMS_DEBUGGED_PER_ARRAY)
						{
							tree->AddChild(indexNode, "...", CheckState_NoCheckBox);
							break;
						}

						char item[256];
						SafeFormat(item, "#%d", i);

						char itemEx[256];

						if (Eq(a.ElementType->Name(), "_Null_Sys_Type_IString"))
						{
							InterfacePointer pInterface = *(InterfacePointer*)pInstance;
							auto* stub = (CStringConstant*)InterfaceToInstance(pInterface);
							SafeFormat(itemEx, "0x%llX - #%d: %s", pInstance, i, stub->pointer);
							auto childNode = tree->AddChild(indexNode, itemEx, CheckState_NoCheckBox);
						}
						else
						{

							SafeFormat(itemEx, "0x%llX - #%d", pInstance, i);
							auto childNode = tree->AddChild(indexNode, itemEx, CheckState_NoCheckBox);

							if (a.ElementType->InterfaceCount() != 0)
							{
								subMember.parentStruct = a.ElementType;
							}

							auto* subInstanceInner = pInstance;

							if (IsNullType(*a.ElementType))
							{
								InterfacePointer pInterface = *(InterfacePointer*)pInstance;
								subInstanceInner = (const uint8*)pInterface;
							}
							subMember.instance = subInstanceInner;
							subMember.parentId = childNode;
							subMember.tree = tree;
							subMember.depth = depth;

							GetMembers(ss, *a.ElementType, item, subInstanceInner, 0, subMember, recurseDepth);
						}
						pInstance += a.ElementLength;
					}
				}
			}
		}

		void OnListMember(IPublicScriptSystem& ss, cstr childName, const Rococo::Compiler::IMember& member, const ListImage* l, const uint8* sfItem, int offset, int recurseDepth) override
		{
			index++;

			char name[256];

			if (!l)
			{
				SafeFormat(name, "[%+d] ->  %p list<%s> %s: null", offset, sfItem, GetFriendlyName(*member.UnderlyingGenericArg1Type()), childName);
			}
			else
			{
				SafeFormat(name, "[%+d] ->  %p list<%s> %s", offset, sfItem, GetFriendlyName(*member.UnderlyingGenericArg1Type()), childName);
			}

			auto node = tree->AddChild(parentId, name, CheckState_NoCheckBox);

			char ptr[256];
			SafeFormat(ptr, "%p list pointer", l);
			tree->AddChild(node, ptr, CheckState_NoCheckBox);

			if (!l)
			{
				return;
			}

			char metrics[256];
			SafeFormat(metrics, "%d elements", l->NumberOfElements);
			tree->AddChild(node, metrics, CheckState_NoCheckBox);

			char refCount[256];
			SafeFormat(refCount, "%lld references", l->refCount);
			tree->AddChild(node, refCount, CheckState_NoCheckBox);
		}

		void OnMapMember(IPublicScriptSystem& ss, cstr childName, const Rococo::Compiler::IMember& member, const MapImage* m, const uint8* sfItem, int offset, int recurseDepth) override
		{
			index++;

			char name[256];

			if (!m)
			{
				SafeFormat(name, "map<%s to %s> %s: null", GetFriendlyName(*member.UnderlyingGenericArg1Type()), GetFriendlyName(*member.UnderlyingGenericArg2Type()), childName);
			}
			else
			{
				SafeFormat(name, "map<%s to %s> %s", GetFriendlyName(*member.UnderlyingGenericArg1Type()), GetFriendlyName(*member.UnderlyingGenericArg2Type()), childName);
			}

			auto node = tree->AddChild(parentId, name, CheckState_NoCheckBox);

			if (!m)
			{
				return;
			}

			char metrics[256];
			SafeFormat(metrics, "%d entries", m->NumberOfElements);
			tree->AddChild(node, metrics, CheckState_NoCheckBox);

			char refCount[256];
			SafeFormat(refCount, "%d references", m->refCount);
			tree->AddChild(node, refCount, CheckState_NoCheckBox);

			char mapInfo[256];
			SafeFormat(mapInfo, "Address %p", m);
			tree->AddChild(node, mapInfo, CheckState_NoCheckBox);

			int index = -1;
			for (auto* p = m->Head; p != nullptr; p = p->Next)
			{
				index++;

				auto* keyType = m->KeyType;
				if (keyType && keyType->InterfaceCount() == 1 && IsIString(keyType->GetInterface(0)))
				{
					InlineString* s = *(InlineString**)Rococo::Script::GetKeyPointer(p);
					if (s != nullptr)
					{
						auto* value = Rococo::Script::GetValuePointer(p);
						SafeFormat(mapInfo, "[%d] '%s' -> %p", index, s->buffer ? s->buffer : "<null>", value);
						tree->AddChild(node, mapInfo, CheckState_NoCheckBox);
					}
				}
			}
		}

		void OnArrayMember(IPublicScriptSystem& ss, cstr childName, const Rococo::Compiler::IMember& member, const struct ArrayImage* array, const uint8* sfItem, int offset, int recurseDepth) override
		{
			index++;

			char name[256];

			if (!array)
			{
				SafeFormat(name, "array<%s> %s: null", GetFriendlyName(*member.UnderlyingGenericArg1Type()), childName);
			}
			else
			{
				SafeFormat(name, "array<%s> %s", GetFriendlyName(*member.UnderlyingGenericArg1Type()), childName);
			}

			auto node = tree->AddChild(parentId, name, CheckState_NoCheckBox);

			if (!array)
			{
				return;
			}

			char metrics[256];
			SafeFormat(metrics, "%d of %d elements", array->NumberOfElements, array->ElementCapacity);
			tree->AddChild(node, metrics, CheckState_NoCheckBox);

			char info[256];
			SafeFormat(info, "Private: Refcount %d. Enum: %s ", array->RefCount, array->LockNumber > 0 ? "locked" : "unlocked");
			tree->AddChild(node, info, CheckState_NoCheckBox);

			if (array->NumberOfElements > 0)
			{
				auto elements = tree->AddChild(node, "#-Elements-#", CheckState_NoCheckBox);
				for (int i = 0; i < array->NumberOfElements; ++i)
				{
					char sindex[16];
					SafeFormat(sindex, "[%d]", i);
					auto element = tree->AddChild(elements, sindex, CheckState_NoCheckBox);

					auto* subInstance = ((const uint8*)array->Start) + array->ElementLength * i;

					if (array->ElementType->InterfaceCount() > 0)
					{
						const uint8* puint8Interface = *(const uint8**)subInstance;
						subInstance = puint8Interface;
						auto* object = InterfaceToInstance((InterfacePointer)subInstance);

						char concreteInfo[256];

						if (object->refCount == ObjectStub::NO_REF_COUNT)
						{
							SafeFormat(concreteInfo, "%s <no-refcount> @ 0x%.16llX", GetFriendlyName(*object->Desc->TypeInfo), object);
						}
						else
						{
							SafeFormat(concreteInfo, "%s refcount: %llu @ 0x%.16llX", GetFriendlyName(*object->Desc->TypeInfo), object->refCount, object);
						}
						tree->AddChild(element, concreteInfo, CheckState_NoCheckBox);
					}
					else
					{
						char concreteInfo[256];
						SafeFormat(concreteInfo, "%s", GetFriendlyName(*array->ElementType));
						tree->AddChild(element, concreteInfo, CheckState_NoCheckBox);
					}

					if (i > 20) break;
				}
			}
		}
	};

	struct VariableEnumeratorPopulator : public IVariableEnumeratorCallback
	{
		virtual void OnVariable(size_t index, const VariableDesc& v, const MemberDef& def)
		{
			if (AreEqual(v.Location, "CPU"))
			{
				return;
			}

			if (IsIString(*def.ResolvedType))
			{
			}

			char desc[256];
			if (v.Value[0] != 0)
			{
				SafeFormat(desc, sizeof(desc), "[%d] %p: %s %s = %s", v.Address, v.Address + SF, v.Type, v.Name, v.Value);
			}
			else
			{
				SafeFormat(desc, sizeof(desc), "[%d] %p: %s %s", v.Address, v.Address + SF, v.Type, v.Name);
			}
			auto node = tree->AddRootItem(desc, CheckState_NoCheckBox);
			tree->SetId(node, int64(depth + 1));

			// At this level of enumeration v.instance refers to a stack address
			TRY_PROTECTED
			{
				MemberEnumeratorPopulator addMember;
				addMember.parentId = node;
				addMember.tree = tree;
				addMember.depth = depth + 1;
				addMember.instance = v.instance;

				InterfacePointer pInterf = (InterfacePointer)(v.instance);
				if (v.s) GetMembers(*ss, *v.s, v.parentName, v.instance, 0, addMember, 1);
			}
				CATCH_PROTECTED
			{

			}
		}

		int32 depth;
		TREE_NODE_ID sfNode;
		IUITree* tree;
		const uint8* SF;
		Script::IPublicScriptSystem* ss;
	};


	void PopulateMemberTree(Visitors::IUITree& tree, IPublicScriptSystem& ss, int depth)
	{
		auto& vm = ss.PublicProgramObject().VirtualMachine();

		const Rococo::uint8* sf = nullptr;
		const Rococo::uint8* pc = nullptr;
		const IFunction* f = nullptr;

		size_t fnOffset;
		size_t pcOffset;
		if (!GetCallDescription(sf, pc, f, fnOffset, ss, depth, pcOffset) || !f)
		{
			return;
		}

		VariableEnumeratorPopulator addToTree;

		addToTree.tree = &tree;
		addToTree.SF = sf;
		addToTree.depth = depth;
		addToTree.ss = &ss;

		TRY_PROTECTED
		{
			Script::ForeachVariable(ss, addToTree, depth);
		}
			CATCH_PROTECTED
		{

		}
	}

	void PopulateMemberTree(IPublicScriptSystem& ss, IDebuggerWindow& debugger, int depth)
	{
		struct : Visitors::ITreePopulator
		{
			IPublicScriptSystem* ss;
			int depth;

			void Populate(Visitors::IUITree& tree) override
			{
				Rococo::PopulateMemberTree(tree, *ss, depth);
			}
		} memberPopulator;

		memberPopulator.ss = &ss;
		memberPopulator.depth = depth;
		debugger.PopulateMemberView(memberPopulator);
	}

	void PopulateCallstack(IPublicScriptSystem& ss, IDebuggerWindow& debugger)
	{
		struct : Visitors::IListPopulator
		{
			IPublicScriptSystem* ss;

			void Populate(Visitors::IUIList& list) override
			{
				auto& vm = ss->PublicProgramObject().VirtualMachine();

				const Rococo::uint8* sf = nullptr;
				const Rococo::uint8* pc = nullptr;
				const IFunction* f = nullptr;

				size_t fnOffset;
				size_t pcOffset;

				int depth = 0;
				while (GetCallDescription(sf, pc, f, fnOffset, *ss, depth++, pcOffset) && f)
				{
					cstr values[] = { f->Name(), f->Module().Name(), nullptr };
					list.AddRow(values);
				}
			}
		} callstackPopulator;

		callstackPopulator.ss = &ss;
		debugger.PopulateCallStackView(callstackPopulator);
	}

	void PopulateVariableList(Visitors::IUIList& list, IPublicScriptSystem& ss, int depth)
	{
		auto& vm = ss.PublicProgramObject().VirtualMachine();

		const Rococo::uint8* sf = nullptr;
		const Rococo::uint8* pc = nullptr;
		const IFunction* f = nullptr;

		size_t fnOffset;
		size_t pcOffset;
		if (!GetCallDescription(sf, pc, f, fnOffset, ss, depth, pcOffset) || !f)
		{
			return;
		}

		struct ANON : public IVariableEnumeratorCallback
		{
			virtual void OnVariable(size_t index, const VariableDesc& v, const MemberDef& def)
			{
				char offset[32];
				SafeFormat(offset, sizeof(offset), "%d", v.Address);
				char descAddress[256];
				SafeFormat(descAddress, sizeof(descAddress), "0x%llX", v.Address + SF);

				cstr values[] = { offset, descAddress, v.Location, v.Type, v.Name, v.Value, nullptr };
				list->AddRow(values);
			}

			int32 depth;
			IUIList* list;
			const uint8* SF;
			Script::IPublicScriptSystem* ss;
		} addToList;

		addToList.ss = &ss;
		addToList.depth = depth;
		addToList.list = &list;
		addToList.SF = sf;

		TRY_PROTECTED
		{
			Script::ForeachVariable(ss, addToList, depth);
		}
			CATCH_PROTECTED
		{

		}
	}


	void PopulateVariables(IPublicScriptSystem& ss, IDebuggerWindow& debugger, int depth)
	{
		struct : Visitors::IListPopulator
		{
			IPublicScriptSystem* ss;
			int depth;

			void Populate(Visitors::IUIList& list) override
			{
				PopulateVariableList(list, *ss, depth);
			}
		} variablePopulator;

		variablePopulator.ss = &ss;
		variablePopulator.depth = depth;

		debugger.PopulateVariableView(variablePopulator);
	}

	void PopulateSourceView(Script::IPublicScriptSystem& ss, IDebuggerWindow& debugger, int64 stackDepth)
	{
		const Rococo::uint8* sf = nullptr;
		const Rococo::uint8* pc = nullptr;
		const Rococo::Compiler::IFunction* f;

		size_t fnOffset;
		size_t pcOffset;
		if (Rococo::Script::GetCallDescription(sf, pc, f, fnOffset, ss, stackDepth, pcOffset) && f)
		{
			auto* tree = ss.GetSourceCode(f->Module());
			if (tree)
			{
				debugger.AddSourceCode(f->Module().Name(), tree->Source().SourceStart());
			}
		}
	}

	struct DebuggerPopulator : IDebuggerPopulator
	{
		Script::IPublicScriptSystem* ss;
		int stackDepth;
		bool refreshAll;

		void Populate(IDebuggerWindow& debugger) override
		{
			if (refreshAll)
			{
				struct : Visitors::IListPopulator
				{
					Script::IPublicScriptSystem* ss;
					IDebuggerWindow* debugger;

					void Populate(Visitors::IUIList& registerListView) override
					{
						PopulateRegisterWindow(*ss, registerListView);
					}
				} anon;

				anon.ss = ss;
				anon.debugger = &debugger;

				debugger.PopulateRegisterView(anon);
			}

			auto& vm = ss->PublicProgramObject().VirtualMachine();
			AutoFree<VM::IDisassembler> disassembler(vm.Core().CreateDisassembler());

			for (int depth = 0; depth < 10; depth++)
			{
				const ISExpression* s = nullptr;
				const uint8* SF;
				auto* f = DisassembleCallStackAndAppendToView(*disassembler, debugger, *ss, vm.Cpu(), depth, &s, &SF, stackDepth);
				if (depth == stackDepth && f != nullptr)
				{
					if (s != nullptr)
					{
						if (s->GetOriginal() != nullptr)
						{
							s = s->GetOriginal();
						}

						auto& origin = s->Tree().Source().Origin();
						auto p0 = s->Start() - Vec2i{ 1,0 };
						auto p1 = s->End() - Vec2i{ 1,0 };
						debugger.SetCodeHilight(s->Tree().Source().Name(), p0, p1, "!");
					}
				}

				if (SF == nullptr) break;
			}

			PopulateMemberTree(*ss, debugger, stackDepth);
			PopulateVariables(*ss, debugger, stackDepth);

			if (refreshAll)
			{
				PopulateCallstack(*ss, debugger);
			}

			PopulateSourceView(*ss, debugger, stackDepth);
		}

	};


	void UpdateDebugger(Script::IPublicScriptSystem& ss, IDebuggerWindow& debugger, int32 stackDepth, bool refreshAll)
	{
		DebuggerPopulator populator;
		populator.ss = &ss;
		populator.stackDepth = stackDepth;
		populator.refreshAll = refreshAll;
		populator.Populate(debugger);
	}

	struct StardardDebugControl : IDebugControl
	{
		Rococo::VM::IVirtualMachine* vm;
		Rococo::Script::IPublicScriptSystem* ss;
		IDebuggerWindow* debugger;

		void Update()
		{
			UpdateDebugger(*ss, *debugger, 0, true);
		}

		void RecurseNamespaces(const INamespace& ns, IUITree& tree, Visitors::TREE_NODE_ID rootId)
		{
			if (ns.ChildCount() > 0)
			{
				auto childSubspacesId = tree.AddChild(rootId, "[Subspaces]", CheckState_Clear);

				for (int i = 0; i < ns.ChildCount(); ++i)
				{
					auto& child = ns.GetChild(i);
					auto childId = tree.AddChild(childSubspacesId, child.Name()->Buffer, CheckState_Clear);
					RecurseNamespaces(child, tree, childId);
				}
			}

			struct : public ICallback<const Rococo::Compiler::IStructure, cstr>
			{
				TREE_NODE_ID childStructuresId;
				TREE_NODE_ID rootId;
				IUITree* tree;
				const Rococo::Compiler::INamespace* ns;
				CALLBACK_CONTROL operator()(const Rococo::Compiler::IStructure& s, cstr alias) override
				{
					if (!childStructuresId) childStructuresId = tree->AddChild(rootId, "[Structures]", CheckState_Clear);

					char desc[256];
					SafeFormat(desc, sizeof(desc), "%s.%s", ns->FullName()->Buffer, alias);
					auto typeId = tree->AddChild(childStructuresId, desc, CheckState_Clear);

					SafeFormat(desc, sizeof(desc), "%s - %d bytes. Defined in %s", Parse::VarTypeName(s.VarType()), s.SizeOfStruct(), s.Module().Name());
					auto typeDescId = tree->AddChild(typeId, desc, CheckState_Clear);

					if (s.VarType() == SexyVarType_Derivative)
					{
						for (int32 i = 0; i < s.MemberCount(); ++i)
						{
							cstr fieldType = s.GetMember(i).UnderlyingType() ? s.GetMember(i).UnderlyingType()->Name() : "Unknown Type";
							SafeFormat(desc, sizeof(desc), "%s %s", fieldType, s.GetMember(i).Name());
							tree->AddChild(typeId, desc, CheckState_Clear);
						}
					}
					return CALLBACK_CONTROL_CONTINUE;
				}
			} enumStructures;

			enumStructures.tree = &tree;
			enumStructures.childStructuresId.value = 0;
			enumStructures.rootId = rootId;
			enumStructures.ns = &ns;

			ns.EnumerateStrutures(enumStructures);

			struct : public ICallback<const Rococo::Compiler::IFunction, cstr>
			{
				TREE_NODE_ID childFunctionsId;
				TREE_NODE_ID rootId;
				IUITree* tree;
				const Rococo::Compiler::INamespace* ns;
				virtual CALLBACK_CONTROL operator()(const Rococo::Compiler::IFunction& f, cstr alias)
				{
					if (!childFunctionsId) childFunctionsId = tree->AddChild(rootId, "[Functions]", CheckState_Clear);

					TREE_NODE_ID typeId;
					char desc[256];

					SafeFormat(desc, sizeof(desc), "%s.%s", ns->FullName()->Buffer, alias);
					typeId = tree->AddChild(childFunctionsId, desc, CheckState_Clear);

					SafeFormat(desc, sizeof(desc), "Defined in %s", f.Module().Name());
					tree->AddChild(typeId, desc, CheckState_Clear);

					AddArguments(f, *tree, typeId);

					return CALLBACK_CONTROL_CONTINUE;
				}
			} enumFunctions;

			enumFunctions.tree = &tree;
			enumFunctions.childFunctionsId.value = 0;
			enumFunctions.rootId = rootId;
			enumFunctions.ns = &ns;

			ns.EnumerateFunctions(enumFunctions);

			if (ns.InterfaceCount() > 0)
			{
				char desc[256];

				auto interfaceId = tree.AddChild(rootId, "[Interfaces]", CheckState_Clear);

				for (int i = 0; i < ns.InterfaceCount(); ++i)
				{
					auto& inter = ns.GetInterface(i);
					auto* base = inter.Base();

					if (base)
					{
						SafeFormat(desc, sizeof(desc), "%s.%s extending %s", ns.FullName()->Buffer, inter.Name(), base->Name());
					}
					else
					{
						SafeFormat(desc, sizeof(desc), "%s.%s", ns.FullName()->Buffer, inter.Name());
					}

					auto interId = tree.AddChild(interfaceId, desc, CheckState_Clear);

					for (int j = 0; j < inter.MethodCount(); ++j)
					{
						auto& method = inter.GetMethod(j);
						if (&method != nullptr)
						{
							SafeFormat(desc, sizeof(desc), "method %s", method.Name());
							auto methodId = tree.AddChild(interId, desc, CheckState_Clear);
							AddArguments(method, tree, methodId);
						}
					}
				}
			}

			struct : public ICallback<const Rococo::Compiler::IFactory, cstr>
			{
				TREE_NODE_ID childFactoryId;
				TREE_NODE_ID rootId;
				IUITree* tree;
				const Rococo::Compiler::INamespace* ns;
				CALLBACK_CONTROL operator()(const Rococo::Compiler::IFactory& f, cstr alias) override
				{
					if (!childFactoryId) childFactoryId = tree->AddChild(rootId, "[Factories]", CheckState_Clear);

					TREE_NODE_ID typeId;
					char desc[256];

					SafeFormat(desc, sizeof(desc), "%s.%s - creates objects of type %s", ns->FullName()->Buffer, alias, f.InterfaceType()->Buffer);
					typeId = tree->AddChild(childFactoryId, desc, CheckState_Clear);

					SafeFormat(desc, sizeof(desc), "Defined in %s", f.Constructor().Module().Name());
					tree->AddChild(typeId, desc, CheckState_Clear);

					AddArguments(f.Constructor(), *tree, typeId);

					return CALLBACK_CONTROL_CONTINUE;
				}
			} enumFactories;

			enumFactories.tree = &tree;
			enumFactories.childFactoryId.value = 0;
			enumFactories.rootId = rootId;
			enumFactories.ns = &ns;

			ns.EnumerateFactories(enumFactories);
		}

		void RefreshAtDepth(int stackDepth) override
		{
			UpdateDebugger(*ss, *debugger, stackDepth, false);
		}

		void PopulateAPITree(Visitors::IUITree& tree) override
		{
			auto& root = ss->PublicProgramObject().GetRootNamespace();
			auto nsid = tree.AddRootItem("[Namespaces]", CheckState_Clear);
			RecurseNamespaces(root, tree, nsid);
		}

		void Continue() override
		{
			vm->ContinueExecution(VM::ExecutionFlags(true, true, false));
			Update();
		}

		void StepOut() override
		{
			vm->StepOut();
			Update();
		}

		void StepOver() override
		{
			vm->StepOver();
			Update();
		}

		void StepNext() override
		{
			vm->StepInto();
			Update();
		}

		void StepNextSymbol() override
		{
			//ISymbols& symbols = ss->PublicProgramObject().ProgramMemory().
			//vm->StepNextSymbol()
		}
	};

	SCRIPTEXPORT_API void DebuggerLoop(Rococo::Script::IPublicScriptSystem& ss, IDebuggerWindow& debugger)
	{
		StardardDebugControl dc;

		dc.vm = &ss.PublicProgramObject().VirtualMachine();
		dc.ss = &ss;
		dc.debugger = &debugger;

		DebuggerPopulator populator;
		populator.ss = &ss;
		populator.stackDepth = 0;
		populator.refreshAll = true;

		debugger.Run(populator, dc);
	}

	void LogStack(IException& ex, ILog& logger)
	{
		char buf[16384];
		Rococo::OS::BuildExceptionString(buf, sizeof buf, ex, true);

		logger.Write(buf);

		logger.Write("\n");

		/*

		auto* sf = ex.StackFrames();
		if (!sf) return;

		struct Formatter : public IStackFrameFormatter
		{
			ILog& logger;
			Formatter(ILog& argLogger) : logger(argLogger)
			{

			}

			void Format(const StackFrame& frame) override
			{
				char buf[1024];
				SafeFormat(buf, "%64s - %s line %d", frame.functionName, frame.sourceFile, frame.lineNumber);
				logger.Write(buf);
			}
		} sfFormatter(logger);

		sf->FormatEachStackFrame(sfFormatter);
		*/
	}


	EXECUTERESULT ExecuteAndCatchIException(IVirtualMachine& vm, IPublicScriptSystem& ss, IDebuggerWindow& debugger, bool trace)
	{
		try
		{
			if (trace && Rococo::OS::IsDebugging())
			{
				struct ANON : VM::ITraceOutput
				{
					Rococo::Script::IPublicScriptSystem& ss;
					IVirtualMachine& vm;
					AutoFree<VM::IDisassembler> disassembler;

					ANON(Rococo::Script::IPublicScriptSystem& _ss, IVirtualMachine& _vm) : vm(_vm), ss(_ss)
					{
						disassembler = vm.Core().CreateDisassembler();
					}

					void Report() override
					{
						VM::IDisassembler::Rep rep;
						disassembler->Disassemble(vm.Cpu().PC(), rep);

						auto id = ss.PublicProgramObject().ProgramMemory().GetFunctionContaingAddress(vm.Cpu().PC() - vm.Cpu().ProgramStart);
						auto* f = GetFunctionFromBytecode(ss.PublicProgramObject(), id);

						char line[256];

						if (f)
						{
							SafeFormat(line, 256, "[ %s ] %s: %s\n", f->Name(), rep.OpcodeText, rep.ArgText);
						}
						else
						{
							SafeFormat(line, 256, "[ ] %s: %s\n", rep.OpcodeText, rep.ArgText);
						}

						if (OS::IsDebugging())
						{
							PrintD("%s", line);
						}
					}
				} tracer(ss, vm);
				EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, false), &tracer);
				return result;
			}
			else
			{
				EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, false));
				return result;
			}
		}
		catch (ParseException& pex)
		{
			auto* s = pex.Source();
			cstr sourceFile = "unknown source";
			if (s != nullptr)
			{
				sourceFile = s->Tree().Source().Name();
			}
			debugger.SetCodeHilight(sourceFile, pex.Start(), pex.End(), pex.Message());
			UpdateDebugger(ss, debugger, 0, true);
			//ss.PublicProgramObject().Log().Write(pex.Message());
			LogStack(pex, ss.PublicProgramObject().Log());
			if (s) Throw(*s, pex.Message());
			else Throw(pex.ErrorCode(), "%s", pex.Message());
			return EXECUTERESULT_THROWN;
		}
		catch (IException& ex)
		{
			UpdateDebugger(ss, debugger, 0, true);
			//ss.PublicProgramObject().Log().Write(ex.Message());
			LogStack(ex, ss.PublicProgramObject().Log());
			Throw(ex.ErrorCode(), "%s", ex.Message());
			return EXECUTERESULT_THROWN;
		}
	}

	void Execute(VM::IVirtualMachine& vm, Rococo::Script::IPublicScriptSystem& ss, IDebuggerWindow& debugger, bool trace)
	{
		EXECUTERESULT result = EXECUTERESULT_YIELDED;

		bool captureStructedException = false;

		if (captureStructedException && !OS::IsDebugging())
		{
			TRY_PROTECTED
			{
				result = ExecuteAndCatchIException(vm, ss, debugger, trace);
			}
				CATCH_PROTECTED
			{
				result = EXECUTERESULT_SEH;
			}
		}
		else
		{
			result = ExecuteAndCatchIException(vm, ss, debugger, trace);
		}

		switch (result)
		{
		case EXECUTERESULT_BREAKPOINT:
			UpdateDebugger(ss, debugger, 0, true);
			Throw(0, "Script hit breakpoint");
			break;
		case EXECUTERESULT_ILLEGAL:
			UpdateDebugger(ss, debugger, 0, true);
			Throw(0, "Script did something bad.");
			break;
		case EXECUTERESULT_NO_ENTRY_POINT:
			Throw(0, "No entry point");
			break;
		case EXECUTERESULT_NO_PROGRAM:
			Throw(0, "No program");
			break;
		case EXECUTERESULT_RETURNED:
			Throw(0, "Unexpected EXECUTERESULT_RETURNED");
			break;
		case EXECUTERESULT_SEH:
			UpdateDebugger(ss, debugger, 0, true);
			Throw(0, "The script triggered a structured exception handler");
			break;
		case EXECUTERESULT_THROWN:
			UpdateDebugger(ss, debugger, 0, true);
			Throw(0, "The script triggered a virtual machine exception");
			break;
		case EXECUTERESULT_YIELDED:
			UpdateDebugger(ss, debugger, 0, true);
			Throw(0, "The script yielded");
			break;
		case EXECUTERESULT_TERMINATED:
			break;
		default:
			Rococo::Throw(0, "Unexpected EXECUTERESULT %d", result);
			break;
		}
	}

	void ExecuteUntilYield(VM::IVirtualMachine& vm, Rococo::Script::IPublicScriptSystem& ss, IDebuggerWindow& debugger, bool trace)
	{
		EXECUTERESULT result = EXECUTERESULT_YIELDED;

		bool captureStructedException = false;

		if (captureStructedException && !OS::IsDebugging())
		{
			TRY_PROTECTED
			{
				result = ExecuteAndCatchIException(vm, ss, debugger, trace);
			}
				CATCH_PROTECTED
			{
				result = EXECUTERESULT_SEH;
			}
		}
		else
		{
			result = ExecuteAndCatchIException(vm, ss, debugger, trace);
		}

		switch (result)
		{
		case EXECUTERESULT_BREAKPOINT:
			UpdateDebugger(ss, debugger, 0, true);
			Throw(0, "Script hit breakpoint");
			break;
		case EXECUTERESULT_ILLEGAL:
			UpdateDebugger(ss, debugger, 0, true);
			Throw(0, "Script did something bad.");
			break;
		case EXECUTERESULT_NO_ENTRY_POINT:
			Throw(0, "No entry point");
			break;
		case EXECUTERESULT_NO_PROGRAM:
			Throw(0, "No program");
			break;
		case EXECUTERESULT_RETURNED:
			Throw(0, "Unexpected EXECUTERESULT_RETURNED");
			break;
		case EXECUTERESULT_SEH:
			UpdateDebugger(ss, debugger, 0, true);
			Throw(0, "The script triggered a structured exception handler");
			break;
		case EXECUTERESULT_THROWN:
			UpdateDebugger(ss, debugger, 0, true);
			Throw(0, "The script triggered a virtual machine exception");
			break;
		case EXECUTERESULT_YIELDED:
			break;
		case EXECUTERESULT_TERMINATED:
			UpdateDebugger(ss, debugger, 0, true);
			Throw(0, "The script terminated while a yield was expected");
			break;
		default:
			Rococo::Throw(0, "Unexpected EXECUTERESULT %d", result);
			break;
		}
	}

	SCRIPTEXPORT_API void ExecuteFunction(ID_BYTECODE bytecodeId, IArgEnumerator& args, Script::IPublicScriptSystem& ss, IDebuggerWindow& debugger, bool trace)
	{
		ss.PublicProgramObject().SetProgramAndEntryPoint(bytecodeId);

		auto& vm = ss.PublicProgramObject().VirtualMachine();

		struct : IArgStack, IOutputStack
		{
			virtual void PushInt32(int32 value)
			{
				vm->Push(value);
			}
			virtual void PushInt64(int64 value)
			{
				vm->Push(value);
			}
			virtual void PushPointer(void* value)
			{
				vm->Push(value);
			}
			virtual int32 PopInt32()
			{
				return vm->PopInt32();
			}

			IVirtualMachine* vm;
		} argStack;

		argStack.vm = &vm;
		args.PushArgs(argStack);

		Execute(vm, ss, debugger, trace);

		args.PopOutputs(argStack);
	};

	SCRIPTEXPORT_API void ExecuteFunctionUntilYield(ID_BYTECODE bytecodeId, Script::IPublicScriptSystem& ss, IDebuggerWindow& debugger, bool trace)
	{
		auto& vm = ss.PublicProgramObject().VirtualMachine();
		ExecuteUntilYield(vm, ss, debugger, trace);
	};

	SCRIPTEXPORT_API void ExecuteFunction(cstr name, IArgEnumerator& args, Script::IPublicScriptSystem& ss, IDebuggerWindow& debugger, bool trace)
	{
		auto& module = ss.PublicProgramObject().GetModule(ss.PublicProgramObject().ModuleCount() - 1);
		auto f = module.FindFunction(name);
		if (f == nullptr)
		{
			Throw(0, "Cannot find function <%s> in <%s>", name, module.Name());
		}

		ss.PublicProgramObject().SetProgramAndEntryPoint(*f);

		auto& vm = ss.PublicProgramObject().VirtualMachine();

		struct : IArgStack, IOutputStack
		{
			virtual void PushInt32(int32 value)
			{
				vm->Push(value);
			}
			virtual void PushInt64(int64 value)
			{
				vm->Push(value);
			}
			virtual void PushPointer(void* value)
			{
				vm->Push(value);
			}
			virtual int32 PopInt32()
			{
				return vm->PopInt32();
			}

			IVirtualMachine* vm;
		} argStack;

		argStack.vm = &vm;
		args.PushArgs(argStack);

		Execute(vm, ss, debugger, trace);

		args.PopOutputs(argStack);
	};

	SCRIPTEXPORT_API int ExecuteSexyScript(ScriptPerformanceStats& stats, ISParserTree& mainModule, IDebuggerWindow& debugger, Script::IPublicScriptSystem& ss, ISourceCache& sources, IScriptEnumerator& implicitIncludes, int32 param, IScriptCompilationEventHandler& onCompile, bool trace, StringBuilder* declarationBuilder)
	{
		Time::ticks start = Time::TickCount();
		InitSexyScript(mainModule, debugger, ss, sources, implicitIncludes, onCompile, declarationBuilder);

		const INamespace* ns = ss.PublicProgramObject().GetRootNamespace().FindSubspace("EntryPoint");
		if (ns == nullptr)
		{
			Throw(0, "Cannot find (namespace EntryPoint) in %s", mainModule.Source().Name());
		}

		const IFunction* f = ns->FindFunction("Main");
		if (f == nullptr)
		{
			Throw(0, "Cannot find function EntryPoint.Main in %s", mainModule.Source().Name());
		}

		ss.PublicProgramObject().SetProgramAndEntryPoint(*f);

		auto& vm = ss.PublicProgramObject().VirtualMachine();

		vm.Core().SetLogger(&ss.PublicProgramObject().Log());

		vm.Push(param);

		stats.compileTime = Time::TickCount() - start;

		start = Time::TickCount();

		Execute(vm, ss, debugger, trace);

		stats.executeTime = Time::TickCount() - start;

		int exitCode = vm.PopInt32();

		vm.Core().SetLogger(nullptr);
		return exitCode;
	}
}