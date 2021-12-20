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
		const Rococo::Compiler::IStructure* rootType = nullptr;
		void* pRootObject = nullptr;

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

		void AddArrayRefValue(int memberIndex, cstr arrayName) override
		{
			if (memberIndex >= container.elements.size())
			{
				Throw(0, "%s: Bad index (%d)", __FUNCTION__, memberIndex);
			}

			auto* member = container.elements[memberIndex].member;

			if (member != nullptr)
			{
				auto& elementType = *member->UnderlyingGenericArg1Type();

				auto i = arrays.find(arrayName);
				if (i == arrays.end())
				{
					auto* image = scriptSystem->CreateArrayImage(elementType);
					i = arrays.insert(arrayName, image).first;
				}
				else
				{
					if (i->second->ElementType != &elementType)
					{
						Throw(0, "Cannot load %s. The target member [%s] type %s is not the same as the array type %s", arrayName, member->Name(), member->UnderlyingGenericArg1Type()->Name(), i->second->ElementType->Name());
					}

					i->second->RefCount++;
				}

				uint8* rawMemberData = container.elements[memberIndex].memberDataOffset + writePosition;
				auto* ppA = (ArrayImage**)rawMemberData;
				if (*ppA != nullptr)
				{
					// If we allowed the C++ to overwrite an array, we would need to handle the case of reference counts going to zero,
					// which entails invoking proper destructors for each array element. We eliminate the complexities by ensuring this will not happen.
					// It is up to the script programmer to ensure arrays are nulled out prior to deserialization.
					auto* a = *ppA;
					Throw(0, "Cannot load %s. The target member array [%s] is not null. Overwriting of existing arrays is not permitted.\nThe length was %d and the capacity was %d", arrayName, member->Name(), a->NumberOfElements, a->ElementCapacity);
				}
				*ppA = i->second;
			}
		}

		void AddMemberType(int32 memberDepth, cstr memberName, VARTYPE validType)
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

		void AddTypeF32(int32 memberDepth, cstr memberName) override
		{
			AddMemberType(memberDepth, memberName, VARTYPE_Float32);
		}

		void AddTypeF64(int32 memberDepth, cstr memberName) override
		{
			AddMemberType(memberDepth, memberName, VARTYPE_Float64);
		}

		void AddTypeI32(int32 memberDepth, cstr memberName) override
		{
			AddMemberType(memberDepth, memberName, VARTYPE_Int32);
		}

		void AddTypeI64(int32 memberDepth, cstr memberName) override
		{
			AddMemberType(memberDepth, memberName, VARTYPE_Int64);
		}

		void AddTypeBool(int32 memberDepth, cstr memberName) override
		{
			AddMemberType(memberDepth, memberName, VARTYPE_Bool);
		}

		void AddTypeArrayRef(int32 memberDepth, cstr memberName) override
		{
			AddMemberType(memberDepth, memberName, VARTYPE_Array);
		}

		void AddTypeInterface(int32 memberDepth, cstr interfaceType, cstr memberName, cstr sourceFile)
		{
			memberRefManager.RollbackToAncestor(memberDepth + 1);

			const IMember* member = GetBestMatchingMember(memberName);

			if (member != nullptr)
			{
				auto& mtype = *member->UnderlyingType();
				if (!IsNullType(mtype))
				{
					Throw(0, "%s failed. Element type was %s of %s. Expected null type (interface type)", __FUNCTION__, mtype.Name(), mtype.Module().Name());
				}

				auto& i = mtype.GetInterface(0);

				if (!Eq(i.Name(), interfaceType))
				{
					Throw(0, "%s failed. Member interface was %s of %s. Expected interface type %s of %s", __FUNCTION__, i.Name(), mtype.Module().Name(), interfaceType, sourceFile);
				}

				container.elements.push_back(ElementMemberDesc{ member, writeCursor - writePosition });
			}
			else
			{
				container.elements.push_back(ElementMemberDesc{ nullptr, 0 });
			}

			memberRefManager.DescendToFirstChild();
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
			uint8* rawMemberData;

			if (!container.elements.empty())
			{
				if (itemIndex >= (int32)container.elements.size())
				{
					Throw(0, "%s: Bad index (%d)", __FUNCTION__, itemIndex);
				}
				rawMemberData = container.elements[itemIndex].memberDataOffset + writePosition;
			}
			else
			{
				if (elementType == nullptr)
				{
					Throw(0, "%s failed. ElementType was nullptr", __FUNCTION__);
				}
				rawMemberData = writePosition;
			}

			auto* pInterface = (InterfacePointer**)rawMemberData;

			auto i = objects.find(objectName);
			if (i == objects.end())
			{
				DeserializedObject newObject;
				i = objects.insert(objectName, newObject).first;
			}

			RequiredInterfaceRef ref;

			if (!container.elements.empty())
			{
				auto* member = container.elements[itemIndex].member;
				auto& memberType = *member->UnderlyingType();

				ref.pInterfaceType = &memberType.GetInterface(0);
			}
			else
			{ 
				ref.pInterfaceType = &elementType->GetInterface(0);	
			}

			ref.ppInterface = *pInterface;
			i->second.requiredInterfaces.push_back(ref);
		}

		void BuildObject(cstr name, cstr type, cstr sourceFile) override
		{
			container.elements.clear();
			elementType = nullptr;

			auto i = objects.find(name);
			if (i == objects.end())
			{
				DeserializedObject object;
				i = objects.insert(name, object).first;
			}

			if (Eq(name, "#Object0"))
			{
				if (!Eq(rootType->Name(), type))
				{
					Throw(0, "Type mismatch: the root object in the archive is %s of %s. Expected %s of %s.", type, sourceFile, rootType->Name(), rootType->Module().Name());
				}
				else if (!Eq(rootType->Module().Name(), sourceFile))
				{
					Throw(0, "Module mismatch: the root object in the archive is %s of %s. Expected %s of %s.", type, sourceFile, rootType->Name(), rootType->Module().Name());
				}

				// N.B the root object might not be an object stub, but in this case it will not be referenced by any other object
				i->second.stub = (ObjectStub*) pRootObject;
				SelectTarget(*rootType, pRootObject);
			}
			else
			{
				i->second.stub = scriptSystem->CreateScriptObject(type, sourceFile);
				SelectTarget(*i->second.stub->Desc->TypeInfo, i->second.stub);
			}
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

		void SelectScriptSystem(Rococo::Script::IPublicScriptSystem& ss) override
		{
			this->scriptSystem = &ss;
		}

		void SelectRootTarget(const Rococo::Compiler::IStructure& rootType, void* pRootObject)
		{
			this->rootType = &rootType;
			this->pRootObject = pRootObject;

			if (rootType.VarType() != VARTYPE_Derivative)
			{
				Throw(0, "The target object was not of derivative type. The target object must be a class or struct");
			}
			else if (IsNullType(rootType))
			{
				Throw(0, "The target object was an interface reference. The target object must be a class or struct.");
			}
		}

		void SelectTarget(const Rococo::Compiler::IStructure& type, void* pObject) override
		{
			this->type = &type;
			this->pObject = (uint8*) pObject;

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

		void PopulateRequiredInterface(DeserializedObject& object)
		{
			for (auto& requiredInterface : object.requiredInterfaces)
			{
				if (requiredInterface.ppInterface)
				{
					auto& type = *object.stub->Desc->TypeInfo;
					for (int i = 0; i < type.InterfaceCount(); i++)
					{
						if (requiredInterface.pInterfaceType == &type.GetInterface(i))
						{
							*requiredInterface.ppInterface = object.stub->pVTables + i;
						}
					}
				}
			}

			object.requiredInterfaces.clear();
		}

		void AddFastStringBuilder(cstr objectRefName, fstring text, int32 capacity)
		{
			FastStringBuilder* fb = scriptSystem->CreateAndPopulateFastStringBuilder(text, capacity);

			auto i = objects.find(objectRefName);
			if (i == objects.end())
			{
				DeserializedObject newObject;
				i = objects.insert(objectRefName, newObject).first;
			}

			i->second.stub = &fb->stub;
		}

		void AddStringConstant(cstr stringRefName, cstr text, int32 stringLength)
		{
			CStringConstant* sc = scriptSystem->DuplicateStringAsConstant(text, stringLength);
			
			auto i = objects.find(stringRefName);
			if (i == objects.end())
			{
				DeserializedObject newObject;
				i = objects.insert(stringRefName, newObject).first;
			}

			i->second.stub = &sc->header;
		}

		void AddNullObject(cstr objectNameRef, cstr nullType, cstr nullTypeModule) override
		{
			auto* nullObjectInterface = scriptSystem->GetUniversalNullObject(nullType, nullTypeModule);
			auto* nullObjectStub = InterfaceToInstance(nullObjectInterface);

			auto i = objects.find(objectNameRef);
			if (i == objects.end())
			{
				DeserializedObject newObject;
				i = objects.insert(objectNameRef, newObject).first;
			}

			i->second.stub = nullObjectStub;
		}

		/*
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
		*/

		void AddTypeDerivative(int memberDepth, cstr type, cstr name, cstr sourceFile) override
		{
			const Rococo::Compiler::IMember* member = GetBestMatchingMember(name);
			
			if (!member)
			{
				Throw(0, "%s %s. No member found", type, name);
			}
			
			auto* memberType = member->UnderlyingType();

			if (IsNullType(*memberType))
			{
				Throw(0, "Bad algorithm");
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

		const IStructure* elementType = nullptr;

		void AddArrayDefinition(cstr refName, cstr elementTypeName, cstr elementTypeSource, int32 length, int32 capacity) override
		{
			container.elements.clear();

			elementType = FindStructure(*scriptSystem, elementTypeName, elementTypeSource);
			if (!elementType)
			{
				Throw(0, "Could not resolve type %s of %s", elementTypeName, elementTypeSource);
			}

			auto i = arrays.find(refName);
			if (i == arrays.end())
			{
				auto* image = scriptSystem->CreateArrayImage(*elementType);
				i = arrays.insert(refName, image).first;
			}
			else
			{
				if (i->second->ElementType != elementType)
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

			SelectTarget(*arrayBuilder.image->ElementType, nullptr);
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
				SelectTarget(*arrayBuilder.image->ElementType, elementPtr);
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

			objectBuilder.SelectScriptSystem(ss);
			objectBuilder.SelectRootTarget(assetType, assetData);

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
