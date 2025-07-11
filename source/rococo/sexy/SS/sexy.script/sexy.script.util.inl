/*
	Sexy Scripting Language - Copright(c)2013. Mark Anthony Taylor. All rights reserved.

	https://github.com/theabstraction/rococo

	Email: mark.anthony.taylor@gmail.com

	The Sexy copyright holder is Mark Anthony Taylor, the English author of 'Lazy Bloke Ghosts of Parliament', 'Lazy Bloke in the 35th Century', 'King of the Republic', 'Origin of the Species' 
	

	1. This software is open-source. It can be freely copied, distributed, compiled and the compilation executed.
	
	1.a Modification of the software is not allowed where such modifcations fail to include this exact comment header in each source file, or alter, reduce or extend the Sexy language.
	The purpose of this limitation is to prevent different incompatible versions of the Sexy language emerging. 

	1.b You are allowed to fix bugs and implement performance improvements providing you inform Sexy's copyright owner via email. Such changes may then be incorporated in
	later versions of Sexy without compromising Sexy's copyright license.
	
	2. You are not permitted to copyright derivative versions of the source code. You are free to compile the code into binary libraries and include the binaries in a commercial application. 

	3. THERE IS NO WARRANTY FOR THE PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE LAW. EXCEPT WHEN OTHERWISE STATED IN WRITING THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES PROVIDE THE PROGRAM �AS IS� WITHOUT
	WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE ENTIRE RISK AS TO THE QUALITY
	AND PERFORMANCE OF THE PROGRAM IS WITH YOU. SHOULD THE PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF ALL NECESSARY SERVICING, REPAIR OR CORRECTION.

	4. IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING WILL ANY COPYRIGHT HOLDER, OR ANY OTHER PARTY WHO MODIFIES AND/OR CONVEYS THE PROGRAM AS PERMITTED ABOVE, BE LIABLE TO YOU FOR
	DAMAGES, INCLUDING ANY GENERAL, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OR INABILITY TO USE THE PROGRAM (INCLUDING BUT NOT LIMITED TO LOSS OF DATA OR DATA BEING RENDERED
	INACCURATE OR LOSSES SUSTAINED BY YOU OR THIRD PARTIES OR A FAILURE OF THE PROGRAM TO OPERATE WITH ANY OTHER PROGRAMS), EVEN IF SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE POSSIBILITY OF
	SUCH DAMAGES.

	5. Any program that distributes this software in source or binary form must include an obvious credit to the the language, its copyright holder, and the language homepage. This must be the case in its
	principal credit screen and its principal readme file.
*/

namespace Rococo
{
	namespace Script
	{
		void AddVariableRef(CCompileEnvironment& ce, const NameString& ns, const IStructure& typeStruct)
		{
			void* hint = (void*)GetTryCatchExpression(ce.Script);
			ce.Builder.AddVariableRef(ns, typeStruct, hint);
		}

		void AssignTempToVariableRef(CCompileEnvironment& ce, int tempDepth, cstr name)
		{
			MemberDef def;
			ce.Builder.TryGetVariableByName(def, name);

			if (def.IsParentValue)
			{
				ce.Builder.Assembler().Append_SwapRegister(VM::REGISTER_D6, VM::REGISTER_SF);
				ce.Builder.Assembler().Append_SetStackFrameValue(def.SFOffset, VM::REGISTER_D4 + tempDepth, BITCOUNT_POINTER);
				ce.Builder.Assembler().Append_SwapRegister(VM::REGISTER_D6, VM::REGISTER_SF);
			}
			else
			{
				ce.Builder.Assembler().Append_SetStackFrameValue(def.SFOffset, VM::REGISTER_D4 + tempDepth, BITCOUNT_POINTER);
			}
		}

		void AddInterfaceVariable(CCompileEnvironment& ce, const NameString& ns, const IStructure& ts)
		{
			ce.Builder.AddInterfaceVariable(ns, ts, (void*)GetTryCatchExpression(ce.Script));
		}

		void AddVariable(CCompileEnvironment& ce, const NameString& ns, const IStructure& typeStruct)
		{
			ce.Builder.AddVariable(ns, typeStruct, (void*)GetTryCatchExpression(ce.Script));
		}

		void AddArgVariable(cstr desc, CCompileEnvironment& ce, const TypeString& type)
		{
			ce.Builder.AddArgVariable(desc, type, (void*)GetTryCatchExpression(ce.Script));
		}

		void AddArgVariable(cstr desc, CCompileEnvironment& ce, const IStructure& type)
		{
			ce.Builder.AddArgVariable(desc, type, (void*)GetTryCatchExpression(ce.Script));
		}

		bool IsAtomicMatch(cr_sex s, cstr value)
		{
			return IsAtomic(s) && AreEqual(s.String(), value);
		}

		bool RequiresDestruction(const IStructure& s)
		{
			if (IsContainerType(s.VarType())) { return true; }
			if (IsPrimitiveType(s.VarType())) return false;
			if (IsNullType(s)) return true;

			TokenBuffer destrName;
			SafeFormat(destrName.Text, TokenBuffer::MAX_TOKEN_CHARS, ("%s.Destruct"), s.Name());
			if (s.Module().FindFunction(destrName) != NULL) return true;

			if (AreEqual(s.Name(), ("_Lock"))) return true;
			if (AreEqual(s.Name(), ("_Node"))) return true;
			if (AreEqual(s.Name(), ("_MapNode"))) return true;

			for (int i = 0; i < s.MemberCount(); ++i)
			{
				const IMember& m = s.GetMember(i);
				const IStructure& mType = *m.UnderlyingType();
				if (RequiresDestruction(mType))
				{
					return true;
				}
			}

			return false;
		}

		void AppendInvoke(CCompileEnvironment& ce, ID_API_CALLBACK callback, cr_sex s)
		{
			ce.Builder.Assembler().Append_Invoke(callback);
			MarkStackRollback(ce, s);
		}

		void AddSymbol(ICodeBuilder& builder, cstr format, ...)
		{
			va_list args;
			va_start(args, format);

			char msg[128];
			SafeVFormat(msg, sizeof(msg), format, args);
			builder.AddSymbol(msg);
		}

		struct ArrayImage;
		struct ListImage;
		struct MapImage;

		void IncrementRef(MapImage* mapImage);

		void DestroyElements(ArrayImage& a, IScriptSystem& ss);
		void ListRelease(ListImage* l, IScriptSystem& ss);
		void MapClear(MapImage* m, IScriptSystem& ss);
		void ArrayDelete(ArrayImage* a, IScriptSystem& ss);

		void DestroyObject(const IStructure& type, uint8* item, IScriptSystem& ss)
		{
			if (IsPrimitiveType(type.VarType())) return;

			switch (type.VarType())
			{
			case SexyVarType_Array:
				{
					ArrayImage* a = *(ArrayImage**)item;
					DestroyElements(*a, ss);
					ArrayDelete(a, ss);
					return;
				}
			case SexyVarType_List:
				{
					ListImage* l = *(ListImage**)item;
					ListRelease(l, ss);
					return;
				}
			case SexyVarType_Map:
				{
					MapImage* m = *(MapImage**)item;
					MapClear(m, ss);
					return;
				}
			}
			
			if (type.InterfaceCount() > 0)
			{
				InterfacePointer pInterface = *(InterfacePointer*)item;
				ss.ProgramObject().DecrementRefCount(pInterface);
			}
			else
			{
				int offset = 0;
				for (int i = 0; i < type.MemberCount(); ++i)
				{
					const IMember& m = type.GetMember(i);
					const IStructure& mType = *m.UnderlyingType();
					DestroyObject(mType, item + offset, ss);
					offset += Rococo::Compiler::IsNullType(mType) ? sizeof(size_t) : mType.SizeOfStruct();
				}
			}
		}

		// All our datatypes are multiples of 4-bytes and are allocated from aligned blocks, so this should generally be faster than memcpy which is bloated with alignment cases
		void AlignedMemcpy(void* __restrict dest, const void* __restrict source, size_t nBytes)
		{
			// TODO -> Replace with SSE code
			size_t nWords = nBytes >> 2;

			const int* end = ((int*)source) + nWords;
			int* target = (int*)dest;

			const int* p = (const int*)source;
			while (p < end)
			{
				*target++ = *p++;
			}
		}

		bool IsGetAccessor(const IArchetype& callee)
		{
			if (callee.NumberOfOutputs() != 1) return false;

			if (callee.IsVirtualMethod())
			{
				return callee.NumberOfInputs() == 1;
			}
			else
			{
				return callee.NumberOfInputs() == 0;
			}
		}
	}//Script
}//Sexy