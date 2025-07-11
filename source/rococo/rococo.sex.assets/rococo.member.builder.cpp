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

#include <rococo.sexy.map.expert.h>

#include <rococo.maths.h>

using namespace Rococo;
using namespace Rococo::IO;
using namespace Rococo::OS;
using namespace Rococo::Compiler;
using namespace Rococo::Sexy;
using namespace Rococo::Script;
using namespace Rococo::Strings;

namespace Rococo::Sex::Assets::Impl
{
	inline ObjectStub* InterfaceToInstance(InterfacePointer i)
	{
		auto* p = ((uint8*)i) + (*i)->OffsetToInstance;
		auto* obj = (ObjectStub*)p;
		return obj;
	}

	bool DoesInterfaceOrBaseMatch(const IInterface& target, const IInterface& sourceWithPossibleBase)
	{
		for (const IInterface* i = &sourceWithPossibleBase; i != nullptr; i = i->Base())
		{
			if (i == &target)
			{
				return true;
			}
		}

		return false;
	}

	int GetIntefaceIndex(const IStructure& concreteType, const IInterface* interfaceType)
	{
		for (int j = 0; j < concreteType.InterfaceCount(); ++j)
		{
			auto& interface = concreteType.GetInterface(j);
			if (DoesInterfaceOrBaseMatch(*interfaceType, interface))
			{
				return j;
			}
		}

		Rococo::Throw(0, "%s does not support interface %s of %s", concreteType.Name(), interfaceType->Name(), interfaceType->NullObjectType().Module().Name());
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
					Rococo::Throw(0, "%s called too many times. Algorithmic error", __ROCOCO_FUNCTION__);
				}
			}
		} memberRefManager;

		IPublicScriptSystem* scriptSystem = nullptr;

		mutable const IStructure* typeIString = nullptr;
		mutable const IStructure* typeIStringBuilder = nullptr;

		struct RequiredInterfaceRef
		{
			// The type of interface we need to define
			const IInterface* pInterfaceType;

			// The address in memory of the interface pointer to overwrite
			InterfacePointer* ppInterface;
		};

		struct DeserializedObject
		{
			ObjectStub* stub = nullptr;

			// The set of interface pointers that need to be overwritten with the object
			std::vector<RequiredInterfaceRef> requiredInterfaces;
		};

		struct RequiredArray
		{
			// The address in memory of the array pointer to overwrite
			ArrayImage** pImageWritePosition;
		};

		stringmap<DeserializedObject> objects;
		stringmap<ArrayImage*> arrays;
		stringmap<ListImage*> lists;
		stringmap<MapImage*> maps;

		struct ElementMemberDesc
		{
			const IMember* member;

			// The number of bytes from the begining of the element to the member
			ptrdiff_t memberDataOffset;
		};

		// The target container to build 
		struct Container
		{
			// The set of primitive members that make up the data of a type
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

		void AddListRefValue(int itemIndex, cstr listName) override
		{
			if (itemIndex >= container.elements.size())
			{
				Rococo::Throw(0, "%s: Bad index (%d)", __ROCOCO_FUNCTION__, itemIndex);
			}

			auto* member = container.elements[itemIndex].member;

			if (member != nullptr)
			{
				auto& valueType = *member->UnderlyingGenericArg1Type();

				ListImage* image;

				if (Eq(listName, "<null>"))
				{
					image = nullptr;
				}
				else
				{
					auto i = lists.find(listName);
					if (i == lists.end())
					{
						image = scriptSystem->CreateListImage(valueType);
						i = lists.insert(listName, image).first;
					}
					else
					{
						if (i->second->ElementType != &valueType)
						{
							Rococo::Throw(0, "Cannot load %s. The target member [%s] value type %s is not the same as the list value type %s", listName, member->Name(), GetFriendlyName(valueType), GetFriendlyName(*i->second->ElementType));
						}

						i->second->refCount++;
					}

					image = i->second;
				}

				uint8* rawMemberData = container.elements[itemIndex].memberDataOffset + writePosition;
				auto* ppA = (ListImage**)rawMemberData;
				if (*ppA != nullptr)
				{
					// If we allowed the C++ to overwrite an array, we would need to handle the case of reference counts going to zero,
					// which entails invoking proper destructors for each array element. We eliminate the complexities by ensuring this will not happen.
					// It is up to the script programmer to ensure arrays are nulled out prior to deserialization.
					auto* currentList = *ppA;
					Rococo::Throw(0, "Cannot load %s. The target member list [%s] is not null. Overwriting of existing lists is not permitted.\nThe length was %d", listName, member->Name(), currentList->NumberOfElements);
				}
				*ppA = image;
			}
		}

		void AddMapRefValue(int itemIndex, cstr mapName) override
		{
			if (itemIndex >= container.elements.size())
			{
				Rococo::Throw(0, "%s: Bad index (%d)", __ROCOCO_FUNCTION__, itemIndex);
			}

			auto* member = container.elements[itemIndex].member;

			if (member != nullptr)
			{
				auto& keyType = *member->UnderlyingGenericArg1Type();
				auto& valueType = *member->UnderlyingGenericArg2Type();

				MapImage* image;

				if (Eq(mapName, "<null>"))
				{
					image = nullptr;
				}
				else
				{
					auto i = maps.find(mapName);
					if (i == maps.end())
					{
						MapImage* newImage = scriptSystem->CreateMapImage(keyType, valueType);
						i = maps.insert(mapName, newImage).first;
					}
					else
					{
						if (i->second->KeyType != &keyType)
						{
							Rococo::Throw(0, "Cannot load %s. The target member [%s] key type %s is not the same as the map key type %s", mapName, member->Name(), GetFriendlyName(*member->UnderlyingGenericArg1Type()), GetFriendlyName(*i->second->KeyType));
						}

						if (i->second->ValueType != &valueType)
						{
							Rococo::Throw(0, "Cannot load %s. The target member [%s] value type %s is not the same as the map value type %s", mapName, member->Name(), GetFriendlyName(*member->UnderlyingGenericArg2Type()), GetFriendlyName(*i->second->ValueType));
						}

						i->second->refCount++;
					}

					image = i->second;
				}

				uint8* rawMemberData = container.elements[itemIndex].memberDataOffset + writePosition;
				auto* ppA = (MapImage**)rawMemberData;
				if (*ppA != nullptr)
				{
					// If we allowed the C++ to overwrite an array, we would need to handle the case of reference counts going to zero,
					// which entails invoking proper destructors for each array element. We eliminate the complexities by ensuring this will not happen.
					// It is up to the script programmer to ensure arrays are nulled out prior to deserialization.
					auto* currentMap = *ppA;
					Rococo::Throw(0, "Cannot load %s. The target member map [%s] is not null. Overwriting of existing maps is not permitted.\nThe length was %d", mapName, member->Name(), currentMap->NumberOfElements);
				}
				*ppA = image;
			}
		}

		void AddArrayRefValue(int memberIndex, cstr arrayName) override
		{
			if (memberIndex >= container.elements.size())
			{
				Rococo::Throw(0, "%s: Bad index (%d)", __ROCOCO_FUNCTION__, memberIndex);
			}

			auto* member = container.elements[memberIndex].member;

			if (member != nullptr)
			{
				auto& elementType = *member->UnderlyingGenericArg1Type();

				ArrayImage* image;

				if (Eq(arrayName, "<null>"))
				{
					image = nullptr;
				}
				else
				{
					auto i = arrays.find(arrayName);
					if (i == arrays.end())
					{
						auto* newImage = scriptSystem->CreateArrayImage(elementType);
						i = arrays.insert(arrayName, newImage).first;
					}
					else
					{
						if (i->second->ElementType != &elementType)
						{
							Rococo::Throw(0, "Cannot load %s. The target member [%s] type %s is not the same as the array type %s", arrayName, member->Name(), member->UnderlyingGenericArg1Type()->Name(), i->second->ElementType->Name());
						}

						i->second->RefCount++;
					}

					image = i->second;
				}

				uint8* rawMemberData = container.elements[memberIndex].memberDataOffset + writePosition;
				auto* ppA = (ArrayImage**)rawMemberData;
				if (*ppA != nullptr)
				{
					// If we allowed the C++ to overwrite an array, we would need to handle the case of reference counts going to zero,
					// which entails invoking proper destructors for each array element. We eliminate the complexities by ensuring this will not happen.
					// It is up to the script programmer to ensure arrays are nulled out prior to deserialization.
					auto* a = *ppA;
					Rococo::Throw(0, "Cannot load %s. The target member array [%s] is not null. Overwriting of existing arrays is not permitted.\nThe length was %d and the capacity was %d", arrayName, member->Name(), a->NumberOfElements, a->ElementCapacity);
				}
				*ppA = image;
			}
		}

		void AddMemberType(int32 memberDepth, cstr memberName, SexyVarType validType)
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
			AddMemberType(memberDepth, memberName, SexyVarType_Float32);
		}

		void AddTypeF64(int32 memberDepth, cstr memberName) override
		{
			AddMemberType(memberDepth, memberName, SexyVarType_Float64);
		}

		void AddTypeI32(int32 memberDepth, cstr memberName) override
		{
			AddMemberType(memberDepth, memberName, SexyVarType_Int32);
		}

		void AddTypeI64(int32 memberDepth, cstr memberName) override
		{
			AddMemberType(memberDepth, memberName, SexyVarType_Int64);
		}

		void AddTypeBool(int32 memberDepth, cstr memberName) override
		{
			AddMemberType(memberDepth, memberName, SexyVarType_Bool);
		}

		void AddTypeArrayRef(int32 memberDepth, cstr memberName) override
		{
			AddMemberType(memberDepth, memberName, SexyVarType_Array);
		}

		void AddTypeListRef(int32 memberDepth, cstr memberName) override
		{
			AddMemberType(memberDepth, memberName, SexyVarType_List);
		}

		void AddTypeMapRef(int32 memberDepth, cstr memberName) override
		{
			AddMemberType(memberDepth, memberName, SexyVarType_Map);
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
					Rococo::Throw(0, "%s failed. Element type was %s of %s. Expected null type (interface type)", __ROCOCO_FUNCTION__, mtype.Name(), mtype.Module().Name());
				}

				auto& i = mtype.GetInterface(0);

				if (!Eq(i.Name(), interfaceType))
				{
					Rococo::Throw(0, "%s failed. Member interface was %s of %s. Expected interface type %s of %s", __ROCOCO_FUNCTION__, i.Name(), mtype.Module().Name(), interfaceType, sourceFile);
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
					Rococo::Throw(0, "%s failed. Element type was %s of %s. Expected a %s of %s", __ROCOCO_FUNCTION__, type, typeSource, member->UnderlyingType()->Name(), member->UnderlyingType()->Module().Name());
				}
			}

			memberRefManager.DescendToFirstChild();
		}

		template<class T> void AddItemValue(int32 itemIndex, T value)
		{
			if (!container.elements.empty())
			{
				if (itemIndex >= (int32)container.elements.size())
				{
					Rococo::Throw(0, "%s: Bad index (%d)", __ROCOCO_FUNCTION__, itemIndex);
				}

				if (container.elements[itemIndex].member != nullptr)
				{
					uint8* rawMemberData = container.elements[itemIndex].memberDataOffset + writePosition;
					auto* memberData = (T*)rawMemberData;
					*memberData = value;
				}
			}
			else
			{
				auto* memberData = (T*)writePosition;
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
					Rococo::Throw(0, "%s: Bad index (%d)", __ROCOCO_FUNCTION__, itemIndex);
				}
				rawMemberData = container.elements[itemIndex].memberDataOffset + writePosition;
			}
			else
			{
				if (elementType == nullptr)
				{
					Rococo::Throw(0, "%s failed. ElementType was nullptr", __ROCOCO_FUNCTION__);
				}
				rawMemberData = writePosition;
			}

			auto* pInterface = (InterfacePointer*)rawMemberData;

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

			ref.ppInterface = pInterface;
			i->second.requiredInterfaces.push_back(ref);
		}

		void CacheStub(const IStructure& objectType, cstr name, void* data)
		{
			// In the event that the object has an interface, then it will have a stub, otherwise its a struct or primitive and is not cached
			if (objectType.InterfaceCount() > 0)
			{
				auto i = objects.find(name);
				if (i == objects.end())
				{
					DeserializedObject object;
					i = objects.insert(name, object).first;
				}

				i->second.stub = (ObjectStub*) data;
			}
		}

		void BuildObject(cstr name, cstr type, cstr sourceFile) override
		{
			container.elements.clear();
			elementType = nullptr;
			
			const IStructure& objectType = scriptSystem->GetTypeForSource(type, sourceFile);

			if (!Rococo::Script::IsSerializable(objectType))
			{
				Rococo::Throw(0, "Bad object %s of type %s defined in %s. Type is not serializable.", name, type, sourceFile);
			}

			if (Eq(name, "#Object0"))
			{
				if (!Eq(rootType->Name(), type))
				{
					Rococo::Throw(0, "Type mismatch: the root object in the archive is %s of %s. Expected %s of %s.", type, sourceFile, rootType->Name(), rootType->Module().Name());
				}
				else if (!Eq(rootType->Module().Name(), sourceFile))
				{
					Rococo::Throw(0, "Module mismatch: the root object in the archive is %s of %s. Expected %s of %s.", type, sourceFile, rootType->Name(), rootType->Module().Name());
				}

				CacheStub(objectType, name, pRootObject);
				SelectTarget(*rootType, pRootObject);
			}
			else
			{
				ObjectStub* newObject = scriptSystem->CreateScriptObject(type, sourceFile);
				CacheStub(objectType, name, newObject);
				SelectTarget(objectType, newObject);
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
					Rococo::Throw(0, "Could not find Sys.Type");
				}

				auto* interfaceIString = sysType->FindInterface("IString");

				if (interfaceIString == nullptr)
				{
					Rococo::Throw(0, "Could not find Sys.Type.IString");
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
					Rococo::Throw(0, "Could not find Sys.Type");
				}

				auto* interfaceIStringBuilder = sysType->FindInterface("IStringBuilder");

				if (interfaceIStringBuilder == nullptr)
				{
					Rococo::Throw(0, "Could not find Sys.Type.IStringBuilder");
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

			if (rootType.VarType() != SexyVarType_Derivative)
			{
				Rococo::Throw(0, "The target object was not of derivative type. The target object must be a class or struct");
			}
			else if (IsNullType(rootType))
			{
				Rococo::Throw(0, "The target object was an interface reference. The target object must be a class or struct.");
			}
		}

		void NullifyDerivativeMember(const IMember& member, const IStructure& type, void* memberData)
		{
			// This function is not well tested. As they say in Terrahawks, 'expect the unexpected'
			if (IsNullType(*member.UnderlyingType()))
			{
				InterfacePointer* ip = (InterfacePointer*)memberData;

				auto* stub = type.GetInterface(0).UniversalNullInstance();
				*ip = stub->pVTables;
			}
			else
			{
				Nullify(type, memberData);
			}
		}

		void Nullify(const IStructure& assetType, void* assetData)
		{
			// This function is not well tested. As they say in Terrahawks, 'expect the unexpected'

			if (assetType.VarType() != SexyVarType_Derivative)
			{
				Rococo::Throw(0, "%s: Error, the asset object was not of derivative type.", __ROCOCO_FUNCTION__);
			}
			else if (IsNullType(assetType))
			{
				Rococo::Throw(0, "%s: Error, the asset object was an interface reference.", __ROCOCO_FUNCTION__);
			}			

			size_t offset = 0;

			for (int i = 0; i < assetType.MemberCount(); ++i)
			{
				auto& member = assetType.GetMember(i);
				auto& memberType = *member.UnderlyingType();

				switch (memberType.VarType())
				{
				case SexyVarType_Derivative:
					NullifyDerivativeMember(member, memberType, ((uint8*) assetData) + offset);
					break;
				default:
					memset(assetData, 0, assetType.SizeOfStruct());
					break;
				}
				
				offset += member.SizeOfMember();
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
				writePosition = (uint8*)objectStub; // Although the write position is the object stub, the member offsets will be set to beyond the vtables
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
			writePosition += sizeof(PRIMITIVE);
			memberRefManager.MoveToNextSibling();
		}

		void WriteNull(const IStructure& memberType)
		{
			switch (memberType.VarType())
			{
			case SexyVarType_Float32:
				WritePrimitive((float)0);
				break;
			case SexyVarType_Float64:
				WritePrimitive((double)0);
				break;
			case SexyVarType_Int32:
				WritePrimitive((int32)0);
				break;
			case SexyVarType_Int64:
				WritePrimitive((int64)0);
				break;
			case SexyVarType_Bool:
				WritePrimitive((boolean32)0);
				break;
			default:
				Rococo::Throw(0, "No default serialization for %s", memberType.Name());
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
			CStringConstant* sc = scriptSystem->ReflectTransientStringByDuplication(text, stringLength);
			
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

		void AddTypeDerivative(int memberDepth, cstr type, cstr name, cstr sourceFile) override
		{
			UNUSED(memberDepth);

			const Rococo::Compiler::IMember* member = GetBestMatchingMember(name);
			
			if (!member)
			{
				Rococo::Throw(0, "%s %s. No member found", type, name);
			}
			
			auto* memberType = member->UnderlyingType();

			if (IsNullType(*memberType))
			{
				Rococo::Throw(0, "Bad algorithm");
				return;
			}

			if (!Eq(memberType->Name(), type))
			{
				Rococo::Throw(0, "%s %s. Mismatches argument type %s", memberType->Name(), name, type);
			}
			else if (!Eq(memberType->Module().Name(), sourceFile))
			{
				Rococo::Throw(0, "%s %s. Source file %s mismatches module name %s", memberType->Name(), name, sourceFile, memberType->Module().Name());
			}
			else if (memberType->VarType() != SexyVarType_Derivative)
			{
				Rococo::Throw(0, "%s %s. Type was not derivative", memberType->Name(), name);
			}
			else
			{
				memberRefManager.DescendToFirstChild();
			}
		}

		void ReturnToParent() override
		{
			memberRefManager.MoveToParent();
			memberRefManager.MoveToNextSibling();
		}

		void DeleteNewObjects()
		{
			// This function is not well tested. As they say in Terrahawks, 'expect the unexpected'
			for (auto& i : objects)
			{
				cstr name = i.first;
				UNUSED(name);

				auto& object = i.second;

				if (object.stub)
				{
					scriptSystem->AlignedFree(object.stub);
				}
			}

			for (auto& a : arrays)
			{
				scriptSystem->AlignedFree(a.second);
			}

			for (auto& l : lists)
			{
				scriptSystem->AlignedFree(l.second);
			}

			for (auto& m : maps)
			{
				scriptSystem->AlignedFree(m.second);
			}

			objects.clear();
		}

		void ResolveReferences()
		{
			for (auto& i : objects)
			{
				cstr name = i.first;
				UNUSED(name);

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

		const IStructure* mapKeyType = nullptr;
		const IStructure* mapValueType = nullptr;
		MapImage* targetMap = nullptr;
		Rococo::Script::MapNode* targetNode = nullptr;

		void AddMapDefinition(cstr refName, cstr keyType, cstr keyTypeSource, cstr valueType, cstr valueTypeSource, int32 length) override
		{
			UNUSED(length);
			mapKeyType = &scriptSystem->GetTypeForSource(keyType, keyTypeSource);
			mapValueType = &scriptSystem->GetTypeForSource(valueType, valueTypeSource);

			auto i = maps.find(refName);
			if (i == maps.end())
			{
				auto* image = scriptSystem->CreateMapImage(*mapKeyType, *mapValueType);
				i = maps.insert(refName, image).first;
			}
			else
			{
				if (i->second->KeyType != mapKeyType)
				{
					Rococo::Throw(0, "Cannot load %s.\nThe map key type has already been defined as being type %s of %s.\nThe operation requested a type %s of %s.", refName, i->second->KeyType->Name(), i->second->KeyType->Module().Name(), mapKeyType->Name(), mapKeyType->Module().Name());
				}

				if (i->second->ValueType != mapValueType)
				{
					Rococo::Throw(0, "Cannot load %s.\nThe map value type has already been defined as being type %s of %s.\nThe operation requested a type %s of %s.", refName, i->second->ValueType->Name(), i->second->ValueType->Module().Name(), mapValueType->Name(), mapValueType->Module().Name());
				}
			}

			targetMap = i->second;
		}

		void SetMapKey(const fstring& keyText)
		{
			void* elementPtr = nullptr;

			VariantValue key;

			if (IsIString(*mapKeyType))
			{
				auto* sc = scriptSystem->ReflectTransientStringByDuplication(keyText, keyText.length);
				key.vPtrValue = sc;
			}
			else 
			{
				switch (mapKeyType->VarType())
				{
				case SexyVarType_Int32:
					key.int32Value = atoi(keyText);
					break;
				case SexyVarType_Int64:
					key.int64Value = atoll(keyText);
					break;
				case SexyVarType_Float32:
					{
						uint32 binRepresentation;
						sscanf_s(keyText, "%x", &binRepresentation);
						key.floatValue = Rococo::Maths::IEEE475::BinaryToFloat(binRepresentation);
					}
					break;
				case SexyVarType_Float64:
					{
						uint64 binRepresentation;
						sscanf_s(keyText, "%llx", &binRepresentation);
						key.doubleValue = Rococo::Maths::IEEE475::BinaryToDouble(binRepresentation);
					}
					break;
				case SexyVarType_Bool:
					{
						boolean32 value = (*keyText == 'Y') ? 1 : 0;
						key.int32Value = value;
					}
					break;
				default:
					Rococo::Throw(0, "%s - Key type %s Not implemented", __ROCOCO_FUNCTION__, GetFriendlyName(*mapKeyType));
				}	
			}

			targetNode = InsertKey(*targetMap, key, reinterpret_cast<IScriptSystem&>(*scriptSystem));
			elementPtr = GetValuePointer(targetNode);
			SelectTarget(*mapValueType, elementPtr);
		}

		const IStructure* elementType = nullptr;

		void AddArrayDefinition(cstr refName, cstr elementTypeName, cstr elementTypeSource, int32 length, int32 capacity) override
		{
			container.elements.clear();

			elementType = &scriptSystem->GetTypeForSource(elementTypeName, elementTypeSource);

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
					Rococo::Throw(0, "Cannot load %s.\nThe array has already been defined as being type %s of %s.\nThe operation requested a type %s of %s.", refName, i->second->ElementType->Name(), i->second->ElementType->Module().Name(), type->Name(), type->Module().Name());
				}
			}

			auto* image = i->second;

			// Edge case -> capacity * image->ElementType->SizeOfStruct() may not fit in an int32

			int64 totalLen = (int64)capacity * (int64)image->ElementLength;
			if (totalLen > 0x7FFFFFFFLL)
			{
				Rococo::Throw(0, "Max capacity exceeded attempting to reserve %d elements for %s", capacity, refName);
			}

			void* elementBuffer = scriptSystem->AlignedMalloc(16, capacity * image->ElementLength);
			if (elementBuffer == nullptr)
			{
				Rococo::Throw(0, "Could not reserve %d elements for %s", capacity, refName);
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
				Rococo::Throw(0, "%s. Bad index", __ROCOCO_FUNCTION__);
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

		ListImage* currentList = nullptr;

		void AddListDefinition(cstr refName, cstr valueType, cstr valueTypeSource, int32 length) override
		{
			UNUSED(length);

			container.elements.clear();

			elementType = &scriptSystem->GetTypeForSource(valueType, valueTypeSource);

			auto i = lists.find(refName);
			if (i == lists.end())
			{
				auto* image = scriptSystem->CreateListImage(*elementType);
				i = lists.insert(refName, image).first;
			}
			else
			{
				if (i->second->ElementType != elementType)
				{
					Rococo::Throw(0, "Cannot load %s.\nThe list has already been defined as being type %s of %s.\nThe operation requested a type %s of %s.", refName, GetFriendlyName(*i->second->ElementType), i->second->ElementType->Module().Name(), GetFriendlyName(*type), type->Module().Name());
				}
			}

			auto* image = i->second;

			image->NumberOfElements = 0;

			currentList = image;

			SelectTarget(*elementType, nullptr);
		}

		void AppendNewListNode(int32 indexHint) override
		{
			UNUSED(indexHint);

			uint8* elementPtr = scriptSystem->AppendListNode(*currentList);
			SelectTarget(*currentList->ElementType, elementPtr);
		}
	};
}

namespace Rococo::Sex::Assets
{
	using namespace Rococo::Sex::Assets::Impl;

	void ParseSexyObjectTree(cstr treeAsCSVString, const IStructure& assetType, void* assetData, Rococo::Script::IPublicScriptSystem& ss)
	{
		SexyObjectBuilder objectBuilder;
		objectBuilder.SelectScriptSystem(ss);
		objectBuilder.SelectRootTarget(assetType, assetData);

		try
		{
			ParseTabbedCSV_AssetFile(treeAsCSVString, objectBuilder);
			objectBuilder.ResolveReferences();
		}
		catch (IException&)
		{
			objectBuilder.Nullify(assetType, assetData);
			objectBuilder.DeleteNewObjects();
			throw;
		}
	}

	void LoadAndParseSexyObjectTree(IInstallation& installation, cstr pingPath, const IStructure& assetType, void* assetData, Rococo::Script::IPublicScriptSystem& ss)
	{
		WideFilePath sysPath;
		installation.ConvertPingPathToSysPath(pingPath, sysPath);

		try
		{
			struct : Strings::IStringPopulator
			{
				const IStructure* assetType = nullptr;
				void* assetData = nullptr;
				Rococo::Script::IPublicScriptSystem* ss = nullptr;

				void Populate(cstr csvString) override
				{
					ParseSexyObjectTree(csvString, *assetType, assetData, *ss);
				}
			} cb;
			
			cb.assetType = &assetType;
			cb.assetData = assetData;
			cb.ss = &ss;

			Rococo::IO::LoadAsciiTextFile(cb, sysPath);
		}
		catch (IException& e)
		{
			Rococo::Throw(e.ErrorCode(), "Error loading asset %s:\n%s", pingPath, e.Message());
		}
	}
}

