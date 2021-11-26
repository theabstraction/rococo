#include <rococo.api.h>
#include <rococo.csv.h>
#include <rococo.io.h>
#include <rococo.os.h>
#include <sexy.compiler.public.h>
#include <rococo.strings.h>

#include <vector>

namespace
{
	using namespace Rococo;
	using namespace Rococo::IO;
	using namespace Rococo::Compiler;
	using namespace Rococo::Sexy;

	inline ObjectStub* InterfaceToInstance(InterfacePointer i)
	{
		auto* p = ((uint8*)i) + (*i)->OffsetToInstance;
		auto* obj = (ObjectStub*)p;
		return obj;
	}

	struct SexyObjectBuilder: ISexyObjectBuilder, IMemberBuilder
	{
		const Rococo::Compiler::IStructure* type = nullptr;
		uint8* pObject = nullptr;
		ObjectStub* objectStub = nullptr;
		uint8* writePosition = nullptr;
		uint8* writeCursor = nullptr;
		const Rococo::Compiler::IStructure* objectType = nullptr;

		// Gives the next member to be overwritten
		std::vector<int> memberIndexStack;

		void Free() override
		{
			delete this;
		}

		void SelectTarget(const Rococo::Compiler::IStructure& type, void* pObject)
		{
			this->type = &type;
			this->pObject = (uint8*) pObject;

			if (type.InterfaceCount() > 0)
			{
				InterfacePointer ip = (InterfacePointer)pObject;
				objectStub = InterfaceToInstance(ip);
				objectType = objectStub->Desc->TypeInfo;
				writePosition = ((uint8*)objectStub) + sizeof(objectStub) + sizeof(VirtualTable*) * objectType->InterfaceCount();
			}
			else
			{
				objectStub = nullptr;
				objectType = &type;
				writePosition = ((uint8*) pObject);
			}

			memberIndexStack.clear();
			memberIndexStack.push_back(0); // 0 gives the member index, and since the stack depth is 1, the index refers to the top level member array
		}

		IMemberBuilder& MemberBuilder()
		{
			return *this;
		}

		int GetWriteOffset(const IStructure& s, int memberIndex)
		{
			int offset = 0;

			for (int i = 0; i < memberIndex; ++i)
			{
				auto& member = s.GetMember(i);
				offset += member.SizeOfMember();
			}

			return offset;
		}

		struct MemberAndOffset
		{
			const IMember* member;
			int offset;
		};

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

		const Rococo::Compiler::IMember* GetMemberRefAndUpdateWriteCursor()
		{
			int structureMemberIndex = memberIndexStack[0];

			const Rococo::Compiler::IMember* member = nullptr;

			if (structureMemberIndex < type->MemberCount())
			{
				member = &type->GetMember(structureMemberIndex);
			}

			if (!member)
			{
				return nullptr;
			}

			int offset = GetWriteOffset(*type, structureMemberIndex);

			const IStructure* parentType = member->UnderlyingType();

			for (int i = 1; i < memberIndexStack.size(); ++i)
			{
				MemberAndOffset childMemberAndOffset = GetMemberAndOffsetForStackPosition(*parentType, i);
				offset += childMemberAndOffset.offset;
				if (childMemberAndOffset.member == nullptr)
				{
					writeCursor = nullptr;
					return nullptr;
				}

				parentType = childMemberAndOffset.member->UnderlyingType();
				member = childMemberAndOffset.member;
			}

			if (member)
			{
				writeCursor = writePosition + (ptrdiff_t)offset;
			}
			else
			{
				writeCursor = nullptr;
			}

			return member;
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
			int offset = 0;

			if (memberIndexStack.size() == 1)
			{
				// We are editing the top level members
				int memberIndex = GetMemberIndex(name, *type);
				if (memberIndex < 0)
				{
					writeCursor = nullptr;
					return nullptr;
				}
				else
				{
					memberIndexStack[0] = memberIndex;
					auto result = GetMemberAndOffsetForStackPosition(*type, 0);
					writeCursor = writePosition + (ptrdiff_t)result.offset;
					return result.member;
				}
			}

			const IStructure* parentType = type;

			for (int i = 1; i < memberIndexStack.size()-1; ++i)
			{
				MemberAndOffset childMemberAndOffset = GetMemberAndOffsetForStackPosition(*parentType, i);
				if (childMemberAndOffset.member == nullptr)
				{
					return nullptr;
				}
				offset += childMemberAndOffset.offset;
				parentType = childMemberAndOffset.member->UnderlyingType();
			}

			MemberAndOffset memberAndOffset{ nullptr, 0 };

			int memberIndex = GetMemberIndex(name, *parentType);
			if (memberIndex < 0)
			{
				return nullptr;
			}
			else
			{
				memberIndexStack.back() = memberIndex;
				memberAndOffset = GetMemberAndOffsetForStackPosition(*type, (int) (memberIndexStack.size() - 1));
				offset += memberAndOffset.offset;
			}

			if (memberAndOffset.member)
			{
				writeCursor = writePosition + (ptrdiff_t)offset;
			}
			else
			{
				writeCursor = nullptr;
			}

			return memberAndOffset.member;
		}

		const Rococo::Compiler::IMember* GetBestMatchingMember(cstr name)
		{
			const Rococo::Compiler::IMember* member = GetMemberRefAndUpdateWriteCursor();
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
			int& lastIndex = memberIndexStack.back();
			lastIndex++;
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

			int& lastIndex = memberIndexStack.back();
			lastIndex++;
		}

		void AddBooleanMember(cstr name, bool value) override
		{
			const Rococo::Compiler::IMember* member = GetBestMatchingMember(name);
			
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
			const Rococo::Compiler::IMember* member = GetBestMatchingMember(name);

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
			const Rococo::Compiler::IMember* member = GetBestMatchingMember(name);

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
			const Rococo::Compiler::IMember* member = GetBestMatchingMember(name);

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
			const Rococo::Compiler::IMember* member = GetBestMatchingMember(name);

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

		void AddDerivativeMember(cstr type, cstr name, cstr sourceFile) override
		{
			const Rococo::Compiler::IMember* member = GetBestMatchingMember(name);
			
			if (!member)
			{
				Throw(0, "%s %s. No member found", type, name);
			}
			
			auto* memberType = member->UnderlyingType();

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
				memberIndexStack.push_back(0);
			}
		}

		void ReturnToParent() override
		{
			memberIndexStack.pop_back();
			if (memberIndexStack.empty())
			{
				Throw(0, "%s called too many times. Algorithmic error", __FUNCTION__);
			}
			memberIndexStack.back()++;
		}
	};

	struct SexyAssetLoader: IAssetLoader
	{
		IInstallation& installation;
		SexyObjectBuilder objectBuilder;
		AutoFree<ICSVTokenParser> sxyaParser = CreateSXYAParser(objectBuilder);
		AutoFree<ITabbedCSVTokenizer> tabbedTokenizer = CreateTabbedCSVTokenizer();
		
		SexyAssetLoader(IInstallation& _installation) : installation(_installation)
		{
		}

		void Free() override
		{
			delete this;
		}

		void LoadAndParse(cstr pingPath, const IStructure& assetType, void* assetData) override
		{
			WideFilePath sysPath;
			installation.ConvertPingPathToSysPath(pingPath, sysPath);

			objectBuilder.SelectTarget(assetType, assetData);

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

			Rococo::OS::LoadAsciiTextFile(parse, sysPath);
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