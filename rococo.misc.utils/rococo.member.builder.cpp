#include <rococo.api.h>
#include <rococo.csv.h>
#include <rococo.io.h>
#include <rococo.os.h>
#include <sexy.compiler.public.h>
#include <rococo.strings.h>
#include <sexy.vm.cpu.h>
#include <sexy.script.h>

#include <vector>

#include <rococo.hashtable.h>

namespace Rococo::Compiler
{
	bool IsNullType(const IStructure& type)
	{
		return StartsWith(type.Name(), "_Null");
	}
}

namespace
{
	using namespace Rococo;
	using namespace Rococo::IO;
	using namespace Rococo::Compiler;
	using namespace Rococo::Sexy;
	using namespace Rococo::Script;

	inline ObjectStub* InterfaceToInstance(InterfacePointer i)
	{
		auto* p = ((uint8*)i) + (*i)->OffsetToInstance;
		auto* obj = (ObjectStub*)p;
		return obj;
	}

	int GetIntefaceIndex(const IStructure& concreteType, const IInterface* interfaceType)
	{
		for (int j = 0; j < concreteType.InterfaceCount(); ++j)
		{
			auto& interfaceJ = concreteType.GetInterface(j);
			if (&interfaceJ == interfaceType)
			{
				return j;
			}
		}

		Throw(0, "%s does not support interface %s of %s", concreteType.Name(), interfaceType->Name(), interfaceType->NullObjectType().Module().Name());
	}

	struct SexyObjectBuilder: ISexyObjectBuilder, IMemberBuilder
	{
		const IStructure* type = nullptr;
		uint8* pObject = nullptr;
		ObjectStub* objectStub = nullptr;
		uint8* writePosition = nullptr;
		uint8* writeCursor = nullptr;
		const IStructure* objectType = nullptr;

		struct MemberAndOffset
		{
			const IMember* member;
			int offset;
		};

		class MemberRefManager
		{
			// The array of member indices that identifies the next expected member used by this API.
			// Index N + 1 gives the child index of the member with Index N
			// E.g memberIndexStack = { 0, 2, 7} => type.member[0].member[2].member[7]
			std::vector<int> memberIndexStack;

		public:
			MemberAndOffset GetMemberAndOffsetForStackPosition(const IStructure& parentType, int stackPosition)
			{
				int offset = 0;
				int nextMemberIndex = memberIndexStack[stackPosition];

				const IMember* childMember = nullptr;
				if (nextMemberIndex < parentType.MemberCount())
				{
					childMember = &parentType.GetMember(nextMemberIndex);
					if (childMember == nullptr)
					{
						return { nullptr, 0 };
					}

					offset += GetWriteOffset(parentType, nextMemberIndex);
				}

				return { childMember, offset };
			}

			MemberAndOffset GetMemberAndOffset(const IStructure& type)
			{
				int structureMemberIndex = GetRoot();

				const IMember* member = nullptr;

				if (structureMemberIndex < type.MemberCount())
				{
					member = &type.GetMember(structureMemberIndex);
				}

				if (!member)
				{
					return { nullptr, 0 };
				}

				int offset = GetWriteOffset(type, structureMemberIndex);

				const IStructure* parentType = member->UnderlyingType();

				for (int i = 1; i < memberIndexStack.size(); ++i)
				{
					MemberAndOffset childMemberAndOffset = GetMemberAndOffsetForStackPosition(*parentType, i);
					offset += childMemberAndOffset.offset;
					if (childMemberAndOffset.member == nullptr)
					{
						return { nullptr, 0 };
					}
					parentType = childMemberAndOffset.member->UnderlyingType();
					member = childMemberAndOffset.member;
				}

				return { member, offset };
			}

			MemberAndOffset FindMember(const IStructure& type, cstr name)
			{
				int offset = 0;

				if (memberIndexStack.size() == 1)
				{
					// We are editing the top level members
					int memberIndex = GetMemberIndex(name, type);
					if (memberIndex < 0)
					{
						return { nullptr, 0 };
					}
					else
					{
						memberIndexStack[0] = memberIndex;
						auto result = GetMemberAndOffsetForStackPosition(type, 0);
						return result;
					}
				}

				const IStructure* parentType = &type;

				for (int i = 1; i < memberIndexStack.size() - 1; ++i)
				{
					MemberAndOffset childMemberAndOffset = GetMemberAndOffsetForStackPosition(*parentType, i);
					if (childMemberAndOffset.member == nullptr)
					{
						return {nullptr, 0};
					}
					offset += childMemberAndOffset.offset;
					parentType = childMemberAndOffset.member->UnderlyingType();
				}

				MemberAndOffset memberAndOffset{ nullptr, 0 };

				int memberIndex = GetMemberIndex(name, *parentType);
				if (memberIndex < 0)
				{
					return { nullptr, 0 };
				}
				else
				{
					memberIndexStack.back() = memberIndex;
					memberAndOffset = GetMemberAndOffsetForStackPosition(type, (int)(memberIndexStack.size() - 1));
					offset += memberAndOffset.offset;
				}

				return memberAndOffset;
			}

			static int GetWriteOffset(const IStructure& s, int memberIndex)
			{
				int offset = 0;

				for (int i = 0; i < memberIndex; ++i)
				{
					auto& member = s.GetMember(i);
					offset += member.SizeOfMember();
				}

				return offset;
			}

			void InitToTypeItself()
			{
				memberIndexStack.clear();
			}

			void MoveToNextSibling()
			{
				auto& refIndex = memberIndexStack.back();
				refIndex++;
			}

			void RollbackToAncestor(size_t targetDepth)
			{
				while (targetDepth < memberIndexStack.size())
				{
					memberIndexStack.pop_back();
				}
			}

			void DescendToFirstChild()
			{
				memberIndexStack.push_back(0);
			}

			void InitToFirstChild()
			{
				InitToTypeItself();
				DescendToFirstChild();
			}

			int GetRoot() const
			{
				return memberIndexStack[0];
			}

			void MoveToParent()
			{
				memberIndexStack.pop_back();
				if (memberIndexStack.empty())
				{
					Throw(0, "%s called too many times. Algorithmic error", __FUNCTION__);
				}
			}
		} memberRefManager;

		IPublicScriptSystem* scriptSystem = nullptr;

		mutable const IStructure* typeIString = nullptr;
		mutable const IStructure* typeIStringBuilder = nullptr;

		struct RequiredInterfaceRef
		{
			const IInterface* pInterfaceType;
			InterfacePointer* ppInterface;
		};

		struct DeserializedObject
		{
			ObjectStub* stub = nullptr;
			std::vector<RequiredInterfaceRef> requiredInterfaces;
		};

		struct RequiredArray
		{
			ArrayImage** pImageWritePosition;
		};

		stringmap<DeserializedObject> objects;
		stringmap<ArrayImage*> arrays;

		struct ElementMemberDesc
		{
			const IMember* member;
			ptrdiff_t memberDataOffset;
		};

		// The target container to build 
		struct Container
		{
			std::vector<ElementMemberDesc> elements;
		} container;

		// The target array to build
		struct ArrayBuilder
		{
			ArrayImage* image = nullptr;
			uint8* elementBuffer = nullptr;
		} arrayBuilder;

		SexyObjectBuilder()
		{

		}

		void AddArrayRefMember(cstr name, cstr arrayRefName) override
		{
			const IMember* member = GetBestMatchingMember(name);
			if (member == nullptr)
			{
				// No array found in target, nothing to write
				return;
			}

			if (member->UnderlyingType()->VarType() != VARTYPE_Array)
			{
				Throw(0, "Cannot load %s. The target member %s is not an array", arrayRefName, name);
			}

			auto& elementType = *member->UnderlyingGenericArg1Type();
			
			auto i = arrays.find(arrayRefName);
			if (i == arrays.end())
			{
				auto* image = scriptSystem->CreateArrayImage(elementType);
				i = arrays.insert(arrayRefName, image).first;
			}
			else
			{
				if (i->second->ElementType != &elementType)
				{
					Throw(0, "Cannot load %s. The target member [%s] type %s is not the same as the array type %s", arrayRefName, name, i->second->ElementType->Name(), elementType.Name());
				}

				i->second->RefCount++;
			}
			
			WritePrimitive(i->second);
		}

		void AddContainerItem(int elementMemberIndex, int32 memberDepth, cstr memberName, VARTYPE validType)
		{
			memberRefManager.RollbackToAncestor(memberDepth + 1);

			const IMember* member = GetBestMatchingMember(memberName);

			if (member == nullptr)
			{
				// We have no match for the element, but this may mean the archive was written before the field was deleted and we want to load what we can.
				container.elements.push_back(ElementMemberDesc{ nullptr, 0 });
				return;
			}

			if (member->UnderlyingType()->VarType() != validType)
			{
				container.elements.push_back(ElementMemberDesc{ nullptr, 0 });
				return;
			}

			container.elements.push_back(ElementMemberDesc{ member, writeCursor - writePosition });

			memberRefManager.MoveToNextSibling();
		}

		void AddContainerItemF32(int elementMemberIndex, int32 memberDepth, cstr memberName) override
		{
			AddContainerItem(elementMemberIndex, memberDepth, memberName, VARTYPE_Float32);
		}

		void AddContainerItemF64(int elementMemberIndex, int32 memberDepth, cstr memberName) override
		{
			AddContainerItem(elementMemberIndex, memberDepth, memberName, VARTYPE_Float64);
		}

		void AddContainerItemI32(int elementMemberIndex, int32 memberDepth, cstr memberName) override
		{
			AddContainerItem(elementMemberIndex, memberDepth, memberName, VARTYPE_Int32);
		}

		void AddContainerItemI64(int elementMemberIndex, int32 memberDepth, cstr memberName) override
		{
			AddContainerItem(elementMemberIndex, memberDepth, memberName, VARTYPE_Int64);
		}

		void AddContainerItemBool(int elementMemberIndex, int32 memberDepth, cstr memberName) override
		{
			AddContainerItem(elementMemberIndex, memberDepth, memberName, VARTYPE_Bool);
		}

		void AddContainerItemDerivative(int32 memberDepth, cstr name, cstr type, cstr typeSource) override
		{
			memberRefManager.RollbackToAncestor(memberDepth + 1);

			const IMember* member = GetBestMatchingMember(name);

			if (member != nullptr)
			{
				auto& mtype = *member->UnderlyingType();
				if (!Eq(mtype.Name(), type) || !Eq(mtype.Module().Name(), typeSource))
				{
					Throw(0, "%s failed. Element type was %s of %s. Expected a %s of %s", __FUNCTION__, type, typeSource, member->UnderlyingType()->Name(), member->UnderlyingType()->Module().Name());
				}
			}

			memberRefManager.DescendToFirstChild();
		}

		template<class T> void AddItemValue(int32 itemIndex, T value)
		{
			if (itemIndex >= (int32)container.elements.size())
			{
				Throw(0, "%s: Bad index (%d)", __FUNCTION__, itemIndex);
			}

			if (container.elements[itemIndex].member != nullptr)
			{
				uint8* rawMemberData = container.elements[itemIndex].memberDataOffset + writePosition;
				auto* memberData = (T*)rawMemberData;
				*memberData = value;
			}
		}

		void AddF32ItemValue(int32 itemIndex, float value)
		{
			AddItemValue(itemIndex, value);
		}

		void AddF64ItemValue(int32 itemIndex, double value)
		{
			AddItemValue(itemIndex, value);
		}

		void AddI32ItemValue(int32 itemIndex, int32 value)
		{
			AddItemValue(itemIndex, value);
		}

		void AddI64ItemValue(int32 itemIndex, int64 value)
		{
			AddItemValue(itemIndex, value);
		}

		void AddBoolItemValue(int32 itemIndex, bool value)
		{
			boolean32 bValue = value ? 1 : 0;
			AddItemValue(itemIndex, bValue);
		}

		void AddObjectRefValue(int itemIndex, cstr objectName)
		{
			if (container.elements.size() > 0 && itemIndex >= (int32)container.elements.size())
			{
				Throw(0, "%s: Bad index (%d)", __FUNCTION__, itemIndex);
			}

			uint8* rawMemberData = writePosition;
			auto* memberData = (InterfacePointer*)rawMemberData;

			auto i = objects.find(objectName);
			if (i == objects.end())
			{
				DeserializedObject newObject;
				i = objects.insert(objectName, newObject).first;
			}

			RequiredInterfaceRef ref;
			ref.pInterfaceType = &type->GetInterface(0);
			ref.ppInterface = memberData;
			i->second.requiredInterfaces.push_back(ref);

			writePosition += sizeof InterfacePointer;
		}

		void AddNewObject(cstr name, cstr type, cstr sourceFile) override
		{
			auto i = objects.find(name);
			if (i == objects.end())
			{
				DeserializedObject object;
				i = objects.insert(name, object).first;
			}

			i->second.stub = scriptSystem->CreateScriptObject(type, sourceFile);

			auto* stub = i->second.stub;

			SelectTarget(*stub->Desc->TypeInfo, stub, *scriptSystem);
		}

		void Free() override
		{
			delete this;
		}

		bool IsIString(const IStructure& type) const
		{
			if (typeIString == nullptr)
			{
				auto& rootNS = scriptSystem->PublicProgramObject().GetRootNamespace();
				auto* sysType = rootNS.FindSubspace("Sys.Type");
				if (!sysType)
				{
					Throw(0, "Could not find Sys.Type");
				}

				auto* interfaceIString = sysType->FindInterface("IString");

				if (interfaceIString == nullptr)
				{
					Throw(0, "Could not find Sys.Type.IString");
				}

				typeIString = &interfaceIString->NullObjectType();
			}

			return typeIString == &type;
		}

		bool IsIStringBuilder(const IStructure& type) const
		{
			if (typeIStringBuilder == nullptr)
			{
				auto& rootNS = scriptSystem->PublicProgramObject().GetRootNamespace();
				auto* sysType = rootNS.FindSubspace("Sys.Type");
				if (!sysType)
				{
					Throw(0, "Could not find Sys.Type");
				}

				auto* interfaceIStringBuilder = sysType->FindInterface("IStringBuilder");

				if (interfaceIStringBuilder == nullptr)
				{
					Throw(0, "Could not find Sys.Type.IStringBuilder");
				}

				typeIStringBuilder = &interfaceIStringBuilder->NullObjectType();
			}

			return typeIStringBuilder == &type;
		}

		void SelectTarget(const Rococo::Compiler::IStructure& type, void* pObject, Rococo::Script::IPublicScriptSystem& ss) override
		{
			this->type = &type;
			this->pObject = (uint8*) pObject;
			this->scriptSystem = &ss;

			if (type.InterfaceCount() > 0 && pObject != nullptr)
			{
				InterfacePointer ip = (InterfacePointer)pObject;
				objectStub = InterfaceToInstance(ip);
				objectType = objectStub->Desc->TypeInfo;
				int32 extraVTables = objectType->InterfaceCount() - 1;
				writePosition = ((uint8*)objectStub) + sizeof(ObjectStub) + sizeof(VirtualTable*) * (size_t) extraVTables;
			}
			else
			{
				objectStub = nullptr;
				objectType = &type;
				writePosition = ((uint8*) pObject);
			}

			memberRefManager.InitToFirstChild();
		}

		IMemberBuilder& MemberBuilder()
		{
			return *this;
		}

		const Rococo::Compiler::IMember* GetMemberRefAndUpdateWriteCursor()
		{
			MemberAndOffset k = memberRefManager.GetMemberAndOffset(*type);

			if (k.member)
			{
				writeCursor = writePosition + (ptrdiff_t)k.offset;
			}
			else
			{
				writeCursor = nullptr;
			}

			return k.member;
		}

		static int GetMemberIndex(cstr name, const IStructure& s)
		{
			for (int i = 0; i < s.MemberCount(); ++i)
			{
				auto& m = s.GetMember(i);
				if (Eq(m.Name(), name))
				{
					return i;
				}
			}

			return -1;
		}

		const Rococo::Compiler::IMember* FindMemberAndUpdateWriteCursor(cstr name)
		{
			auto k = memberRefManager.FindMember(*type, name);

			if (k.member == nullptr)
			{
				writeCursor = nullptr;
				return nullptr;
			}

			writeCursor = writePosition + (ptrdiff_t) k.offset;
			return k.member;
		}

		const IMember* GetBestMatchingMember(cstr name)
		{
			const IMember* member = GetMemberRefAndUpdateWriteCursor();
			if (member)
			{
				if (!Eq(member->Name(), name))
				{
					member = FindMemberAndUpdateWriteCursor(name);
				}
			}
			else
			{
				member = FindMemberAndUpdateWriteCursor(name);
			}

			return member;
		}

		template<class PRIMITIVE> void WritePrimitive(PRIMITIVE value)
		{
			auto* primitiveWritePosition = reinterpret_cast<PRIMITIVE*>(writePosition);
			*primitiveWritePosition = value;
			writePosition += sizeof PRIMITIVE;
			memberRefManager.MoveToNextSibling();
		}

		void WriteNull(const IStructure& memberType)
		{
			switch (memberType.VarType())
			{
			case VARTYPE_Float32:
				WritePrimitive((float)0);
				break;
			case VARTYPE_Float64:
				WritePrimitive((double)0);
				break;
			case VARTYPE_Int32:
				WritePrimitive((int32)0);
				break;
			case VARTYPE_Int64:
				WritePrimitive((int64)0);
				break;
			case VARTYPE_Bool:
				WritePrimitive((boolean32)0);
				break;
			default:
				Throw(0, "No default serialization for %s", memberType.Name());
			}

			memberRefManager.MoveToNextSibling();
		}

		void AddBooleanMember(cstr name, bool value) override
		{
			const IMember* member = GetBestMatchingMember(name);
			
			if (!member)
			{
				Throw(0, "boolean32 %s. No member found", name);
			}
			else if (member->UnderlyingType()->VarType() == VARTYPE_Bool)
			{
				boolean32 bValue = value ? 1 : 0;
				WritePrimitive(bValue);
			}
			else
			{
				WriteNull(*member->UnderlyingType());
			}
		}

		void AddDoubleMember(cstr name, double value) override
		{
			const IMember* member = GetBestMatchingMember(name);

			if (!member)
			{
				Throw(0, "double %s. No member found", name);
			}
			else if (member->UnderlyingType()->VarType() == VARTYPE_Float64)
			{
				WritePrimitive(value);
			}
			else
			{
				WriteNull(*member->UnderlyingType());
			}
		}

		void AddFloatMember(cstr name, float value) override
		{
			const IMember* member = GetBestMatchingMember(name);

			if (!member)
			{
				Throw(0, "float %s. No member found", name);
			}
			else if (member->UnderlyingType()->VarType() == VARTYPE_Float32)
			{
				WritePrimitive(value);
			}
			else
			{
				WriteNull(*member->UnderlyingType());
			}
		}

		void AddInt32Member(cstr name, int32 value) override
		{
			const IMember* member = GetBestMatchingMember(name);

			if (!member)
			{
				Throw(0, "int32 %s. No member found", name);
			}
			else if (member->UnderlyingType()->VarType() == VARTYPE_Int32)
			{
				WritePrimitive(value);
			}
			else
			{
				WriteNull(*member->UnderlyingType());
			}
		}

		void AddInt64Member(cstr name, int64 value) override
		{
			const IMember* member = GetBestMatchingMember(name);

			if (!member)
			{
				Throw(0, "int64 %s. No member found", name);
			}
			else if (member->UnderlyingType()->VarType() == VARTYPE_Int64)
			{
				WritePrimitive(value);
			}
			else
			{
				WriteNull(*member->UnderlyingType());
			}
		}

		void AddFastStringBuilder(cstr name, fstring text, int32 capacity, cstr objectRefName)
		{
			const IMember* member = GetBestMatchingMember(name);

			if (!member)
			{
				Throw(0, "%s: No member found", name);
			}

			auto& type = *member->UnderlyingType();

			if (!IsIStringBuilder(type) && !IsIString(type))
			{
				Throw(0, "Expected %s to be of type IStringBuilder or IString, but was of type %s", name, type.Name());
			}

			FastStringBuilder* fb = scriptSystem->CreateAndPopulateFastStringBuilder(text, capacity);
			WritePrimitive(fb->stub.pVTables);
		}

		void AddStringConstant(cstr name, cstr text, int32 stringLength)
		{
			const IMember* member = GetBestMatchingMember(name);

			if (!member)
			{
				Throw(0, "%s: No member found", name);
			}

			auto& type = *member->UnderlyingType();

			if (!IsIString(type))
			{
				Throw(0, "Expected %s to be of type Sys.Type.IString, but was of type %s", name, type.Name());
			}

			CStringConstant* sc = scriptSystem->DuplicateStringAsConstant(text, stringLength);
			WritePrimitive(sc->header.pVTables);
		}

		void AddInterfaceMember(cstr name, cstr interfaceType, cstr interfaceSource, cstr instanceType, cstr instanceSource, OBJECT_NAME objectName) override
		{
			const Rococo::Compiler::IMember* member = GetBestMatchingMember(name);

			if (!member)
			{
				Throw(0, "%s %s. No member found", interfaceType, name);
			}

			auto& type = *member->UnderlyingType();

			if (type.InterfaceCount() != 1)
			{
				Throw(0, "%s. Member was not of interface type", name, interfaceType);
			}

			auto& interface0 = type.GetInterface(0);

			if (!Eq(interface0.Name(), interfaceType))
			{
				Throw(0, "%s. Member was of interface type %s, not of interface type", name, type.GetInterface(0).Name(), interfaceType);
			}

			if (!Eq(type.Module().Name(), interfaceSource))
			{
				Throw(0, "%s. Interface source was  %s, not %s", name, type.Module().Name(), interfaceSource);
			}

			if (Eq(objectName, "0"))
			{
				// We need to get the universal null object for the specified instance type
				InterfacePointer pNullObject = scriptSystem->GetUniversalNullObject(instanceType, instanceSource);
				WritePrimitive(pNullObject);
			}
			else
			{
				if (objectName[0] != '#')
				{
					Throw(0, "Unhandled object name: %s", objectName);
				}

				auto i = objects.find(objectName);
				if (i == objects.end())
				{
					DeserializedObject newObject;
					i = objects.insert(objectName, newObject).first;
				}

				RequiredInterfaceRef ref;
				ref.pInterfaceType = &interface0;
				ref.ppInterface = (InterfacePointer*)writePosition;
				i->second.requiredInterfaces.push_back(ref);

				writePosition += sizeof InterfacePointer;
			}			
		}

		void AddInterface(const Rococo::Compiler::IMember& member, const IInterface& expectedType, cstr type, cstr name, cstr sourceFile)
		{
			if (!Eq(expectedType.Name(), type))
			{
				Throw(0, "%s %s. Mismatches interface type %s", expectedType.Name(), name, type);
			}

			memberRefManager.DescendToFirstChild();
		}

		void AddDerivativeMember(cstr type, cstr name, cstr sourceFile) override
		{
			const Rococo::Compiler::IMember* member = GetBestMatchingMember(name);
			
			if (!member)
			{
				Throw(0, "%s %s. No member found", type, name);
			}
			
			auto* memberType = member->UnderlyingType();

			if (memberType->InterfaceCount() > 0)
			{
				auto& interface0 = memberType->GetInterface(0);
				AddInterface(*member, interface0, type, name, sourceFile);
				return;
			}

			if (!Eq(memberType->Name(), type))
			{
				Throw(0, "%s %s. Mismatches argument type %s", memberType->Name(), name, type);
			}
			else if (!Eq(memberType->Module().Name(), sourceFile))
			{
				Throw(0, "%s %s. Source file %s mismatches module name %s", memberType->Name(), name, sourceFile, memberType->Module().Name());
			}
			else if (memberType->VarType() != VARTYPE_Derivative)
			{
				Throw(0, "%s %s. Type was not derivative", memberType->Name(), name);
			}
			else
			{
				memberRefManager.DescendToFirstChild();
			}
		}

		void EnterDerivedContainerItem() override
		{
			
		}

		void LeaveDerivedContainerItem() override
		{
			
		}

		void ReturnToParent() override
		{
			memberRefManager.MoveToParent();
			memberRefManager.MoveToNextSibling();
		}

		void ResolveReferences()
		{
			for (auto& i : objects)
			{
				cstr name = i.first;
				auto& object = i.second;

				auto& concreteType = *object.stub->Desc->TypeInfo;

				for (auto& requiredInterfaceRef : object.requiredInterfaces)
				{
					int interfaceIndex = GetIntefaceIndex(concreteType, requiredInterfaceRef.pInterfaceType);

					InterfacePointer ip = object.stub->pVTables + interfaceIndex;
					*requiredInterfaceRef.ppInterface = ip;
				}

				object.stub->refCount = (int64) object.requiredInterfaces.size();
			}
		}

		void AddArrayDefinition(cstr refName, cstr elementType, cstr elementTypeSource, int32 length, int32 capacity) override
		{
			auto* type = FindStructure(*scriptSystem, elementType, elementTypeSource);
			if (!type)
			{
				Throw(0, "Could not resolve type %s of %s", elementType, elementTypeSource);
			}

			auto i = arrays.find(refName);
			if (i == arrays.end())
			{
				auto* image = scriptSystem->CreateArrayImage(*type);
				i = arrays.insert(refName, image).first;
			}
			else
			{
				if (i->second->ElementType != type)
				{
					Throw(0, "Cannot load %s.\nThe array has already been defined as being type %s of %s.\nThe operation requested a type %s of %s.", refName, i->second->ElementType->Name(), i->second->ElementType->Module().Name(), type->Name(), type->Module().Name());
				}
			}

			auto* image = i->second;

			// Edge case -> capacity * image->ElementType->SizeOfStruct() may not fit in an int32

			int64 totalLen = (int64)capacity * (int64)image->ElementLength;
			if (totalLen > 0x7FFFFFFFLL)
			{
				Throw(0, "Max capacity exceeded attempting to reserve %d elements for %s", capacity, refName);
			}

			void* elementBuffer = scriptSystem->AlignedMalloc(16, capacity * image->ElementLength);
			if (elementBuffer == nullptr)
			{
				Throw(0, "Could not reserve %d elements for %s", capacity, refName);
			}

			image->Start = elementBuffer;
			image->ElementCapacity = capacity;
			image->NumberOfElements = length;

			arrayBuilder.image = image;
			arrayBuilder.elementBuffer = (uint8*) image->Start;

			SelectTarget(*arrayBuilder.image->ElementType, nullptr, *scriptSystem);
		}

		void SetArrayWriteIndex(int32 index) override
		{
			if (index > arrayBuilder.image->NumberOfElements)
			{
				Throw(0, "%s. Bad index", __FUNCTION__);
			}

			auto* elementPtr = arrayBuilder.elementBuffer + index * arrayBuilder.image->ElementLength;

			if (IsNullType(*arrayBuilder.image->ElementType))
			{
				objectStub = nullptr;
				objectType = arrayBuilder.image->ElementType;
				writePosition = elementPtr;
				memberRefManager.InitToTypeItself();
			}
			else
			{
				SelectTarget(*arrayBuilder.image->ElementType, elementPtr, *scriptSystem);
			}
		}
	};

	struct SexyAssetLoader: IAssetLoader
	{
		IInstallation& installation;
		SexyObjectBuilder objectBuilder;
		AutoFree<ICSVTokenParser> sxyaParser = CreateSXYAParser(objectBuilder);
		AutoFree<ITabbedCSVTokenizer> tabbedTokenizer = CreateTabbedCSVTokenizer();
		
		SexyAssetLoader(IInstallation& _installation) :
			installation(_installation),
			objectBuilder()
		{
		}

		void Free() override
		{
			delete this;
		}

		void LoadAndParse(cstr pingPath, const IStructure& assetType, void* assetData, Rococo::Script::IPublicScriptSystem& ss) override
		{
			WideFilePath sysPath;
			installation.ConvertPingPathToSysPath(pingPath, sysPath);

			objectBuilder.SelectTarget(assetType, assetData, ss);

			struct ANON: IEventCallback<cstr>
			{
				SexyAssetLoader* This = nullptr;

				void OnEvent(cstr csvString) override
				{
					// Tokenize the CSV file, which sends tokens into the sxyaParser, which populates the asset via objectBuilder
					This->tabbedTokenizer->Tokenize(csvString, *This->sxyaParser);
				}
			} parse;

			parse.This = this;

			try
			{
				Rococo::OS::LoadAsciiTextFile(parse, sysPath);
				objectBuilder.ResolveReferences();
			}
			catch (IException& e)
			{
				Throw(e.ErrorCode(), "Error loading asset %s:\n%s", pingPath, e.Message());
			}
		}
	};
}

namespace Rococo::Sexy
{
	ISexyObjectBuilder* CreateSexyObjectBuilder()
	{
		return new SexyObjectBuilder();
	}

	IAssetLoader* CreateAssetLoader(IInstallation& installation)
	{
		return new SexyAssetLoader(installation);
	}
}

namespace Rococo::Script
{
	const IStructure* FindStructure(IPublicScriptSystem& ss, cstr localTypeName, cstr moduleName, bool throwOnError)
	{
		auto& obj = ss.PublicProgramObject();
		for (int i = 0; i < obj.ModuleCount(); ++i)
		{
			auto& module = obj.GetModule(i);
			if (Eq(module.Name(), moduleName))
			{
				auto* type = module.FindStructure(localTypeName);
				if (!type && throwOnError)
				{
					Throw(0, "%s(ss, %s, %s, true) failed. Could not find structure in module.", __FUNCTION__, localTypeName, moduleName);
				}
				return type;
			}
		}

		if (throwOnError)
		{
			Throw(0, "%s(ss, %s, %s, true) failed. Could not find module.", __FUNCTION__, localTypeName, moduleName);
		}

		return nullptr;
	}
}
