/*
	Sexy Scripting Language - Copright(c)2013. Mark Anthony Taylor. All rights reserved.

	http://www.sexiestengine.com

	Email: mark.anthony.taylor@gmail.com

	The Sexy copyright holder is Mark Anthony Taylor, the English author of 'Lazy Bloke Ghosts of Parliament', 'Lazy Bloke in the 35th Century', 'King of the Republic', 'Origin of the Species' 
	and 'Society of Demons.'

	1. This software is open-source. It can be freely copied, distributed, compiled and the compilation executed.
	
	1.a Modification of the software is not allowed where such modifcations fail to include this exact comment header in each source file, or alter, reduce or extend the Sexy language.
	The purpose of this limitation is to prevent different incompatible versions of the Sexy language emerging. 

	1.b You are allowed to fix bugs and implement performance improvements providing you inform Sexy's copyright owner via email. Such changes may then be incorporated in
	later versions of Sexy without compromising Sexy's copyright license.
	
	2. You are not permitted to copyright derivative versions of the source code. You are free to compile the code into binary libraries and include the binaries in a commercial application. 

	3. THERE IS NO WARRANTY FOR THE PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE LAW. EXCEPT WHEN OTHERWISE STATED IN WRITING THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES PROVIDE THE PROGRAM “AS IS” WITHOUT
	WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE ENTIRE RISK AS TO THE QUALITY
	AND PERFORMANCE OF THE PROGRAM IS WITH YOU. SHOULD THE PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF ALL NECESSARY SERVICING, REPAIR OR CORRECTION.

	4. IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING WILL ANY COPYRIGHT HOLDER, OR ANY OTHER PARTY WHO MODIFIES AND/OR CONVEYS THE PROGRAM AS PERMITTED ABOVE, BE LIABLE TO YOU FOR
	DAMAGES, INCLUDING ANY GENERAL, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OR INABILITY TO USE THE PROGRAM (INCLUDING BUT NOT LIMITED TO LOSS OF DATA OR DATA BEING RENDERED
	INACCURATE OR LOSSES SUSTAINED BY YOU OR THIRD PARTIES OR A FAILURE OF THE PROGRAM TO OPERATE WITH ANY OTHER PROGRAMS), EVEN IF SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE POSSIBILITY OF
	SUCH DAMAGES.

	5. Any program that distributes this software in source or binary form must include an obvious credit to the the language, its copyright holder, and the language homepage. This must be the case in its
	principal credit screen and its principal readme file.
*/

#include "sexy.script.stdafx.h"
#include "sexy.compiler.helpers.h"
#include "sexy.s-parser.h"
#include "sexy.vm.h"
#include "sexy.vm.cpu.h"

#include <stdarg.h>
#include <algorithm>
#include <unordered_map>
#include <sexy.stdstrings.h>

using namespace Rococo;
using namespace Rococo::Sex;
using namespace Rococo::Compiler;
using namespace Rococo::Script;

namespace Rococo 
{ 
	namespace Script
	{
		void AppendInvokeCallDestructor(CCompileEnvironment& ce, const IStructure& s, cstr name, int SFoffset);
		void AppendDeconstructAll(CCompileEnvironment& ce, cr_sex sequence);

		size_t GetOffsetTo(cstr memberName, const IStructure& s)
		{
			size_t offset = 0;
			for(int i = 0; i < s.MemberCount(); i++)
			{
				const IMember& member = s.GetMember(i);
				if (Rococo::AreEqual(member.Name(), ("_refCount")))
				{
					break;							
				}

				offset += member.SizeOfMember();
			}

			return offset;
		}

		int GetStackRequirement(const IStructure& str, cr_sex s)
		{
			VARTYPE outputType = str.VarType();
			BITCOUNT bits = GetBitCount(outputType);
			switch(bits)
			{
			case BITCOUNT_32:
				return 4;
				break;
			case BITCOUNT_64:
				return 8;
				break;
			default:
				Throw(s, ("Algorithmic error: unhandled bitcount in argument to function"));
				return 0;
			}
		}

		void PushVariableRef(cr_sex s, ICodeBuilder& builder, const MemberDef& def, cstr name, int interfaceIndex)
		{
			if (IsNullType(*def.ResolvedType) || (def.ResolvedType->VarType() == VARTYPE_Array && def.IsContained))
			{
				MemberDef refDef;
				builder.TryGetVariableByName(OUT refDef, name);
				builder.PushVariable(IN refDef);
			}
			else
			{
				builder.PushVariableRef(name, interfaceIndex);
			}
		}

		int GetCommonInterfaceIndex(const IStructure& object, const IStructure& argType)
		{
			if (&object == &argType) return -1; 

			if (argType.InterfaceCount() == 0) return -1; // -1 incompatable types

			const IInterface& argInterf = argType.GetInterface(0);

			for(int i = 0; i < object.InterfaceCount(); ++i)
			{
				const IInterface& objectInterf = object.GetInterface(i);

				for (auto* I = &objectInterf; I != nullptr; I = I->Base())
				{
					if (I == &argInterf)
					{
						return i;
					}
				}
			}

			return -1;
		}

		void AddMember(IStructureBuilder& s, cr_sex field)
		{
			AssertCompound(field);
			AssertNotTooFewElements(field, 2);

			cr_sex argTypeExpr = GetAtomicArg(field, 0);
			sexstring type = argTypeExpr.String();

			if (s.Prototype().IsClass)
			{
				if (AreEqual(type, ("implements")))
				{
					for (int i = 1; i < field.NumberOfElements(); ++i)
					{
						cr_sex nameExpr = field.GetElement(i);
						AssertQualifiedIdentifier(nameExpr);
						s.AddInterface(nameExpr.String()->Buffer);
					}

					return;
				}
				else if (AreEqual(type, ("defines")))
				{
					if (field.NumberOfElements() == 2 && IsAtomic(field[1]))
					{
						s.AddInterface(field[1].String()->Buffer);
					} 
					else if (field.NumberOfElements() == 4 && IsAtomic(field[1]) && IsAtomic(field[2]) && IsAtomic(field[3]) && AreEqual(field[2].String(), ("extends")))
					{
						s.AddInterface(field[1].String()->Buffer);
					}
					else
					{
						Throw(field, ("Expecting (defines <fully-qualified-interface-name>) or (defines <fully-qualified-interface-name> extends <fully-qualified-interface-name>)"));
					}
					return;
				}
			}

			if (AreEqual(type, ("array")))
			{
				cr_sex elementTypeExpr = GetAtomicArg(field, 1);
				sexstring elementType = elementTypeExpr.String();

				AssertQualifiedIdentifier(elementTypeExpr);

				for(int i = 2; i < field.NumberOfElements(); ++i)
				{
					cr_sex nameExpr = field.GetElement(i);
					AssertLocalIdentifier(nameExpr);			

					s.AddMember(NameString::From(nameExpr.String()), TypeString::From(("_Array")), elementType->Buffer);
				}	
			}
			else if (AreEqual(type, ("list")))
			{
				cr_sex elementTypeExpr = GetAtomicArg(field, 1);
				sexstring elementType = elementTypeExpr.String();

				AssertQualifiedIdentifier(elementTypeExpr);

				for(int i = 2; i < field.NumberOfElements(); ++i)
				{
					cr_sex nameExpr = field.GetElement(i);
					AssertLocalIdentifier(nameExpr);			

					s.AddMember(NameString::From(nameExpr.String()), TypeString::From(("_List")), elementType->Buffer);
				}	
			}
			else if (AreEqual(type, ("map")))
			{
				cr_sex keyTypeExpr = GetAtomicArg(field, 1);
				cr_sex valueTypeExpr = GetAtomicArg(field, 2);
				sexstring keyType = keyTypeExpr.String();
				sexstring valueType = valueTypeExpr.String();

				AssertQualifiedIdentifier(keyTypeExpr);
				AssertQualifiedIdentifier(valueTypeExpr);

				for(int i = 3; i < field.NumberOfElements(); ++i)
				{
					cr_sex nameExpr = field.GetElement(i);
					AssertLocalIdentifier(nameExpr);			

					s.AddMember(NameString::From(nameExpr.String()), TypeString::From(("_Map")), keyType->Buffer, valueType->Buffer);
				}	
			}
			else
			{
				AssertQualifiedIdentifier(argTypeExpr);

				for(int i = 1; i < field.NumberOfElements(); ++i)
				{
					cr_sex nameExpr = field.GetElement(i);
					AssertLocalIdentifier(nameExpr);			

					IInterfaceBuilder* interf = MatchInterface(argTypeExpr, s.Module());
					if (interf != NULL)
					{
						s.AddInterfaceMember(NameString::From(nameExpr.String()), TypeString::From(type));
					}
					else
					{
						s.AddMember(NameString::From(nameExpr.String()), TypeString::From(type));
					}
				}	
			}
		}

		cstr Comparative(int delta) { return delta > 0 ? ("more") : ("fewer"); } 

		void ValidateArchetypeMatchesArchetype(cr_sex s, const IArchetype& f, const IArchetype& archetype, cstr source)
		{
			int delta = archetype.NumberOfInputs() - f.NumberOfInputs();
			if (delta != 0)
			{
				sexstringstream<1024> streamer;
				streamer.sb << ("There are ") << Comparative(delta) << (" inputs in ") << source << archetype.Name() << (" than in that of ") << f.Name();
				Throw(s, streamer);
			}

			delta = archetype.NumberOfOutputs() - f.NumberOfOutputs();
			if (delta != 0)
			{
				sexstringstream<1024> streamer;
				streamer.sb << ("There are ") << Comparative(delta) << (" outputs in ") << source << archetype.Name() << (" than that of ") << f.Name();
				Throw(s, streamer);
			}

			int32 argCount = ArgCount(f);

			for(int32 i = 0; i < argCount; ++i)
			{
				const IStructure& st = archetype.GetArgument(i);
				const IStructure& stf = f.GetArgument(i);
				cstr argname = f.GetArgName(i);

				if (archetype.IsVirtualMethod())
				{
					if (i == 0 && st.VarType() == VARTYPE_Pointer && AreEqual(archetype.GetArgName(0), ("_typeInfo")))
					{
						if (stf.Prototype().IsClass && AreEqual(THIS_POINTER_TOKEN,argname))
						{
							continue;
						}
					}
					else if (st.VarType() == VARTYPE_Pointer && AreEqual(archetype.GetArgName(i), ("_vTable"), 7))
					{
						if (stf.Prototype().IsClass && AreEqual(THIS_POINTER_TOKEN,argname))
						{
							continue;
						}
					}
				}
			
				if (&stf != &st)
				{
					sexstringstream<1024> streamer;
					streamer.sb << source << archetype.Name() << (": Argument [") << i << "] (" << GetFriendlyName(st) << " " << argname << ("). Type did not match that of the implementation. Expected '") << GetFriendlyName(stf) << ("'");
					Throw(s, streamer);
				}

				const IStructure* interfGenericArg1 = archetype.GetGenericArg1(i);
				const IStructure* concreteGenericArg1 = f.GetGenericArg1(i);

				if (interfGenericArg1 != concreteGenericArg1)
				{
					sexstringstream<1024> streamer;

					// Not really expecting the generic args to be NULL, as we should already have bailed out above, but handle the case

					streamer.sb << ("Error validating concrete method against the interface's specification for (") << f.Name() << ("...). \n");
					if (archetype.GetGenericArg1(i) != NULL)
					{
						streamer.sb << ("Interface's method with generic argument type '") << GetFriendlyName(*interfGenericArg1) << ("' does not match ");
					}
					else
					{
						streamer.sb << ("Interface's method has no generic argument type and so does not match ");
					}
				
					if (f.GetGenericArg1(i) != NULL)
					{
						streamer.sb << ("concrete generic argument type '") << GetFriendlyName(*concreteGenericArg1) << ("'");
					}
					else
					{
						streamer.sb << ("concrete method with no generic argument type ");
					}

				
					Throw(f.Definition() != NULL ? *(const ISExpression*)(f.Definition()) : s, "%s", (cstr) streamer);
				}
			}
		}

		void CompileAssignmentDirectiveFromAtomic(CCompileEnvironment& ce, cr_sex directive, const IStructure& varStruct, bool explicitKeyword)
		{
			int offset = explicitKeyword ? 0 : -1; // explicit means the directive begins with the assignment keyword, else it begins with the target variable
			cr_sex targetExpr = GetAtomicArg(directive, 1 + offset);
			cstr targetVariable = targetExpr.String()->Buffer;
			cr_sex assignmentChar = GetAtomicArg(directive, 2 + offset);
			cr_sex sourceValue = directive.GetElement(3 + offset);
			VARTYPE targetType = varStruct.VarType();

			cstr sourceText = sourceValue.String()->Buffer;

			TokenBuffer symbol;
			StringPrint(symbol, ("%s=%s"), targetVariable, (cstr) sourceValue.String()->Buffer);
		
			if (targetType == VARTYPE_Closure)
			{
				IFunctionBuilder* f = ce.Builder.Module().FindFunction(sourceText);
				if (f != NULL)
				{
					ValidateArchetypeMatchesArchetype(directive, *f, *varStruct.Archetype(), ("archetype "));

					if (f->Builder().NeedsParentsSF() && ce.Builder.Owner().GetArgumentByName(targetVariable) != NULL)
					{
						Throw(sourceValue, ("Cannot return the closure from the function, as it accesses its parent's stack variables"));
					}

					CodeSection section;
					f->Code().GetCodeSection(OUT section);

					ce.Builder.AddSymbol(symbol);

					VariantValue v;
					v.byteCodeIdValue = section.Id;
					ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D4, v, BITCOUNT_64);

					TokenBuffer targetBytecodeId;
					StringPrint(targetBytecodeId, ("%s.bytecodeId"), targetVariable);
					ce.Builder.AddSymbol(targetBytecodeId);
					ce.Builder.AssignTempToVariable(0, targetBytecodeId);

					VariantValue parentSF;
					parentSF.charPtrValue = NULL;
					ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D4, parentSF, BITCOUNT_POINTER);

					TokenBuffer targetParentSF;
					StringPrint(targetParentSF, ("%s.parentSF"), targetVariable);
					ce.Builder.AddSymbol(targetParentSF);
					ce.Builder.AssignTempToVariable(0, targetParentSF);
					return;
				}
			}

			// Either a literal or an identifier
		
			VARTYPE varType = Parse::GetLiteralType(sourceText);
			if (IsPrimitiveType(varType))
			{
				try
				{
					ce.Builder.AddSymbol(symbol);
					ce.Builder.AssignLiteral(NameString::From(targetVariable), sourceText);
					return;
				}
				catch(IException& stcex)
				{
					Throw(directive, stcex.Message());
				}
			}
			else
			{
				VARTYPE sourceType = ce.Builder.GetVarType(sourceText);
				if (IsPrimitiveType(sourceType))
				{
					if (targetType == sourceType)
					{
						ce.Builder.AddSymbol(symbol);
						ce.Builder.AssignVariableToVariable(sourceText, targetVariable);
						return;
					}
					else
					{
						sexstringstream<1024> streamer;
						streamer.sb << ("The type of ") << targetVariable << (" does not match the type of ") << sourceText;
						Throw(directive, streamer);
					}
				}
				else if (sourceType == VARTYPE_Closure)
				{
					if (targetType == sourceType)
					{
						MemberDef sourceDef, targetDef;
						ce.Builder.TryGetVariableByName(sourceDef, sourceText);
						ce.Builder.TryGetVariableByName(targetDef, targetVariable);

						if (sourceDef.CapturesLocalVariables && !targetDef.CapturesLocalVariables)
						{
							sexstringstream<1024> streamer;
							streamer.sb << ("Could not copy ") << sourceText << (" to ") << targetVariable << (". The target variable accepts regular function references, but not closures.");
							Throw(directive, streamer);
						}

						ce.Builder.AddSymbol(symbol);
						ce.Builder.AssignVariableToVariable(sourceText, targetVariable);
						return;
					}
					else
					{
						sexstringstream<1024> streamer;
						streamer.sb << ("The type of ") << targetVariable << (" does not match the type of ") << sourceText;
						Throw(directive, streamer);
					}
				}
				else if (sourceType == VARTYPE_Derivative)
				{
					MemberDef def;
					if (!ce.Builder.TryGetVariableByName(def, sourceText))
					{
						Throw(directive, "Could not find variable %s", sourceText);
					}

					/* TODO - delete this comment
					if (def.ResolvedType != &varStruct)
					{
						Throw(directive, "Could not assign %s %s to %s %s", def.ResolvedType->Name(), sourceText, varStruct.Name(), targetVariable);
					} 
					*/

					if (Eq(sourceText, targetVariable))
					{
						Throw(directive, "Could assign variable to itself. Tautology is redundant");
					}

					ce.Builder.AssignVariableToVariable(sourceText, targetVariable, explicitKeyword);
				}
				else
				{
					if (varStruct.VarType() == VARTYPE_Derivative && !IsNullType(varStruct))
					{
						Throw(directive, ("Only interface references and primitive types can be initialized in an assignment"));
					}				

					if (AreEqual(sourceText, ("GetCurrentExpression")))
					{
						const CClassExpression* express =  ce.SS.GetExpressionReflection(directive);
						VariantValue sexPtr;
						sexPtr.vPtrValue = (void*) &express->Header.pVTables[0]; // sexPtr is the interface ptr, not the instance ptr

						MemberDef def;
						ce.Builder.TryGetVariableByName(OUT def, targetVariable);

						ce.Builder.Assembler().Append_SetStackFrameImmediate(def.SFOffset + def.MemberOffset, sexPtr, BITCOUNT_POINTER);
						return;
					}

					if (TryCompileFunctionCallAndReturnValue(ce, sourceValue, targetType, &varStruct, NULL))
					{
						TokenBuffer symbol;
						if (targetType == VARTYPE_Derivative)
						{
							// If a function returns a derivative type then it returns a pointer to a derivative type

							StringPrint(symbol, ("-> %s"), (cstr) targetVariable);
							ce.Builder.AddSymbol(symbol);
							ce.Builder.AssignTempToVariable(Rococo::ROOT_TEMPDEPTH, targetVariable);
						}
						else if (targetType == VARTYPE_Closure)
						{	
							StringPrint(symbol, ("-> %s"), (cstr)targetVariable);
							ce.Builder.AddSymbol(symbol);

							TokenBuffer bytecodeId;
							StringPrint(bytecodeId, ("%s.bytecodeId"), targetVariable);
							ce.Builder.AssignTempToVariable(Rococo::ROOT_TEMPDEPTH, bytecodeId);

							TokenBuffer parentSF;
							StringPrint(parentSF, ("%s.parentSF"), targetVariable);
							ce.Builder.AssignTempToVariable(10, parentSF);
						}
						else
						{						
							StringPrint(symbol, ("-> %s"), (cstr) targetVariable);
							ce.Builder.AddSymbol(symbol);
							ce.Builder.AssignTempToVariable(Rococo::ROOT_TEMPDEPTH, targetVariable);
						}
					}
					else
					{
						Throw(sourceValue, ("The RHS of the assignment was unrecognized"));
					}
				}
			}
		}

		void CompileAssignmentDirectiveFromCompound(CCompileEnvironment& ce, cr_sex directive, const IStructure& varStruct, bool explicitKeyword)
		{
			int offset = explicitKeyword ? 0 : -1; // explicit means the directive begins with the assignment keyword, else it begins with the target variable
			cstr targetVariable = GetAtomicArg(directive, 1 + offset).String()->Buffer;
			cr_sex sourceValue = directive.GetElement(3 + offset);
		
			VARTYPE targetType = varStruct.VarType();

			TokenBuffer symbol;
			StringPrint(symbol, ("%s=(...)"), targetVariable);

			bool negate = false;

			switch(targetType)
			{
			case VARTYPE_Bool:
				if (TryCompileBooleanExpression(ce, sourceValue, true, negate))
				{
					ce.Builder.AddSymbol(symbol);
					ce.Builder.AssignTempToVariable(Rococo::ROOT_TEMPDEPTH, targetVariable);
					if (negate) ce.Builder.Assembler().Append_BooleanNot(VM::REGISTER_D7);
					return;
				}
				break;
			case VARTYPE_Float32:
			case VARTYPE_Int32:
			case VARTYPE_Int64:
			case VARTYPE_Float64:
				if (TryCompileArithmeticExpression(ce, sourceValue, true, targetType))
				{
					ce.Builder.AddSymbol(symbol);
					ce.Builder.AssignTempToVariable(Rococo::ROOT_TEMPDEPTH, targetVariable);
					return;
				}
				break;
			case VARTYPE_Closure:
				if (TryCompileFunctionCallAndReturnValue(ce, sourceValue, targetType, NULL, varStruct.Archetype()))
				{
					TokenBuffer targetId;
					StringPrint(targetId, ("%s.bytecodeId"), targetVariable);

					ce.Builder.AddSymbol(symbol);
					ce.Builder.AssignTempToVariable(Rococo::ROOT_TEMPDEPTH, targetId); 

					TokenBuffer targetSF;
					StringPrint(targetSF, ("%s.parentSF"), targetVariable);

					ce.Builder.AddSymbol(symbol);
				
					// hotfix for 64-bit binaries - id goest to D7 while parentSF goes to D14, an unused register
					ce.Builder.AssignTempToVariable(Rococo::ROOT_TEMPDEPTH + 7, targetSF); 
					return;
				}
				else if (TryCompileClosureDef(ce, sourceValue, *varStruct.Archetype(), ce.Builder.Owner().GetArgumentByName(targetVariable) == NULL))
				{
					ce.Builder.EnableClosures(targetVariable);
					TokenBuffer symbol;
					StringPrint(symbol, ("%s.bytecodeId = (closure ...)"), targetVariable);
					ce.Builder.AddSymbol(symbol);

					TokenBuffer targetVariableByteCodeId;
					StringPrint(targetVariableByteCodeId, ("%s.bytecodeId"), targetVariable);
					ce.Builder.AssignTempToVariable(Rococo::ROOT_TEMPDEPTH, targetVariableByteCodeId);

					TokenBuffer targetVariableParentSF;
					StringPrint(symbol, ("%s.parentSF = SF"), targetVariable);
					ce.Builder.AddSymbol(symbol);
					StringPrint(targetVariableParentSF, ("%s.parentSF"), targetVariable);
					ce.Builder.AssignTempToVariable(-2, targetVariableParentSF);
					return;
				}
				break;
			case VARTYPE_Pointer:
				if (TryCompileFunctionCallAndReturnValue(ce, sourceValue, targetType, NULL, NULL))
				{				
					ce.Builder.AddSymbol(symbol);
					ce.Builder.AssignTempToVariable(Rococo::ROOT_TEMPDEPTH, targetVariable);
					return;
				}			
				break;
			case VARTYPE_Derivative: // Function returns a pointer to an interface in D7, the ref count was incremented by the callee
				if (TryCompileFunctionCallAndReturnValue(ce, sourceValue, targetType, &varStruct, NULL))
				{				
					ce.Builder.AddSymbol(symbol);
					static int returnArgIndex = 0;
					char argv[64];
					SafeFormat(argv, 64, "_retArg%d", returnArgIndex++);
					AddVariable(ce, NameString::From(argv), ce.Object.Common().TypePointer());
					ce.Builder.AssignTempToVariable(Rococo::ROOT_TEMPDEPTH, argv);

					// Decrement the reference of the target variable, as we are assigning a new object to it
					ce.Builder.AssignVariableToTemp(targetVariable, 0);
					ce.Builder.Append_DecRef();
					ce.Builder.AssignVariableToTemp(argv, 0);
					ce.Builder.AssignTempToVariable(0, targetVariable);
					return;
				}			
				break;
			default:
				break;
			}

			Throw(directive, ("Cannot determine RHS of assignment"));
		}

		void CompileAssignAtomicValueToMember(CCompileEnvironment& ce, const IMember& member, cr_sex src, cstr targetVariable)
		{
			// Either a literal or an identifier
			cstr sourceText = src.String()->Buffer;

			TokenBuffer symbol;
			StringPrint(symbol, ("%s=%s"), targetVariable, sourceText);

			VARTYPE varType = Parse::GetLiteralType(sourceText);
			if (IsPrimitiveType(varType))
			{
				try
				{
					ce.Builder.AddSymbol(symbol);
					ce.Builder.AssignLiteral(NameString::From(targetVariable), sourceText);
					return;
				}
				catch(IException& stcex)
				{
					Throw(src, stcex.Message());
				}
			}
			else
			{
				VARTYPE sourceType = ce.Builder.GetVarType(sourceText);
				if (IsPrimitiveType(sourceType))
				{
					if (member.UnderlyingType()->VarType() == sourceType)
					{
						AssignVariableToVariable(ce, src, targetVariable, sourceText);
						return;
					}
					else
					{
						sexstringstream<1024> streamer;
						streamer.sb << ("The type of ") << targetVariable << (" does not match the type of ") << sourceText;
						Throw(src, streamer);
					}
				}
				else
				{
					sexstringstream<1024> streamer;
					streamer.sb << ("Cannot assign to type of ") << targetVariable << (". The source is not a primitive type");
					Throw(src, streamer);
				}
			}
		}

		bool IsPublic(const IMember& member)
		{
			return member.Name()[0] != '_';
		}

		int PublicMemberCount(const IStructure& s)
		{
			int count = 0;
			for (int i = 0; i < s.MemberCount(); ++i)
			{
				const IMember& member = s.GetMember(i);
				if (IsPublic(member))
				{
					count++;
				}
			}

			return count;
		}

		void CompileAssignToStringMember(CCompileEnvironment& ce, cstr variableName, const IStructure& st, const IMember& member, cr_sex src)
		{
			// Assume member underlying type is null-string
			if (IsStringLiteral(src))
			{
				if (!IsIStringInlined(ce.Script))
				{
					Throw(src, ("Memberwise initialization of an IString member is not allowed unless all IString implementations support string inlining"));
				}

				sexstring valueStr = src.String();
				CStringConstant* sc = CreateStringConstant(ce.Script, valueStr->Length, valueStr->Buffer, &src);

				VariantValue ptrToConstant;
				ptrToConstant.vPtrValue = sc->header.pVTables;

				AddSymbol(ce.Builder, "'%s'", valueStr->Buffer);

				MemberDef def;
				ce.Builder.TryGetVariableByName(def, variableName);
				if (def.Usage == ARGUMENTUSAGE_BYVALUE)
				{
					UseStackFrameFor(ce.Builder, def);
					ce.Builder.Assembler().Append_SetStackFrameImmediate(def.SFOffset + def.MemberOffset, ptrToConstant, BITCOUNT_POINTER);
					RestoreStackFrameFor(ce.Builder, def);
				}
				else
				{
					ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D4, ptrToConstant, BITCOUNT_POINTER);
					ce.Builder.AssignTempToVariable(0, variableName);
				}
			}
			else if (IsAtomic(src))
			{
				ce.Builder.AssignVariableToVariable(src.String()->Buffer, variableName, true);
			}
			else
			{
				Throw(src, "Memberwise initialization of a string member requires the argument be an atomic string literal or variable");
			}
		}

		void CompileAssignMember(CCompileEnvironment& ce, cstr variableName, const IStructure& st, const IMember& member, cr_sex src, bool isConstructing)
		{
			const IStructure& memberType = *member.UnderlyingType();

			switch(memberType.VarType())
			{
			default:
				if (IsAtomic(src))
				{
					CompileAssignAtomicValueToMember(ce, member, src, variableName);
				}
				else
				{
					Throw(src, ("Do not know how to assign the value of the compound expression to the member variable"));
				}
				return;
			case VARTYPE_Derivative:
				{
					if (*member.UnderlyingType() == ce.Object.Common().SysTypeIString().NullObjectType())
					{
						CompileAssignToStringMember(ce, variableName, st, member, src);	
						return;
					}

					if (isConstructing && member.UnderlyingType()->InterfaceCount() > 0)
					{
						auto& interf0 = member.UnderlyingType()->GetInterface(0);
						auto* nullObj = interf0.UniversalNullInstance();
						auto* nullInterface = static_cast<InterfacePointer>(nullObj->pVTables);

						VariantValue nullInterfValue;
						nullInterfValue.vPtrValue = nullInterface;
						ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D4, nullInterfValue, BITCOUNT_POINTER);
						ce.Builder.AssignTempToVariable(0, variableName);
					}

					if (IsAtomic(src))
					{
						ce.Builder.AssignVariableToVariable(src.String()->Buffer, variableName, isConstructing);
						return;
					}
					else if (!IsCompound(src))
					{
						sexstringstream<1024> streamer;
						streamer.sb << memberType.Name() << (" is a derived type, and requires a compound initializer");
						Throw(src, streamer);
						return;
					}

					if (src.NumberOfElements() != PublicMemberCount(memberType))
					{
						sexstringstream<1024> streamer;
						streamer.sb << member.Name() << (" has ") << memberType.MemberCount() << (" elements. But ") << src.NumberOfElements() << (" were supplied ");
						Throw(src, streamer);
						return;
					}

					int publicMemberIndex = 0;

					for(int i = 0; i < memberType.MemberCount(); ++i)
					{
						const IMember& subMember = memberType.GetMember(i);
						if (IsPublic(subMember))
						{
							TokenBuffer memberName;
							StringPrint(memberName, ("%s.%s"), variableName, subMember.Name());
							CompileAssignMember(ce, memberName, memberType, subMember, src.GetElement(publicMemberIndex++), isConstructing);
						}
					}
					return;
				}
			case VARTYPE_Bad:
				{
					sexstringstream<1024> streamer;
					streamer.sb << memberType.Name() << (" is a bad type, and cannot be initialized");
					Throw(src, streamer);
					return;
				}
			}
		}

		void CompileAssignExpressionDirective(CCompileEnvironment& ce, cr_sex expression, cstr refName)
		{
			MemberDef def;
			ce.Builder.TryGetVariableByName(OUT def, refName);
		
			VariantValue v;
			v.vPtrValue = (void*) &GetSystem(ce.Script).GetExpressionReflection(expression)->Header.pVTables[0];

			TokenBuffer symbol;
			StringPrint(symbol, ("%s = '..."), refName);

			ce.Builder.AddSymbol(symbol);

			if (def.MemberOffset != 0 || def.Usage != ARGUMENTUSAGE_BYVALUE)
			{			
				ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D4, v, BITCOUNT_POINTER);
				ce.Builder.AssignTempToVariable(0, refName);
			}
			else
			{
				ce.Builder.Assembler().Append_SetStackFrameImmediate(def.SFOffset, v, BITCOUNT_POINTER);
			}
		}
	
		void CompileMemberwiseAssignment(CCompileEnvironment& ce, cr_sex directive, const IStructure& varStruct, int offset)
		{
			cstr targetVariable = GetAtomicArg(directive, 1 + offset).String()->Buffer;
			
			if (PublicMemberCount(varStruct) + 3 + offset != directive.NumberOfElements())
			{
				Throw(directive, "Expecting one element in the constructor for every member of the structure:\n Source: '%s - %s'", varStruct.Module().Name(), varStruct.Name());
			}

			int publicCount = 0;

			for(int i = 0; i < varStruct.MemberCount(); ++i)
			{
				const IMember& member = varStruct.GetMember(i);
				TokenBuffer memberName;
				StringPrint(memberName, ("%s.%s"), targetVariable, member.Name());

				if (IsPublic(member))
				{				
					CompileAssignMember(ce, memberName, varStruct, member, directive.GetElement(3 + offset + publicCount), offset == 0);
					publicCount++;
				}
			}
		}

	   void CompileUnaryOperatorOverload(CCompileEnvironment& ce, cr_sex directive, const IStructure& varStruct, int offset)
	   {
		  Throw(directive, ("Unary operator not implemented"));
	   }

	   const IFunction* FindFunction(cr_sex origin, cstr shortName, const IModule& module)
	   {
		  const IFunction* f = module.FindFunction(shortName);
		  if (f != nullptr) return f;

		  const IFunction* finalFunction = nullptr;
		  const INamespace* finalNS = nullptr;
		  for (int i = 0; i < module.PrefixCount(); ++i)
		  {
			 const INamespace& prefix = module.GetPrefix(i);
			 f = prefix.FindFunction(shortName);

			 if (f != nullptr)
			 {
				if (finalFunction != nullptr)
				   ThrowNamespaceConflict(origin, prefix, *finalNS, ("function"), shortName);
				else
				{
				   finalFunction = f;
				   finalNS = &prefix;
				}
			 }
		  }

		  return finalFunction;
	   }

	   cstr GetOperationPrefix(cstr op)
	   {
		  static std::unordered_map<stdstring, cstr> mapOperatorToFunctionPrex =
		  {
			 { ("+"), ("Add") },
			 { ("-"), ("Subtract") },
			 { ("*"), ("Multiply") },
			 { ("/"), ("Divide") }
		  };

		  auto i = mapOperatorToFunctionPrex.find(op);
		  return (i == mapOperatorToFunctionPrex.end()) ? nullptr : i->second;
	   }

	   int PushBinaryOperatorInputs(CCompileEnvironment& ce, cr_sex s, const IStructure& type, const IArchetype& callee, int firstArgIndex, const IStructure& atype, const IStructure& btype)
	   {
		  // (vec3 f = a + b) -> (AddVec3toVec3 a b f)
		  // 
		  int inputStackAllocCount = 0;

		  if (callee.NumberOfInputs() != 3)
		  {
			 Throw(s, ("Binary operator %s must have three arguments"), callee.Name());
		  }

		  if (callee.NumberOfOutputs() != 0)
		  {
			 Throw(s, ("Binary operator %s must not return output"), callee.Name());
		  }

		  if (&callee.GetArgument(0) != &atype)
		  {
			 Throw(s, ("First input argument was not of type %s. It was of type %s"), atype.Name(), callee.GetArgument(0).Name());
		  }

		  if (&callee.GetArgument(1) != &btype)
		  {
			 Throw(s, ("Second input argument was not of type %s. It was of type %s"), atype.Name(), callee.GetArgument(1).Name());
		  }
     
		  int indices[] { 2 + firstArgIndex, 4 + firstArgIndex, 0 + firstArgIndex };

		  for(int i = 0; i < 3; i++)
		  {
			 cstr inputName = callee.GetArgName(i);
			 const IStructure& argType = callee.GetArgument(i);
         
			 int inputStackCost = PushInput(ce, s, indices[i], argType, nullptr, inputName, callee.GetGenericArg1(i));
			 inputStackAllocCount += inputStackCost;
		  }
		  return inputStackAllocCount;
	   }

	   const IStructure* GetFirstMemberWithLiteralType(const IStructure& s)
	   {
		  if (IsNumericTypeOrBoolean(s.VarType()))
		  {
			 return &s;
		  }

		  if (s.InterfaceCount() != 0)
		  {
			 return nullptr;
		  }

		  // The structure was not a 

		  for (int32 i = 0; i < s.MemberCount(); ++i)
		  {
			 auto* memberType = s.GetMember(i).UnderlyingType();
			 if (memberType && memberType->Name()[0] != char('_'))
			 {
				auto* m = GetFirstMemberWithLiteralType(*memberType);
				if (m) return m;
			 }
		  }

		  return nullptr;
	   }

	   const IStructure& GetModuleDisposition(CCompileEnvironment& ce, IModule& module)
	   {
		  for (int i = 0; i < ce.Builder.Module().PrefixCount(); i++)
		  {
			 auto& prefix = ce.Builder.Module().GetPrefix(i);
			 if (AreEqual(prefix.Name(), ("F32")))
			 {
				return ce.Object.Common().TypeFloat32();
			 }
			 else if (AreEqual(prefix.Name(), ("F64")))
			 {
				return ce.Object.Common().TypeFloat64();
			 }
			 else if (AreEqual(prefix.Name(), ("I32")))
			 {
				return ce.Object.Common().TypeInt32();
			 }
			 else if (AreEqual(prefix.Name(), ("I64")))
			 {
				return ce.Object.Common().TypeInt64();
			 }
		  }

		  return ce.Object.Common().TypeFloat32();
	   }

	   const IStructure& GetFunctionDisposition(CCompileEnvironment& ce)
	   {
		  return GetModuleDisposition(ce, ce.Builder.Module());
	   }

	   const IStructure& GetLiteralDisposition(CCompileEnvironment& ce, const IStructure* hintStruct)
	   {
		  if (hintStruct)
		  {
			 // Motivation - if say we are dealing with Vec3f then literals could well be Float32s
			 const IStructure* s = GetFirstMemberWithLiteralType(*hintStruct);
			 if (s) return *s;
		  }

		  return GetFunctionDisposition(ce);
	   }

	   const IStructure& GetBestType(CCompileEnvironment& ce, cr_sex svalue, cstr value, const IStructure* hintStruct)
	   {
		  MemberDef def;
		  if (ce.Builder.TryGetVariableByName(def, value))
		  {
			 return *def.ResolvedType;
		  }
      
		  const IStructure& disposition = GetLiteralDisposition(ce, hintStruct);

		  VariantValue parsedValue;
		  if (Parse::PARSERESULT_GOOD == Parse::TryParse(parsedValue, disposition.VarType(), value))
		  {
			 return disposition;
		  }
		  else
		  {
			 if (Parse::PARSERESULT_GOOD == Parse::TryParse(parsedValue, VARTYPE_Float32, value))
			 {
				return ce.Object.Common().TypeFloat32();
			 }
		  }
		  Throw(svalue, ("Cannot infer best variable type for atomic expression"));
		  return *hintStruct;
	   }

	   // Infer a type for an expression
	   const IStructure& GetBestType(CCompileEnvironment& ce, cr_sex s, const IStructure* hintStruct)
	   {
		  switch (s.Type())
		  {
		  case EXPRESSION_TYPE_ATOMIC:
			 return GetBestType(ce, s, s.String()->Buffer, hintStruct);
		  case EXPRESSION_TYPE_COMPOUND:
			 Throw(s, ("Cannot infer variable type from compound expression"));
		  case EXPRESSION_TYPE_STRING_LITERAL:
			 return ce.Object.Common().SysTypeIString().NullObjectType();
		  default:
			 Throw(s, ("Cannot infer variable type from expression"));
			 return *hintStruct;
		  }
	   }

	   void CompileBinaryOperatorOverload(cstr operation, CCompileEnvironment& ce, cr_sex directive, const IStructure& varStruct, int offset)
	   {
		  // Assume varStruct is struct
		  // Convert (Vec3 c = a + b) into (Vec3 c)(AddVec3toVec3 a b c)

		  cr_sex sa = directive[2 + offset];
		  cr_sex sb = directive[4 + offset];

		  const IStructure& atype = GetBestType(ce, sa, &varStruct);
		  const IStructure& btype = GetBestType(ce, sb, &varStruct);

		  cstr instrumentName_a = atype.Name();
		  cstr instrumentName_b = btype.Name();

		  char functionName[Rococo::NAMESPACE_MAX_LENGTH];
		  SafeFormat(functionName, sizeof(functionName), ("%s%s%s"), operation, atype.Name(), btype.Name());

		  auto* overloadFn = FindFunction(directive, functionName, ce.Script.ProgramModule());
		  if (!overloadFn)
		  {
			 Throw(directive, ("Could not find binary operator function (%s (%s a)(%s b)(%s output) -> ).\nBe sure to use the correct (using ...) directive and include any necessary modules in your script."), functionName, instrumentName_a, instrumentName_b, varStruct.Name());
		  }

		  int inputStackAllocCount = PushBinaryOperatorInputs(ce, directive, varStruct, *overloadFn, offset, atype, btype);
		  AppendFunctionCallAssembly(ce, *overloadFn);
		  ce.Builder.MarkExpression(&directive);
		  RepairStack(ce, directive, *overloadFn);
		  int outputOffset = GetOutputSFOffset(ce, inputStackAllocCount, 0);
		  PopOutputs(ce, directive, *overloadFn, outputOffset, false);
		  ce.Builder.AssignClosureParentSFtoD6();
	   }

	   void CompileOperatorOverload(CCompileEnvironment& ce, cr_sex directive, const IStructure& varStruct, int offset)
	   {
		  if (directive.NumberOfElements() - offset == 4)
		  {
			 CompileUnaryOperatorOverload(ce, directive, varStruct, offset);
		  }
		  else if (directive.NumberOfElements() - offset == 5)
		  {
			 cr_sex sop = directive[3 + offset];
			 cstr op = sop.String()->Buffer;
			 if (IsAtomic(sop))
			 {
				cstr prefix = GetOperationPrefix(op);
				if (prefix == nullptr)
				{
				   Throw(0, ("Unknown operator. Expecting one of [+-*/] "));
				}
				CompileBinaryOperatorOverload(prefix, ce, directive, varStruct, offset);
			 }
			 else
			 {
				Throw(directive, ("Expecting binary operator, but did not find an atomic expression"));
			 }
		  }
		  else
		  {
			 Throw(directive, ("Expecting binary or unary operator, but there were too many arguments"));
		  }
	   }

	   void CompileAssignmentDirective(CCompileEnvironment& ce, cr_sex directive, const IStructure& varStruct, bool explicitKeyword)
	   {
		   int offset = explicitKeyword ? 0 : -1; // explicit means the directive begins with the assignment keyword, else it begins with the target variable
		   AssertNotTooFewElements(directive, 4 + offset);

		   if (varStruct.VarType() == VARTYPE_Derivative && !varStruct.Prototype().IsClass)
		   {
			   if (directive.NumberOfElements() == 4 + offset)
			   {
				   cr_sex secondarg = directive[3 + offset];
				   if (IsAtomic(secondarg))
				   {
					   cr_sex ssource = directive[1 + offset];
					   sexstring dsttext = ssource.String();
					   sexstring srctext = secondarg.String();

					   if (!IsCapital(srctext->Buffer[0]))
					   {
						   ce.Builder.AssignVariableToVariable(srctext->Buffer, dsttext->Buffer);
					   }
					   else
					   {
						   // Potentially a get accessor function
						   CompileAssignmentDirectiveFromAtomic(ce, directive, varStruct, explicitKeyword);
					   }
				   }
				   else
				   {
					  CompileMemberwiseAssignment(ce, directive, varStruct, offset);
				   }
			   }
			   else if (directive.NumberOfElements() == (6 + offset))
			   {
				   cr_sex secondarg = directive[4 + offset];
				   if (IsAtomic(secondarg) &&
					   (AreEqual(secondarg.String(), ("+")) ||
						   AreEqual(secondarg.String(), ("-")) ||
						   AreEqual(secondarg.String(), ("*"))))
				   {
					   CompileOperatorOverload(ce, directive, varStruct, offset + 1);
				   }
				   else
				   {
					   CompileMemberwiseAssignment(ce, directive, varStruct, offset);
				   }
			   }
			   else
			   {
				   CompileMemberwiseAssignment(ce, directive, varStruct, offset);
			   }
			   return;
		   }

		   AssertNotTooManyElements(directive, 4 + offset);
		   cr_sex sourceValue = directive.GetElement(3 + offset);

		   if (IsAtomic(sourceValue))
		   {
			   CompileAssignmentDirectiveFromAtomic(ce, directive, varStruct, explicitKeyword);
		   }
		   else if (IsCompound(sourceValue))
		   {
			   CompileAssignmentDirectiveFromCompound(ce, directive, varStruct, explicitKeyword);
		   }
		   else
		   {
			   sexstringstream<1024> streamer;
			   cstr targetVariable = GetAtomicArg(directive, 1 + offset).String()->Buffer;
			   streamer.sb << ("Bad expression on RHS of assignment: ") << targetVariable;
			   Throw(directive, streamer);
		   }
	   }

		void ValidateUnusedVariable(cr_sex identifierExpr, ICodeBuilder& builder)
		{
			cstr id = identifierExpr.String()->Buffer;
			if (builder.GetVarType(id) != VARTYPE_Bad)
			{
				sexstringstream<1024> streamer;
				streamer.sb << ("Variable name ") << id << (" is already defined in the context");
				Throw(identifierExpr, streamer);
			}
		}	

		int CompileThisToInstancePointerArg(CCompileEnvironment& ce, cr_sex s, cstr classInstance)
		{
			MemberDef refDef;
			ce.Builder.TryGetVariableByName(OUT refDef, classInstance);

			AddArgVariable(("instance"), ce, ce.Object.Common().TypePointer());

			if (refDef.Usage == ARGUMENTUSAGE_BYVALUE)
			{
				ce.Builder.Assembler().Append_PushStackFrameAddress(refDef.SFOffset);		
				return (int) sizeof(void*);			
			}
			else
			{			
				int offset = ce.Builder.GetThisOffset();
				if (offset == 0)
				{
					ce.Builder.Assembler().Append_PushStackVariable(refDef.SFOffset, BITCOUNT_POINTER);
				}
				else
				{
					ce.Builder.Assembler().Append_GetStackFrameMemberPtr(VM::REGISTER_D4, refDef.SFOffset, ce.Builder.GetThisOffset());
					ce.Builder.Assembler().Append_PushRegister(VM::REGISTER_D4, BITCOUNT_POINTER);
				}
			
				return (int) sizeof(void*);			
			}				
		}

		IFunction& GetConstructor(IModuleBuilder& module, cr_sex typeExpr)
		{
			sexstring type = typeExpr.String();

			TokenBuffer qualifiedConstructorName;
			StringPrint(qualifiedConstructorName, ("%s.%s"), type->Buffer, ("Construct"));
			IFunction* constructor = module.FindFunction(qualifiedConstructorName);
			if (constructor == NULL)
			{
				sexstringstream<1024> streamer;
				streamer.sb << ("Cannot find constructor in source module: ") << qualifiedConstructorName;
				Throw(typeExpr, streamer);
			}
			return *constructor;
		}

		void AddVariableAndSymbol(CCompileEnvironment& ce, cstr type, cstr name)
		{
			char declText[256];
			SafeFormat(declText, 256, ("%s %s"), type, name);
			ce.Builder.AddSymbol(declText);

			AddVariable(ce, NameString::From(name), TypeString::From(type));
		}

		void InitClassMember(CCompileEnvironment& ce, int sfMemberOffset, const IStructure& memberType)
		{
			VariantValue classDesc;
			classDesc.vPtrValue = (void*) memberType.GetVirtualTable(0);
			ce.Builder.Assembler().Append_SetStackFrameImmediate(sfMemberOffset, classDesc, BITCOUNT_POINTER);

			VariantValue allocSize;
			allocSize.int32Value = memberType.SizeOfStruct();
			ce.Builder.Assembler().Append_SetStackFrameImmediate(sfMemberOffset + sizeof(size_t), allocSize, BITCOUNT_32);

			int interfaceOffset = ObjectStub::BYTECOUNT_INSTANCE_TO_INTERFACE0;
			for(int i = 0; i < memberType.InterfaceCount(); ++i)
			{
				VariantValue interfVTable;
				interfVTable.vPtrValue = (void*) memberType.GetVirtualTable(1 + i);
				ce.Builder.Assembler().Append_SetStackFrameImmediate(sfMemberOffset + interfaceOffset, interfVTable, BITCOUNT_POINTER);	
				interfaceOffset += sizeof(void*);
			}

			// Null out IString's buffer and length members
			auto* iString = &ce.Object.Common().SysTypeIString();
			auto* iInterface0 = &memberType.GetInterface(0);
			for (auto* i = iInterface0; i != nullptr; i = i->Base())
			{
				if (i == iString)
				{
					cstr blank = "";
					VariantValue nullPtr;
					nullPtr.charPtrValue = const_cast<char*>(blank);
					ce.Builder.Assembler().Append_SetStackFrameImmediate(sfMemberOffset + ObjectStub::BYTECOUNT_INSTANCE_TO_INTERFACE0 + sizeof(size_t) + sizeof(int32), nullPtr, BITCOUNT_POINTER);

					VariantValue zeroInt32;
					zeroInt32.int32Value = 0;
					ce.Builder.Assembler().Append_SetStackFrameImmediate(sfMemberOffset + ObjectStub::BYTECOUNT_INSTANCE_TO_INTERFACE0 + sizeof(size_t), zeroInt32, BITCOUNT_32);
					break;
				}
			}
		}

		void InitSubmembers(CCompileEnvironment& ce, const IStructure& s, int sfMemberOffset);

		void ProtectedInitSubmembers(CCompileEnvironment& ce, const IStructure& s, int sfMemberOffset)
		{
			for(int i = 0; i < s.MemberCount(); ++i)
			{
				const IMember& member = s.GetMember(i);
				const IStructure* memberType = member.UnderlyingType();
				if (memberType)
				{
					if (memberType->Prototype().IsClass)
					{
						if (IsNullType(*memberType))
						{
							VariantValue v;
							auto& instance = *memberType->GetInterface(0).UniversalNullInstance();
							v.vPtrValue = &instance.pVTables;
							ce.Builder.Assembler().Append_SetStackFrameImmediate(sfMemberOffset, v, BITCOUNT_POINTER);
						}
						else
						{
							InitClassMember(ce, sfMemberOffset, *memberType);
							InitSubmembers(ce, *memberType, sfMemberOffset);
						}
					}
					else if (memberType->VarType() == VARTYPE_Array)
					{
						VariantValue nullRef;
						nullRef.vPtrValue = nullptr;
						ce.Builder.Assembler().Append_SetStackFrameImmediate(sfMemberOffset, nullRef, BITCOUNT_POINTER);
					}
					else
					{
						InitSubmembers(ce, *memberType, sfMemberOffset);
					}
				}
				else // Primitve type
				{
				}

				sfMemberOffset += member.SizeOfMember();
			}
		}

		void InitSubmembers(CCompileEnvironment& ce, const IStructure& s, int sfMemberOffset)
		{
			try
			{
				ProtectedInitSubmembers(ce, s, sfMemberOffset);
			}
			catch (IException& ex)
			{
				Throw(ex.ErrorCode(), "%s\nException compiling init submembers of %s", ex.Message(), s.Name());
			}
		}

		bool HasClassMembers(const IStructure& s)
		{
			if (IsPrimitiveType(s.VarType())) return false;

			for(int i = 0; i < s.MemberCount(); ++i)
			{
				const IMember& member = s.GetMember(i);
				const IStructure* memberType = member.UnderlyingType();

				if (memberType->VarType() == VARTYPE_Array)
				{
					return true;
				}
			
				if (memberType->Prototype().IsClass)
				{
					return true;
				}

				if (HasClassMembers(*memberType))
				{
					return true;
				}
			}

			return false;
		}

		void InitDefaultReferences(cr_sex s, CCompileEnvironment& ce, cstr id, const IStructure& st)
		{
			if (st.VarType() == VARTYPE_Closure)
			{
				MemberDef def;
				if (!ce.Builder.TryGetVariableByName(OUT def, id) || def.Usage != ARGUMENTUSAGE_BYVALUE)
				{
					Throw(s, ("Cannot compile closure. Unhandled syntax"));
				}

				IFunctionBuilder& nullFunction = GetNullFunction(ce.Script, *st.Archetype());

				CodeSection section;
				nullFunction.Builder().GetCodeSection(section);

				TokenBuffer idcode, idoffset;
				StringPrint(idcode, ("%s.bytecodeId"), id);
				StringPrint(idoffset, ("%s.parentSF"), id);

				VariantValue codeRef;
				codeRef.byteCodeIdValue = section.Id;
				ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D4, codeRef, BITCOUNT_64);
				ce.Builder.AssignTempToVariable(0, idcode);

				VariantValue zeroRef;
				zeroRef.charPtrValue = 0;
				ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D4, zeroRef, BITCOUNT_64);
				ce.Builder.AssignTempToVariable(0, idoffset);

				AddSymbol(ce.Builder, ("%s -> (Null-Function Null-SF)"), id);
			}
		}

		void InitClassMembers(CCompileEnvironment& ce, cstr id)
		{
			MemberDef def;
			ce.Builder.TryGetVariableByName(OUT def, id);

			if (def.Usage == ARGUMENTUSAGE_BYVALUE)
			{
				InitSubmembers(ce, *def.ResolvedType, def.SFOffset + def.MemberOffset);
			}
			else if (HasClassMembers(*def.ResolvedType))
			{
				ce.Builder.Assembler().Append_GetStackFrameValue(def.SFOffset, VM::REGISTER_D5, BITCOUNT_POINTER);
				ce.Builder.Assembler().Append_SwapRegister(VM::REGISTER_D5, VM::REGISTER_SF);

				InitSubmembers(ce, *def.ResolvedType, def.MemberOffset);

				ce.Builder.Assembler().Append_SwapRegister(VM::REGISTER_D5, VM::REGISTER_SF);
			}
		}

		void CompileConstructFromConcreteConstructor(CCompileEnvironment& ce, const IStructure& type, cstr id, cr_sex args)
		{
			// (Class instance1 (<arg1>...<argN>)))
			const IFunction* constructor = type.Constructor();
			if (constructor == NULL)
			{
				sexstringstream<1024> streamer;
				streamer.sb << ("Needs a constructor function ") << type.Name() << (".Construct inside ") << type.Module().Name(); 
				Throw(*args.Parent(), streamer);
			}

			AddSymbol(ce.Builder, ("%s %s (...)"), type.Name(), id); 
			AddVariable(ce, NameString::From(id), type);
	
			int inputCount = constructor->NumberOfInputs();

			int mapIndex = GetIndexOf(1, args, ("->"));
			if (mapIndex > 0) Throw(args, ("Mapping token are not allowed in constructor calls, which have no output"));
			if (args.NumberOfElements() < inputCount - 1) Throw(args, ("Too few arguments to constructor call"));
			if (args.NumberOfElements() > inputCount - 1) Throw(args, ("Too many arguments to constructor call"));

			int inputStackAllocCount = PushInputs(ce, args, *constructor, true, 0);		
			inputStackAllocCount += CompileInstancePointerArg(ce, id);

			AppendFunctionCallAssembly(ce, *constructor); 

			ce.Builder.MarkExpression(args.Parent());

			RepairStack(ce, *args.Parent(), *constructor);
			ce.Builder.AssignClosureParentSFtoD6();
		}

		void CompileContructorCall(CCompileEnvironment& ce, const IStructure& type, cstr id, cr_sex constructorArgs)
		{
			if (IsAtomic(constructorArgs) || IsStringLiteral(constructorArgs))
			{
				Throw(*constructorArgs.Parent(), ("Unrecognized variable initialization syntax. Expecting null or compound constructor args"));
			}

			if (type.VarType() != VARTYPE_Derivative)
			{
				Throw(*constructorArgs.Parent(), ("Constructor declarations are only available for derivative types"));
			}

			if (IsNullType(type))
			{
				// Expecting factory invocation of form (<interface> <id> (<factory> <factarg1> ... <factargN>))
				CompileConstructFromFactory(ce, type, id, constructorArgs);
				return;
			}

			// Expecting concrete invocation of form (<class> <id> (<arg1> ... <argN>))
			CompileConstructFromConcreteConstructor(ce, type, id, constructorArgs);
		}

		IModule& GetSysTypeMemoModule(CScript& script)
		{
			return GetModule(script).Object().GetModule(0);
		}

		void CompileCreateStringConstant(CCompileEnvironment& ce, cstr id, cr_sex decl, cr_sex value)
		{
			sexstring valueStr = value.String();

			CStringConstant* sc = CreateStringConstant(ce.Script, valueStr->Length, valueStr->Buffer, &value);

			AddSymbol(ce.Builder, ("StringConstant %s"), id);
			AddInterfaceVariable(ce, NameString::From(id), ce.Object.Common().SysTypeIString().NullObjectType());
				
			MemberDef ptrDef;
			ce.Builder.TryGetVariableByName(OUT ptrDef, id);
				
			cstr format = (value.String()->Length > 24) ? (" = '%.24s...'") : (" = '%s'");
			AddSymbol(ce.Builder, format, (cstr) value.String()->Buffer);
		
			VariantValue ptr;
			ptr.vPtrValue = (void*) &sc->header.pVTables[0];
			ce.Builder.Assembler().Append_SetStackFrameImmediate(ptrDef.SFOffset, ptr, BITCOUNT_POINTER);		
		}

		bool TryCompileAsStringConstantAssignment(CCompileEnvironment& ce, const IStructure& type, cstr id, cr_sex decl)
		{
			if (ce.Object.Common().SysTypeIString().NullObjectType() != type)  return false;
			if (decl.NumberOfElements() != 4) return false;	
		
			cr_sex value = decl.GetElement(3);
			if (value.Type() != EXPRESSION_TYPE_STRING_LITERAL) return false;		
		
			CompileCreateStringConstant(ce, id, decl, value);

			return true;
		}

		bool IsDeclarationAssignment(cr_sex s)
		{
			if (s.NumberOfElements() == 4)
			{
				cr_sex assignCharExpr = s.GetElement(2);
				if (AreEqual(assignCharExpr.String(), ("=")))
				{
					return true;
				}
			}

			return false;
		}

		void CompileAsExpressionArg(CCompileEnvironment& ce, const IStructure& type, cstr id, cr_sex decl)
		{
			if (type.InterfaceCount() == 0 || &type.GetInterface(0) != &ce.Object.Common().SysTypeIExpression())
			{
				Throw(decl, "Expecting tyoe Sys.Reflection.IExpression, the only known variable declarations with\r\n"
							"5 elements are expressions defintions of the format (Sys.Type.IExpression s = ' (<s_def>))"); 
			}

			cr_sex expression = decl.GetElement(4);

			AddInterfaceVariable(ce, NameString::From(id), type);
			CompileAssignExpressionDirective(ce, expression, id);
		}

		void CompileAsNodeValueAssign(CCompileEnvironment& ce, const IStructure& type, cstr id, cr_sex decl)
		{
			// (type id = & <node-name>)
			cr_sex nodeNameExpr = decl.GetElement(4);
			if (!IsAtomic(nodeNameExpr)) Throw(nodeNameExpr, ("Expecting (type id = & <node-name>). The <node-name> element needs to be atomic"));

			cstr nodeName = nodeNameExpr.String()->Buffer;
			ce.Builder.AssignVariableRefToTemp(nodeName, Rococo::ROOT_TEMPDEPTH); // The node pointer is now in D7

			const IStructure* nodeType = ce.Builder.GetVarStructure(nodeName);
			if (*nodeType == ce.Object.Common().TypeNode())
			{
				const IStructure& elementType = GetNodeDef(ce, decl, nodeName);
				if (elementType != type)
				{
					ThrowTypeMismatch(decl, elementType, type, ("The node element type did not match the declaration type"));
				}

				if (elementType.InterfaceCount() > 0)
				{
					Throw(decl, "Interface lists do not support the '&' directive. Use node.Value instead");
				}
			
				AppendInvoke(ce, GetListCallbacks(ce).NodeGetElementRef, decl); // The element ref is now in D7
			}
			else if (*nodeType == ce.Object.Common().TypeMapNode())
			{
				const MapNodeDef& def = GetMapNodeDef(ce, decl, nodeName);
				if (def.mapdef.ValueType != type)
				{
					ThrowTypeMismatch(decl, def.mapdef.ValueType, type, ("The node element type did not match the declaration type"));
				}

				if (def.mapdef.ValueType.InterfaceCount() > 0)
				{
					if (sizeof(size_t) == 8)
					{
						AppendInvoke(ce, GetMapCallbacks(ce).MapNodeGet64, decl); // The element is now in D7
					}
					else
					{
						AppendInvoke(ce, GetMapCallbacks(ce).MapNodeGet32, decl); // The element is now in D7
					}

					ce.Builder.Assembler().Append_MoveRegister(VM::REGISTER_D7, VM::REGISTER_D4, BITCOUNT_POINTER);
					ce.Builder.Append_IncRef();
				}
				else
				{
					AppendInvoke(ce, GetMapCallbacks(ce).MapNodeGetRef, decl); // The element ref is now in D7
				}
			}
			else
			{
				Throw(decl, ("The name did not match a known node type"));
			}
		
			AddVariableRef(ce, NameString::From(id), type);
			AssignTempToVariableRef(ce, Rococo::ROOT_TEMPDEPTH, id); // The id variable is now pointing to the element
		}

		void AssertDefaultConstruction(CCompileEnvironment& ce, cr_sex decl, const IStructure& s)
		{
			if (IsPrimitiveType(s.VarType())) return;

			if (s == ce.StructList())
			{
				Throw(decl, ("Objects of the given type cannot be default constructed as one of the members is a linked list.\r\nProvide an explicit constructor and declare instances using the syntax: ( <type> <name> ( <arg1>...<argN> ) )"));
			}
			else if (s == ce.StructMap())
			{
				Throw(decl, ("Objects of the given type cannot be default constructed as one of the members is a map.\r\nProvide an explicit constructor and declare instances using the syntax: ( <type> <name> ( <arg1>...<argN> ) )"));
			}

			for(int i = 0; i < s.MemberCount(); ++i)
			{
				const IMember& m = s.GetMember(i);
				AssertDefaultConstruction(ce, decl, *m.UnderlyingType());
			}
		}

		void* GetInterfacePtrFromNullInstancePtr(void* instancePtr);

		void CompileClassAsDefaultVariableDeclaration(CCompileEnvironment& ce, const IStructure& st, cstr id, cr_sex decl, bool initializeValues)
		{
			if (st.Name()[0] != '_')
			{
				Throw(decl, "Concrete classes cannot be created on the stack. Use a factory");
			}

			if (st.InterfaceCount() == 0)
			{
				Throw(decl, "Expecting at least one interface for the class object");
			}

			AddSymbol(ce.Builder, ("%s %s"), GetFriendlyName(st), id);
			AssertDefaultConstruction(ce, decl, st);
			AddInterfaceVariable(ce, NameString::From(id), st);

			VariantValue value;
			value.vPtrValue = GetInterfacePtrFromNullInstancePtr(st.GetInterface(0).UniversalNullInstance());

			ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D4, value, BITCOUNT_POINTER);
			ce.Builder.AssignTempToVariable(0, id);
		}

		void CompileAsDefaultVariableDeclaration(CCompileEnvironment& ce, const IStructure& type, cstr id, cr_sex sDef, bool initializeValues)
		{	
			if (type.Prototype().IsClass)
			{
				 CompileClassAsDefaultVariableDeclaration(ce, type, id, sDef, initializeValues);
			}
			else
			{
				char constructorName[256];
				SafeFormat(constructorName, "%s.Construct", type.Name());
				auto* constructor = type.Module().FindFunction(constructorName);

				AddSymbol(ce.Builder, "%s %s", GetFriendlyName(type), id);
				AssertDefaultConstruction(ce, sDef, type);
				AddVariable(ce, NameString::From(id), type);
				
				if (constructor)
				{
					if (constructor->NumberOfInputs() > 1)
					{
						// Something in addition to the implicit 'this' pointer

						char buf[1024];
						StackStringBuilder ssb(buf, sizeof buf);
						ssb.AppendFormat("Constructor missing arguments, so default construction not permitted.\t");
						ssb.AppendFormat("Format is: \n\t(%s ", constructorName);

						for (int i = 0; i < constructor->NumberOfInputs() - 1; ++i)
						{
							auto& arg = constructor->GetArgument(i);
							cstr argName = constructor->GetArgName(i);
							ssb.AppendFormat("(%s %s) ", GetFriendlyName(arg), argName);
						}
						Throw(sDef, "%s)\n", (cstr)*ssb);
					}

					// Invoke constructor
					int inputStackAllocCount = PushInputs(ce, sDef, *constructor, true, 0);
					inputStackAllocCount += CompileInstancePointerArg(ce, id);

					AppendFunctionCallAssembly(ce, *constructor);

					ce.Builder.MarkExpression(sDef.Parent());

					RepairStack(ce, *sDef.Parent(), *constructor);
					ce.Builder.AssignClosureParentSFtoD6();
				}	
				else
				{
					if (initializeValues)
					{
						InitClassMembers(ce, id);
						InitDefaultReferences(sDef, ce, id, type);
					}
				}
			}
		}

		void InitInterfaceToToNullObject(CCompileEnvironment& ce, const NameString& name, const IStructure& type)
		{
			ce.Builder.AssignPointer(name, type.GetInterface(0).UniversalNullInstance());
		}
		
		bool CanThrow(CCompileEnvironment& ce, cr_sex s)
		{
			if (IsCompound(s))
			{
				return true;
			}
			else if (IsAtomic(s))
			{
				auto name = s.String();
				MemberDef def;
				if (!ce.Builder.TryGetVariableByName(def, name->Buffer))
				{
					return true;
				}
			}

			return false;
		}

		void CompileAsVariableDeclarationWithAssignment(CCompileEnvironment& ce, const IStructure& type, cstr id, cr_sex decl)
		{
			// (<type> <id> = <...>)


			if (type.Prototype().IsClass)
			{	
				if (TryCompileAsStringConstantAssignment(ce, type, id, decl))
				{
					return;
				}

				if (IsDeclarationAssignment(decl))
				{
					AddInterfaceVariable(ce, NameString::From(id), type);

					cr_sex rhs = decl[2];

					if (CanThrow(ce, rhs))
					{
						// If the rhs can throw an exception then the lhs must be properly initialized before the rhs is called
						// otherwise when the ref count is decremented we will get a memory exception error
						InitInterfaceToToNullObject(ce, NameString::From(id), type);
					}

					CompileAssignmentDirective(ce, decl, type, true);
					return;
				}
			}		
		
			CompileAsDefaultVariableDeclaration(ce, type, id, decl, false);
			CompileAssignmentDirective(ce, decl, type, true);
		}

		bool IsStructOrArchetypeOrInterface(cr_sex typeExpr, IModuleBuilder& module)
		{
			IStructure* s = MatchStructure(typeExpr, module);
			if (s == NULL)
			{
				// Failed to find a structure with the given name, so look for a closure
				IArchetype* a = MatchArchetype(typeExpr, module);
				if (a == NULL)
				{
					return false;
				}
				else
				{
					s = module.Object().IntrinsicModule().FindStructure(("__Closure"));
					if (s == NULL)
					{
						Throw(typeExpr, ("Error could not find __Closure intrinsic"));
					}
				}
			}

			return true;
		}

		void CompileAsVariableDeclarationAndInit(CCompileEnvironment& ce, const IStructure& type, cstr id, cr_sex decl)
		{		
			if (decl.NumberOfElements() == 3)
			{
				cr_sex constructorArgs = decl.GetElement(2);			
				CompileContructorCall(ce, type, id, constructorArgs);
			}
			else
			{
				// (type id = (constructor_args)) used in member by member construction
				AssertAtomicMatch(decl.GetElement(2), ("="));
							
				if (decl.NumberOfElements() == 5)
				{			
					if (IsAtomicMatch(decl.GetElement(3), ("'")))
					{
						CompileAsExpressionArg(ce, type, id, decl);
						return;
					}
					else if (IsAtomicMatch(decl.GetElement(3), ("&")))
					{
						CompileAsNodeValueAssign(ce, type, id, decl);
						return;
					}
				}
					
				// (type id = value)
				CompileAsVariableDeclarationWithAssignment(ce, type, id, decl);	
			}
		}

		void ValidateLocalDeclarationVariable(const IStructure& st, cr_sex idExpr)
		{
			if (!IsAtomic(idExpr))
			{
				Throw(idExpr, ("Expecting a local identifier variable name in this position"));
			}

			AssertLocalIdentifier(idExpr);
		}

		void CompileAsVariableDeclaration(CCompileEnvironment& ce, cr_sex decl)
		{
			AssertCompound(decl);

			cr_sex typeExpr = decl.GetElement(0);
			cr_sex idExpr = decl.GetElement(1);

			IModuleBuilder& source = ce.Builder.Module();
			int nElements = decl.NumberOfElements();

			const IStructure* st = MatchStructure(typeExpr, source);
			if (st == NULL)
			{
				const IInterface* interf = MatchInterface(typeExpr, source);
				if (interf != NULL)
				{
					st = &interf->NullObjectType();
				}
			}
			else
			{
				if (st->InterfaceCount() > 0 && st->Name()[0] != '_')
				{
					Throw(typeExpr, "Cannot allocate classes on the stack. Use a factory instantiate a class");
				}
			}

			if (st == NULL)
			{
				ThrowTokenNotFound(decl, typeExpr.String()->Buffer, source.Name(), ("type"));
			}

			ValidateLocalDeclarationVariable(*st, idExpr);
			cstr id = idExpr.String()->Buffer;
			if (nElements == 2)
			{
				CompileAsDefaultVariableDeclaration(ce, *st, id, decl, true);
			}
			else
			{
				CompileAsVariableDeclarationAndInit(ce, *st, id, decl);
			}
		}

		bool IsAssignment(cr_sex s)
		{
			if (s.NumberOfElements() < 3)	
			{
				return false;
			}

			if (!IsAtomic(s[0]))
			{
				return false;
			}

			cr_sex operatorExpr = s[1];
			if (!IsAtomic(operatorExpr))
			{
				return false;
			}

			cstr op = operatorExpr.String()->Buffer;
			if (!AreEqual(op, ("=")))
			{
				return false;
			}

			return true;
		}

		void AssertGetVariable(OUT MemberDef& def, cstr name, CCompileEnvironment& ce, cr_sex exceptionSource)
		{
			if (!ce.Builder.TryGetVariableByName(def, name))
			{
				NamespaceSplitter splitter(name);

				cstr instance, member;
				if (splitter.SplitTail(instance, member))
				{
					if (ce.Builder.TryGetVariableByName(def, instance))
					{
						ThrowTokenNotFound(exceptionSource, member, instance, ("member"));
					}
				}
			
				ThrowTokenNotFound(exceptionSource, name, ce.Builder.Owner().Name(), ("variable"));
			}
		}

		// Returns true iff structure is Plain Old Data.
		bool IsPLOD(const IStructure& type)
		{
			VARTYPE vt = type.VarType();
			if (vt == VARTYPE_Pointer)
			{
				// Pointers represent native data
				return false;
			}

			if (IsPrimitiveType(vt))
				return true;

			if (type.Name()[0] == '_')
			{
				// System types are not PLOD
				return false;
			}

			TokenBuffer destructorName;
			StringPrint(destructorName, ("%s.Destruct"), type.Name());
		
			if (type.Module().FindFunction(destructorName) != NULL)
			{
				// Destructors are generally used to tidy up native resources, hence indicate we are not PLODs
				return false;
			}

			for(int i = 0; i < type.MemberCount(); ++i)
			{
				const IMember& m = type.GetMember(i);
				if (!IsPLOD(*m.UnderlyingType()))
				{
					return false;
				}			
			}

			return true;
		}

		bool CompileAsStructureAssignmentFromCompound(CCompileEnvironment& ce, const IStructure& varStruct, cstr varName, cr_sex value)
		{
			if (value.NumberOfElements() == 1)
			{
				return CompileAsStructureAssignmentFromCompound(ce, varStruct, varName, value.GetElement(0));
			}

			if (value.NumberOfElements() == 2)
			{
				cr_sex command = GetAtomicArg(value, 0);
				cstr commandText = command.String()->Buffer;

				cr_sex arg = value.GetElement(1);

				if (islower(commandText[0]))
				{
					MemberDef def;
					AssertGetVariable(OUT def, commandText, ce, command);

					if (*def.ResolvedType == ce.StructArray())
					{					
						const IStructure& elementType = GetElementTypeForArrayVariable(ce, value, commandText);
						const IStructure* varType = ce.Builder.GetVarStructure(varName);

						if (varType == NULL)
						{
							Throw(*value.Parent(), ("Cannot determine structure type on LHS of assignment"));
						}

						if (*varType != elementType)
						{
							ThrowTypeMismatch(*value.Parent(), *varType, elementType, ("Array type does not match assignment target type"));
						}

						if (!IsPLOD(elementType))
						{
							Throw(value, "The array element type is not plain data. One or more of its members was reference counted. Use syntax (foreach <local-var> # (<array> <index> (...logic....))");
						}

						if (!TryCompileArithmeticExpression(ce, arg, true, VARTYPE_Int32))
						{
							Throw(command, ("Expecting Int32 valued expression for array index"));
						} // D7 now contains the array index

						ce.Builder.AssignVariableRefToTemp(commandText, 0); // D4 contains the array ptr
						ce.Builder.AssignVariableRefToTemp(varName, 1); // D5 contains the structure ptr

						ce.Builder.Assembler().Append_Invoke(GetArrayCallbacks(ce).ArrayGetByRef);
						return true;
					}
					else
					{
						Throw(command, ("Expecting array name"));
					}
				}
				else
				{
					Throw(command, ("Expecting variable name"));
				}
			}

			return false;
		}

		bool CompileAsStructureAssignment(CCompileEnvironment& ce, const IStructure& varStruct, cr_sex directive)
		{
		  if (directive.NumberOfElements() == 3)
		  {
			 cstr varName = directive.GetElement(0).String()->Buffer;
			 cr_sex rhs = directive.GetElement(2);

			 switch (rhs.Type())
			 {
			 case EXPRESSION_TYPE_COMPOUND:
				return CompileAsStructureAssignmentFromCompound(ce, varStruct, varName, rhs);
			 default:
				Throw(directive, ("Non-compound structure assignment is not yet implemented"));
				return false;
			 }
		  }
		  else if (directive.NumberOfElements() == 5)
		  {
			 auto& sop = directive[3];

			 if (IsAtomic(sop))
			 {
				cstr prefix = nullptr;
				auto s = sop.String();
				if (AreEqual(s, ("+"))) prefix = ("Add");
				else  if (AreEqual(s, ("-"))) prefix = ("Subtract");
				else  if (AreEqual(s, ("*"))) prefix = ("Multiply");
				else return false;
				CompileBinaryOperatorOverload(prefix, ce, directive, varStruct, 0);
				return true;
			 }
			 else
			 {
				return false;
			 }
		  }
		  else
		  {
			 return false;
		  }
		}

		bool TryCompileAsImplicitSetDirective(CCompileEnvironment& ce, cr_sex directive)
		{
			if (!IsAssignment(directive))
			{
				return false;
			}

			cr_sex keywordExpr = GetAtomicArg(directive, 0);
			cstr token = keywordExpr.String()->Buffer;

			MemberDef def;
			if (ce.Builder.TryGetVariableByName(OUT def, token))
			{
				const IStructure* varStruct = def.ResolvedType;
				VARTYPE tokenType = varStruct->VarType();

				if (IsPrimitiveType(tokenType) || tokenType == VARTYPE_Closure)
				{
					CompileAssignmentDirective(ce, directive, *varStruct, false);
					return true;
				}
				else if (tokenType == VARTYPE_Derivative)
				{
					if (varStruct->Prototype().IsClass)
					{
						if (!TryCompileAsLateFactoryCall(ce, def, directive))
						{
							CompileAssignmentDirective(ce, directive, *varStruct, false);
						}

						return true;
					}
					else
					{
						if (CompileAsStructureAssignment(ce, *varStruct, directive))
						{
							return true;
						}
						else
						{
							CompileAssignmentDirective(ce, directive, *varStruct, false);
							return true;
						}
					}
				}
			}

			return false;
		}

		const ISExpression* GetTryCatchExpression(CScript& script);

		void NoteDestructorPositions(CCompileEnvironment& ce, const IStructure& structureToDelete, int baseSFoffset)
		{
			if (RequiresDestruction(structureToDelete))
			{
				ce.Builder.NoteDestructorPosition(baseSFoffset, structureToDelete);
			}
		}
	
		void MarkStackRollback(CCompileEnvironment& ce, cr_sex invokeExpression)
		{
			const ISExpression* tryCatchBlock = GetTryCatchExpression(ce.Script);

			int stackCorrection = 0;

			int nTempVariables = ce.Builder.GetVariableCount() - ArgCount(ce.Builder.Owner());

			for(int i = 0; i < nTempVariables; i++)
			{
				size_t lastIndex = ce.Builder.GetVariableCount() - i - 1;
				MemberDef def;
				cstr name;
				ce.Builder.GetVariableByIndex(OUT def, OUT name, (int32) lastIndex);

				if (tryCatchBlock != NULL && def.Userdata != tryCatchBlock)
				{
					// We mark the rollback position to the tryCatchBlock
					break;
				}

				stackCorrection += def.AllocSize;			
			}

			ce.Builder.NoteStackCorrection(stackCorrection);

			for(int i = 0; i < nTempVariables; i++)
			{
				size_t lastIndex = ce.Builder.GetVariableCount() - i - 1;
				MemberDef def;
				cstr name;
				ce.Builder.GetVariableByIndex(OUT def, OUT name, (int32) lastIndex);

				if (tryCatchBlock != NULL && def.Userdata != tryCatchBlock)
				{
					break;
				}

				if (def.Usage == ARGUMENTUSAGE_BYVALUE && def.location != VARLOCATION_NONE)
				{
					NoteDestructorPositions(ce, *def.ResolvedType, def.SFOffset);
				}
				else if (def.Usage == ARGUMENTUSAGE_BYREFERENCE && (*def.ResolvedType == ce.Object.Common().TypeNode() || *def.ResolvedType == ce.Object.Common().TypeMapNode()))
				{
					NoteDestructorPositions(ce, *def.ResolvedType, def.SFOffset);
				}
			}
		}

		void CompileConstructInterfaceCall(CCompileEnvironment& ce, const IFunction& constructor, const IInterface& targetInterface, const IStructure& classType, cr_sex s)
		{
			// (construct class-name arg1 arg2 ... argN)
			// D4 contained InterfacePointer*, the effective output variable
			// First allocate space for the concrete class and copy address to the instance ref
			ce.Builder.AddDynamicAllocateObject(classType, targetInterface);

			AddVariable(ce, NameString::From("_instance"), ce.Object.Common().TypePointer());

			AssignTempToVariableRef(ce, 0, "_instance");

			MemberDef instanceDef;
			ce.Builder.TryGetVariableByName(instanceDef, "_instance");

			int inputCount = constructor.NumberOfInputs() - 1;
			int nRequiredElements = 2 + inputCount;

			int mapIndex = GetIndexOf(1, s, ("->"));
			if (mapIndex > 0) Throw(s, ("Mapping token are not allowed in constructor calls, which have no output"));
			if (s.NumberOfElements() < nRequiredElements) Throw(s, ("Too few arguments to constructor call"));
			if (s.NumberOfElements() > nRequiredElements) Throw(s, ("Too many arguments to constructor call"));
				
			int inputStackAllocCount = PushInputs(ce, s, constructor, true, 2);	
			inputStackAllocCount += sizeof(size_t);

			AddArgVariable("instance", ce, ce.Object.Common().TypePointer());

			ce.Builder.Assembler().Append_PushStackVariable(instanceDef.SFOffset, BITCOUNT_POINTER);

			AppendFunctionCallAssembly(ce, constructor);
			RepairStack(ce, s, constructor);
	
			ce.Builder.AssignClosureParentSFtoD6();
		}

		int GetIndexOfInterface(const IStructure& concreteClass, const IInterface& interf);

		void CompileConstructInterfaceCall(CCompileEnvironment& ce, cr_sex s)
		{
			if (ce.factory == nullptr)
			{
				Throw(s, "Keyword 'construct' is only permitted inside a factory function");
			}

			// (construct class-name arg1 arg2 ... argN)
			AssertNotTooFewElements(s, 2);

			cr_sex classNameExpr = GetAtomicArg(s, 1);
			sexstring className = classNameExpr.String();
		
			const IStructure& classType = GetClass(classNameExpr, ce.Script);	
			const IInterface& thisInterf = ce.factory->ThisInterface();
			const IFunction& constructor = GetConstructor(classType, classNameExpr);

			int index = Rococo::Script::GetIndexOfInterface(classType, thisInterf);

			if (index >= 0)
			{
				CompileConstructInterfaceCall(REF ce, IN constructor, IN thisInterf, IN classType, IN s);
			}
			else
			{
				Throw(s, "The class does not implement the interface associated with the local variable");
			}
		}

		void CompileInterfaceCast(CCompileEnvironment& ce, IStructure& toType, cr_sex toNameExpr, cr_sex fromNameExpr)
		{
			sexstring toName = toNameExpr.String();
			sexstring fromName = fromNameExpr.String();
			AddVariable(ce, NameString::From(toName->Buffer), toType);

			IFunction& fnDynCast = GetFunctionByFQN(ce, *toNameExpr.Parent(), ("Sys.Native._DynamicCast"));

			const IInterface& castToInterf = toType.GetInterface(0);
			cstr castToInterfName = castToInterf.Name();

			VariantValue v;
			v.vPtrValue = (void*) &castToInterf;
			ce.Builder.AddSymbol(castToInterfName);
			ce.Builder.Assembler().Append_PushLiteral(BITCOUNT_POINTER, v);

			MemberDef refDef;
			if (!ce.Builder.TryGetVariableByName(refDef, fromName->Buffer))
			{
				Throw(fromNameExpr, "Cannot interpret argument as a variable. Only variables can be cast. Check spelling.");
			}

			ce.Builder.PushVariable(refDef);

			ce.Builder.AddSymbol(("_DynamicCast to D4"));

			AddArgVariable(("cast_to_interface"), ce, ce.Object.Common().TypePointer());
			AddArgVariable(("cast_from_ref"), ce, ce.Object.Common().TypePointer());

			AppendFunctionCallAssembly(ce, fnDynCast);
			MarkStackRollback(ce, *toNameExpr.Parent());
			ce.Builder.AddSymbol(toName->Buffer);

			MemberDef nameDef;
			ce.Builder.TryGetVariableByName(nameDef, toName->Buffer);
			ce.Builder.Assembler().Append_SetStackFrameValue(nameDef.SFOffset, VM::REGISTER_D4, BITCOUNT_POINTER);

			ce.Builder.PopLastVariables(2, true);
		}

		void CompileCast(CCompileEnvironment& ce, cr_sex s)
		{
			// (cast <from_variable> -> <to_type> <to_variable> )
			AssertNotTooFewElements(s, 5);
			AssertNotTooManyElements(s, 5);

			cr_sex toTypeExpr = GetAtomicArg(s, 3);
			cr_sex toName = GetAtomicArg(s, 4);
			cr_sex fromName = GetAtomicArg(s, 1);
			cr_sex mapToken = GetAtomicArg(s, 2);

			if (!AreEqual(mapToken.String(), ("->")))
			{
				Throw(s, ("Expecting syntax: (cast <from_variable> -> <to_type> <to_variable> )"));
			}

			AssertQualifiedIdentifier(toTypeExpr);
			AssertLocalIdentifier(toName);
			IStructure* toType = MatchStructure(toTypeExpr, ce.Builder.Module());
			if (toType == NULL)
			{
				Throw(toTypeExpr, ("Unknown target type for cast"));
			}

			if (IsNullType(*toType))
			{
				CompileInterfaceCast(ce, *toType, toName, fromName);
			}
			else
			{
				Throw(toTypeExpr, ("Only interfaces can be the target type of a cast"));
			}
		}

		bool IsExpressionInterface(const IStructure& s)
		{
			if (s.InterfaceCount() > 0)
			{
				auto& i = s.GetInterface(0);
				for (const auto* pInterf = &i; pInterf != nullptr; pInterf = pInterf->Base())
				{
					// N.B make sure modules match as a security measure
					if (AreEqual("IExpression", pInterf->Name()) && &pInterf->NullObjectType().Module() == &s.Object().GetModule(3))
					{
						return true;
					}
				}
			}

			return false;
		}

		void CompileReflect(CCompileEnvironment& ce, cr_sex s)
		{
			// (reflect <function-id> <lhs-variable> <rhs-variable>)
			AssertNotTooFewElements(s, 4);
			AssertNotTooManyElements(s, 4);

			cr_sex sFunctionId = s[1];
			cr_sex sLHSVariable = s[2];
			cr_sex sRHSVariable = s[3];

			AssertAtomic(sFunctionId);
			AssertAtomic(sLHSVariable);
			AssertAtomic(sRHSVariable);

			auto functionId = sFunctionId.String();
			auto lhsVariableName = sLHSVariable.String();
			auto rhsVariableName = sRHSVariable.String();

			MemberDef lhsVariableDef;
			if (!ce.Builder.TryGetVariableByName(lhsVariableDef, lhsVariableName->Buffer))
			{
				Throw(sLHSVariable, "(serialize <function-id> <lhs-variable-name> <rhs-variable-name>): Could not identify the LHS variable");
			}

			MemberDef rhsVariableDef;
			if (!ce.Builder.TryGetVariableByName(rhsVariableDef, rhsVariableName->Buffer))
			{
				Throw(sRHSVariable, "(serialize <function-id> <lhs-variable-name> <rhs-variable-name>): Could not identify the RHS variable");
			}

			ID_API_CALLBACK id = ce.SS.TryGetRawReflectionCallbackId(functionId->Buffer);
			if (!id)
			{
				Throw(sFunctionId, "(serialize <function-id> <lhs-variable-name> <rhs-variable-name>): Could not match function-id to any known reflection function");
			}

			VariantValue v;
			v.vPtrValue = (void*)lhsVariableDef.ResolvedType;
			ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D4, v, BITCOUNT_POINTER); // D4 gets the lhs type

			ce.Builder.AssignVariableRefToTemp(lhsVariableName->Buffer, 1); // D5 gets the lhs reference

			VariantValue name;
			name.vPtrValue = (void*)lhsVariableName->Buffer;
			ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D6, name, BITCOUNT_POINTER); // D6 gets the rhs name

			v.vPtrValue = (void*)rhsVariableDef.ResolvedType;
			ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D7, v, BITCOUNT_POINTER); // D7 gets the rhs type

			ce.Builder.AssignVariableRefToTemp(rhsVariableName->Buffer, 4); // D8 gets the rhs reference


			ce.Builder.Assembler().Append_Invoke(id);
		}

		void CompileSerializeFromInterface(cr_sex s, CCompileEnvironment& ce, const MemberDef& srcDef, const MemberDef& trgDef)
		{
			if (trgDef.ResolvedType->VarType() != VARTYPE_Derivative)
			{
				Throw(s, "(serialize <src> -> <targets>): The target was not a class or struct");
			}

			cstr srcName = s[1].String()->Buffer;
			ce.Builder.AssignVariableToTemp(srcName, 1); // D5

			cstr trgName = s[3].String()->Buffer;
			ce.Builder.AssignVariableRefToTemp(trgName, 3); // D7

			VariantValue v;
			v.vPtrValue = (void*)trgDef.ResolvedType;
			ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D4, v, BITCOUNT_POINTER);

			ce.Builder.Assembler().Append_Invoke(ce.SS.GetIdSerializeCallback());
		}

		void CompileSerialize(CCompileEnvironment& ce, cr_sex s)
		{
			// (serialize <source> -> <target>)
			AssertNotTooFewElements(s, 4);
			AssertNotTooManyElements(s, 4);

			cr_sex ssrc = s[1];
			cr_sex sassign = s[2];
			cr_sex starget = s[3];

			AssertAtomic(ssrc);
			AssertAtomic(sassign);
			AssertAtomic(starget);

			auto src = ssrc.String();
			auto assign = sassign.String();
			auto target = starget.String();

			if (!AreEqual(assign, "->")) Throw(sassign, "(serialize <src> -> <target>): Expecting -> at this position.");
			
			MemberDef srcDef;
			if (!ce.Builder.TryGetVariableByName(srcDef, src->Buffer))
			{
				Throw(ssrc, "(serialize <src> -> <target>): Could not identify the source variable");
			}

			MemberDef trgDef;
			if (!ce.Builder.TryGetVariableByName(trgDef, target->Buffer))
			{
				Throw(ssrc, "(serialize <src> -> <target>): Could not identify the target variable");
			}

			if (IsExpressionInterface(*srcDef.ResolvedType))
			{
				CompileSerializeFromInterface(s, ce, srcDef, trgDef);
			}
			else
			{
				Throw(ssrc, "(serialize <src> -> <target>): Neither the source nor the target variable was an IExpression");
			}
		}

		void CompileTrip(CCompileEnvironment& ce, cr_sex s)
		{
			ce.Builder.Assembler().Append_TripDebugger();
		}

		void CompileReturnFromFunction(CCompileEnvironment& ce, cr_sex s)
		{
			if (s.NumberOfElements() > 1)
			{
				Throw(s, "Return statements take no arguments. Output variables should be set with assignment semantics.");
			}
			AppendDeconstructAll(ce, s);
			ce.Builder.Assembler().Append_Return();
		}

		void CompleAsNodeDeclaration(CCompileEnvironment& ce, cr_sex s)
		{
			// (node n = <source>)
			AssertNotTooFewElements(s, 4);
			AssertNotTooManyElements(s, 4);

			AssertAtomicMatch(s.GetElement(2), ("="));

			cr_sex nodeNameExpr = GetAtomicArg(s, 1);
			cstr nodeName = nodeNameExpr.String()->Buffer;
			AssertLocalIdentifier(nodeNameExpr);

			cr_sex source = s.GetElement(3);
			if (IsAtomic(source))
			{
				CompileAsListNodeDeclaration(ce, nodeName, source);
			}
			else if (IsCompound(source))
			{
				CompileAsMapNodeDeclaration(ce, nodeName, source);
			}
			else
			{
				Throw(s, ("Expecting atomic or compound expression as the final item in the node definition"));
			}
		}

		void CompileAsYield(CCompileEnvironment& ce, cr_sex s)
		{
			// (yield)

			if (s.NumberOfElements() == 1)
			{
				AssertNotTooManyElements(s, 2);
				ce.Builder.Assembler().Append_Yield();
			}
			else if (s.NumberOfElements() == 2)
			{
				AssertAtomic(s[1]);

				cstr arg = s[1].String()->Buffer;

				VariantValue waitPeriod;
				if (Parse::PARSERESULT_GOOD == Parse::TryParse(waitPeriod, VARTYPE_Int64, arg))
				{
					if (waitPeriod.int64Value > 0)
					{
						AddSymbol(ce.Builder, "yield %lld us", waitPeriod.int64Value);
						ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D4, waitPeriod, BITCOUNT_64);
					}
					else
					{
						Throw(s, "yield: negative yield period!");
					}
				}
				else
				{
					MemberDef def;
					if (!ce.Builder.TryGetVariableByName(def, arg))
					{
						Throw(s, "yield argument was neither a literal int64 nor a known variable");
					}

					if (def.ResolvedType->VarType() != VARTYPE_Int64)
					{
						Throw(s, "yield argument was neither a literal int64 nor an int64 variable");
					}

					ce.Builder.AssignVariableToTemp(arg, 0);
				}

				VariantValue nullVal;
				nullVal.int64Value = 0;
				ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D5, nullVal, BITCOUNT_64);

				ce.Builder.Assembler().Append_MoveRegister(VM::REGISTER_PC, VM::REGISTER_D7, BITCOUNT_POINTER);
				ce.Builder.Assembler().Append_Invoke(ce.SS.GetScriptCallbacks().idYieldMicroseconds);
			}
			else
			{
				AssertNotTooManyElements(s, 2);
			}
		}

		void CompileAsGlobalAccess(CCompileEnvironment& ce, cr_sex s)
		{
			// Either (global <varname> -> <target_variable>)
			// or 
			// (global <varname> <- <source_variable>)

		  if (s.NumberOfElements() != 4)
		  {
			 Throw(s, ("Expecting (global <varname> -> <target_variable>) or (global <varname> <- <source_variable>). Either way, global expression have 4 elements."));
		  }

		  if (!IsAtomic(s[1]))
		  {
			 Throw(s[1], ("Expecting atomc value for <varname> in (global <varname> ..."));
		  }

			if (!IsAtomic(s[2]))
			{
				Throw(s[2], ("Expecting -> or <- in (global <varname> -> ..."));
			}

		  if (!IsAtomic(s[3]))
		  {
			 Throw(s[3], ("Expecting source or target variable name in (global <varname> <-/-> <source/target>)"));
		  }

			sexstring globalName = s[1].String();
			sexstring localVarName = s[3].String();
			sexstring operation = s[2].String();

			GlobalValue* g = GetGlobalValue(ce.Script, globalName->Buffer);
			if (g == nullptr)
			{
				Throw(s[1], ("Unrecognized global variable name"));
			}

			VariantValue literalValue;
			if (Parse::PARSERESULT_GOOD == Parse::TryParse(literalValue, g->type, localVarName->Buffer))
			{
				if (!AreEqual(operation, ("<-")))
				{
					Throw(s[3], ("Expecting <- when assigning a literal expression to a global variable"));
				}

				ce.Builder.AssignLiteralToGlobal(*g, literalValue);
				return;
			}

			MemberDef def;
			if (!ce.Builder.TryGetVariableByName(def, localVarName->Buffer))
			{
				Throw(s[3], ("Expecting local variable name"));
			}

			if (def.ResolvedType->VarType() != g->type)
			{
				sexstringstream<1024> streamer;
				streamer.sb << ("The global variable type ") << GetTypeName(g->type) << (" does not match the local variable type ") << def.ResolvedType->Name();
				Throw(s, streamer);
			}

			if (AreEqual(operation, ("->")))
			{
				// assign local value
				ce.Builder.AssignVariableFromGlobal(*g, def);
			}
			else if (AreEqual(operation, ("<-")))
			{
				// assign global value
				ce.Builder.AssignVariableToGlobal(*g, def);
			}
			else
			{
				Throw(s[2], ("Expecting <- or ->"));
			}
		}

		bool TryCompileAsKeyword(CCompileEnvironment& ce, sexstring token, cr_sex s)
		{
			if (AreEqual(token, "if"))
			{
				CompileIfThenElse(ce, s);
				return true;
			}
			else if (AreEqual(token, "cast"))
			{
				CompileCast(ce, s);
				return true;
			}
			else if (AreEqual(token, "while"))
			{
				CompileWhileLoop(ce, s);
				return true;
			}
			else if (AreEqual(token, "break"))
			{
				CompileBreak(ce, s);
				return true;
			}
			else if (AreEqual(token, "continue"))
			{
				CompileContinue(ce, s);
				return true;
			}
			else if (AreEqual(token, "do"))
			{
				CompileDoWhile(ce, s);
				return true;
			}
			else if (AreEqual(token, "foreach"))
			{
				CompileForEach(ce, s);
				return true;
			}
			else if (AreEqual(token, "throw"))
			{
				CompileThrow(ce, s);			
				return true;
			}
			else if (AreEqual(token, "construct"))
			{
				CompileConstructInterfaceCall(ce, s);
				return true;
			}
			else if (AreEqual(token, "try"))
			{
				CompileExceptionBlock(ce, s);
				return true;
			}
			else if (AreEqual(token, "return"))
			{
				CompileReturnFromFunction(ce, s);
				return true;
			}
			else if (AreEqual(token, "array"))
			{
				CompileArrayDeclaration(ce, s);
				return true;
			}
			else if (AreEqual(token, "list"))
			{
				CompileListDeclaration(ce, s);
				return true;
			}
			else if (AreEqual(token, "map"))
			{
				CompileMapDeclaration(ce, s);
				return true;
			}
			else if (AreEqual(token, "node"))
			{
				CompleAsNodeDeclaration(ce, s);
				return true;
			}
			else if (AreEqual(token, "yield"))
			{
				CompileAsYield(ce, s);
				return true;
			}
			else if (AreEqual(token, "global"))
			{
				CompileAsGlobalAccess(ce, s);
				return true;
			}
			else if (AreEqual(token, "debug"))
			{
				CompileTrip(ce, s);
				return true;
			}
			else if (AreEqual(token, "serialize"))
			{
				CompileSerialize(ce, s);
				return true;
			}
			else if (AreEqual(token, "reflect"))
			{
				CompileReflect(ce, s);
				return true;
			}
			else
			{
				return false;
			}
		}

		void ReturnVariableInterface(CCompileEnvironment& ce, const MemberDef& def, cr_sex exceptionSource, cstr sourceName, cstr outputName, const MemberDef& output)
		{
			const IStructure& src = *def.ResolvedType;
			const IInterface& outputInterface = src.GetInterface(0);

			if (!IsNullType(src))
			{
				for(int i = 0; i < src.InterfaceCount(); ++i)
				{
					if (outputInterface == src.GetInterface(i))
					{
						int instanceToInterfaceOffset = sizeof(ObjectStub) + ((i - 1) * sizeof(void*));
						ce.Builder.Assembler().Append_GetStackFrameMemberPtr(VM::REGISTER_D4, def.SFOffset, def.MemberOffset + instanceToInterfaceOffset);
						ce.Builder.Assembler().Append_SetStackFrameValue(output.SFOffset, VM::REGISTER_D4, BITCOUNT_POINTER);
						return;
					}
				}

				sexstringstream<1024> streamer;
				streamer.sb <<  ("The RHS type '") << GetFriendlyName(src) << ("' does not implement interface ") << outputInterface.Name();
				Throw(exceptionSource, streamer);
			}
			else
			{
				ce.Builder.AssignVariableToVariable(sourceName, outputName);
			}
		}

		void AssignArrayToArray(CCompileEnvironment& ce, cr_sex s, const MemberDef& lhs, cstr lhsString, cstr rhsString)
		{
			MemberDef rhs;
			if (!ce.Builder.TryGetVariableByName(OUT rhs, rhsString) || rhs.ResolvedType != &ce.StructArray()) 
			{
				Throw(s, "Expecting array type %s", rhsString);
			}

			const IStructure& lhsType = GetElementTypeForArrayVariable(ce, s, lhsString);
			const IStructure& rhsType = GetElementTypeForArrayVariable(ce, s, rhsString);

			if (&lhsType != &rhsType)
			{
				Throw(s, "Could not assign array. LHS is (array %s). RHS is (array %s)", GetFriendlyName(lhsType), GetFriendlyName(rhsType));
			}

			if (!StartsWith(lhsString, "this.") && lhs.location != VARLOCATION_TEMP && lhs.location != VARLOCATION_OUTPUT)
			{
				Throw(s, "Could not assign array. The target has to be an output or temporary/local variable");
			}

			ce.Builder.AssignVariableToTemp(lhsString, 0, 0); // D4 has the LHS reference
			ce.Builder.AssignVariableToTemp(rhsString, 1, 0); // D5 has the RHS reference
			AppendInvoke(ce, GetArrayCallbacks(ce).ArrayAssign, s);
		
			UseStackFrameFor(ce.Builder, lhs);

			if (!lhs.IsContained)
			{
				ce.Builder.Assembler().Append_SetStackFrameValue(lhs.SFOffset + lhs.MemberOffset, VM::REGISTER_D5, BITCOUNT_POINTER);
			}
			else
			{
				ce.Builder.Assembler().Append_SetStackFramePtrFromD5(lhs.SFOffset, lhs.MemberOffset);
			}
			RestoreStackFrameFor(ce.Builder, lhs);
		}

		void AssignVariableToVariable(CCompileEnvironment& ce, cr_sex exceptionSource, cstr lhs, cstr rhs)
		{
			try
			{
				MemberDef def;
				if (ce.Builder.TryGetVariableByName(OUT def, lhs))
				{
					if (def.ResolvedType->InterfaceCount() != 0)
					{
						ReturnVariableInterface(ce, def, exceptionSource, rhs, lhs, def);
						return;
					}
				}

				char symbol[256];
				SafeFormat(symbol, 256, ("%s = %s"), lhs, rhs);

				NamespaceSplitter splitter(rhs);

				cstr member = rhs;

				cstr body, tail;
				if (splitter.SplitHead(body, tail))
				{
					cstr mappedToken = ce.MapMethodToMember(tail);
					if (mappedToken != NULL)
					{
						NamespaceSplitter subSplitter(body);

						cstr instance, subInstance;
						if (!subSplitter.SplitTail(instance, subInstance))
						{
							subInstance = body;
						}

						const IStructure* st = ce.Builder.GetVarStructure(subInstance);
						if (st == NULL)
						{
							ThrowTokenNotFound(exceptionSource, subInstance, ce.Builder.Owner().Name(), ("structure"));
						}

						if (st == &ce.StructArray())
						{
							TokenBuffer mappedSymbol;
							StringPrint(mappedSymbol, ("%s.%s"), body, mappedToken);
							ce.Builder.AddSymbol(symbol);
							ce.Builder.AssignVariableToVariable(mappedSymbol, lhs);
							return;
						}
					}
					else if (AreEqual(tail, ("PopOut")))
					{
						VARTYPE type = ce.Builder.GetVarType(lhs);
						if (!IsPrimitiveType(type))
						{
							Throw(exceptionSource, ("Only primitive types can be popped out from an array"));
						}
						CompileAsPopOutFromArray(ce, exceptionSource, body, ce.Builder.GetVarType(lhs));
						ce.Builder.AssignTempToVariable(Rococo::ROOT_TEMPDEPTH, lhs);
						return;
					}
				}

				if (def.ResolvedType == &ce.StructArray())
				{
					AssignArrayToArray(ce, exceptionSource, def, lhs, rhs);
				}
				else
				{
					ce.Builder.AddSymbol(symbol);
					ce.Builder.AssignVariableToVariable(rhs, lhs);
				}
			}
			catch (IException& e)
			{
				Throw(exceptionSource, e.Message());
			}
		}

		void CompileTrivialAssignment(CCompileEnvironment& ce, cr_sex s, sexstring lhs, sexstring rhs)
		{
			AssignVariableToVariable(ce, s, lhs->Buffer, rhs->Buffer);	
		}

		void CompileTrivialStringAssign(CCompileEnvironment& ce, cr_sex s, sexstring lhs, sexstring rhs)
		{
			cstr variableName = lhs->Buffer;

			MemberDef def;
			if (!ce.Builder.TryGetVariableByName(OUT def, variableName))
			{
				Throw(s, "String assign failed: cannot find %s", variableName);
			}

			const IInterface& istring = ce.Object.Common().SysTypeIString();
			if (def.ResolvedType != &istring.NullObjectType())
			{
				Throw(s, "Cannot assign a string to %s - not a Sys.Type.IString", variableName);
			}

			CStringConstant* sc = CreateStringConstant(ce.Script, rhs->Length, rhs->Buffer, &s);

			cstr format = (rhs->Length > 24) ? (" = '%.24s...'") : (" = '%s'");
			AddSymbol(ce.Builder, format, (cstr)rhs->Buffer);

			VariantValue ptrToStringConstant;
			ptrToStringConstant.vPtrValue = sc->header.pVTables;

			if (def.Usage == ARGUMENTUSAGE_BYVALUE || def.location == VARLOCATION_OUTPUT)
			{
				UseStackFrameFor(ce.Builder, def);
				ce.Builder.Assembler().Append_SetStackFrameImmediate(def.SFOffset + def.MemberOffset, ptrToStringConstant, BITCOUNT_POINTER);
				RestoreStackFrameFor(ce.Builder, def);
			}
			else
			{
				ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D7, ptrToStringConstant, BITCOUNT_POINTER);
				ce.Builder.AssignTempToVariable(Rococo::ROOT_TEMPDEPTH, variableName);
			}
		}

		bool IsValidVariableName(cstr token)
		{
			NamespaceSplitter splitter(token);

			cstr instance, member;
			if (splitter.SplitTail(instance, member))
			{
				return IsLowerCase(member[0]);
			}
			else
			{
				return IsLowerCase(token[0]);
			}
		}

		enum DeltaFunction
		{
			DELTA_SUBTRACT,
			DELTA_ADD,
			DELTA_MULTIPLY,
			DELTA_DIVIDE
		};

		static const char* deltaOps[]  = { "-=", "+=", "*=", "/=" };

		// Assume delta value is in D4, aggregate is in D5, and target will be D4
		void Append_FloatDeltaOp(CCompileEnvironment& ce, DeltaFunction opIndex, BITCOUNT bc)
		{
			VM::FLOATSPEC spec = (bc == BITCOUNT_64) ? VM::FLOATSPEC_DOUBLE : VM::FLOATSPEC_SINGLE;

			switch (opIndex)
			{
			case DELTA_SUBTRACT:
				ce.Builder.Assembler().Append_FloatSubtract(VM::REGISTER_D5, VM::REGISTER_D4, spec);
				break;
			case DELTA_ADD:
				ce.Builder.Assembler().Append_FloatAdd(VM::REGISTER_D5, VM::REGISTER_D4, spec);
				break;
			case DELTA_MULTIPLY:
				ce.Builder.Assembler().Append_FloatMultiply(VM::REGISTER_D5, VM::REGISTER_D4, spec);
				break;
			case DELTA_DIVIDE:
				ce.Builder.Assembler().Append_FloatDivide(VM::REGISTER_D5, VM::REGISTER_D4, spec);
				break;
			}
		}

		bool TryCompileAsDeltaStatement(CCompileEnvironment& ce, cr_sex s)
		{
			int nElements = s.NumberOfElements();
			if (nElements != 3)
			{
				return false;
			}

			cr_sex lhs = s.GetElement(0);
			cr_sex ops = s.GetElement(1);
			cr_sex rhs = s.GetElement(2);

			if (!IsAtomic(lhs) || !IsAtomic(ops)) return false;

			cstr name = lhs.String()->Buffer;
			cstr op = ops.String()->Buffer;

			MemberDef def;
			if (!ce.Builder.TryGetVariableByName(def, name))
				return false;

			int opIndex = -1;

			for (int i = 0; i < 4; i++)
			{
				if (AreEqual(deltaOps[i], op))
				{
					opIndex = i;
					break;
				}
			}

			if (opIndex == -1)
			{
				return false;
			}

			if (!IsAtomic(rhs))
			{
				Throw(rhs, "Delta operator '%s' requires rhs is a variable or a literal value", deltaOps[opIndex]);
			}

			auto type = def.ResolvedType->VarType();

			switch (type)
			{
			case VARTYPE_Int32:
			case VARTYPE_Int64:
			case VARTYPE_Float32:
			case VARTYPE_Float64:
				break;
			default:
				Throw(lhs, "Delta operator '%s' requires lhs is a numeric variable: Int32, Int64, Float32 or Float64", deltaOps[opIndex]);
			}

			auto bc = GetBitCount(type);

			cstr src = rhs.String()->Buffer;

			VariantValue value;
			if (Parse::TryParse(value, type, src) == Parse::PARSERESULT_GOOD)
			{
				ce.Builder.AssignVariableToTemp(name, 1); // target variable value is now in D5

				if (type == VARTYPE_Int32 || type == VARTYPE_Int64)
				{
					switch (opIndex)
					{
					case DELTA_SUBTRACT:
						ce.Builder.Assembler().Append_SubtractImmediate(VM::REGISTER_D5, bc, VM::REGISTER_D4, value);
						break;
					case DELTA_ADD:
						ce.Builder.Assembler().Append_AddImmediate(VM::REGISTER_D5, bc, VM::REGISTER_D4, value);
						break;
					case DELTA_MULTIPLY:
						ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D4, value, bc);
						ce.Builder.Assembler().Append_IntMultiply(VM::REGISTER_D5, bc, VM::REGISTER_D4); // result ends in D4
						break;
					case DELTA_DIVIDE:
						ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D4, value, bc);
						ce.Builder.Assembler().Append_IntDivide(VM::REGISTER_D5, bc, VM::REGISTER_D4); // result ends in D4
						break;
					}
				}
				else // F32 or F64
				{
					ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D4, value, bc);
					Append_FloatDeltaOp(ce, (DeltaFunction) opIndex, bc);
				}
			}
			else
			{
				// Assume rhs argument is source variable
				MemberDef sourceDef;
				if (!ce.Builder.TryGetVariableByName(sourceDef, src))
				{
					Throw(rhs, "Delta operator '%s' requires rhs is a numeric variable or literal value", deltaOps[opIndex]);
				}

				auto srcType = sourceDef.ResolvedType->VarType();

				if (srcType != type)
				{
					Throw(s, "%s and %s must be of the same type", name, src);
				}

				ce.Builder.AssignVariableToTemp(name, 1); // target variable value is now in D5
				ce.Builder.AssignVariableToTemp(src, 0); // src variable value is now in D4

				if (type == VARTYPE_Int32 || type == VARTYPE_Int64)
				{
					switch (opIndex)
					{
					case DELTA_SUBTRACT:
						ce.Builder.Assembler().Append_IntSubtract(VM::REGISTER_D5, bc, VM::REGISTER_D4);  // result ends in D4
						break;
					case DELTA_ADD:
						ce.Builder.Assembler().Append_IntAdd(VM::REGISTER_D5, bc, VM::REGISTER_D4);  // result ends in D4
						break;
					case DELTA_MULTIPLY:
						ce.Builder.Assembler().Append_IntMultiply(VM::REGISTER_D5, bc, VM::REGISTER_D4); // result ends in D4
						break;
					case DELTA_DIVIDE:
						ce.Builder.Assembler().Append_IntDivide(VM::REGISTER_D5, bc, VM::REGISTER_D4); // result ends in D4
						break;
					}
				}
				else // F32 or F64
				{
					Append_FloatDeltaOp(ce, (DeltaFunction)opIndex, bc);
				}
			}

			ce.Builder.AssignTempToVariable(0, name); // D4 now assigned back to the target variable
			return true;
		}

		bool TryCompileAsTrivialAssignment(CCompileEnvironment& ce, cr_sex s)
		{
			int nElements = s.NumberOfElements();
			if (nElements == 3)
			{
				cr_sex lhs = s.GetElement(0);
				cr_sex middle = s.GetElement(1);
				cr_sex rhs = s.GetElement(2);

				if (IsAtomic(lhs) && IsAtomic(middle) && (IsAtomic(rhs) || IsStringLiteral(rhs)) )
				{
					sexstring lhsToken = lhs.String();
					sexstring rhsToken = rhs.String();
					sexstring middleToken = middle.String();

					if (AreEqual(middleToken, ("=")))
					{
						if (IsStringLiteral(rhs))
						{
							CompileTrivialStringAssign(ce, s, lhsToken, rhsToken);
							return true;
						}

						if (IsValidVariableName(lhsToken->Buffer) && IsValidVariableName(rhsToken->Buffer))
						{
							if (AreEqual(rhsToken, ("true")) || AreEqual(rhsToken, ("false")))
							{
								return false;
							}

							MemberDef targetDef;
							if (!ce.Builder.TryGetVariableByName(targetDef, lhsToken->Buffer))
							{
								Throw(lhs, ("Unrecognized variable name"));
							}

							if (targetDef.CapturesLocalVariables)
							{
								Throw(lhs, ("The target variable refers to a closure. It is immutable."));
							}

							CompileTrivialAssignment(ce, s, lhsToken, rhsToken);
							return true;
						}
					}
				}
			}

			return false;
		}

		bool TryCompileMacroInvocation(CCompileEnvironment& ce, cr_sex s, sexstring token)
		{
			if (token->Buffer[0] != '#')
			{
				return false;
			}

			cstr macroName = token->Buffer+1;

			NamespaceSplitter splitter(macroName);	

			const IFunction *f;

			cstr nsBody, fname;
			if (splitter.SplitTail(OUT nsBody, OUT fname))
			{
				INamespace& ns = AssertGetNamespace(ce.Builder.Module().Object(), s, nsBody);

				const IMacro* macro = ns.FindMacro(fname);
				if (macro == NULL) ThrowTokenNotFound(s, fname, ns.FullName()->Buffer, ("macro"));		
				f = &macro->Implementation();
			}
			else
			{
				f = ce.Builder.Module().FindFunction(macroName);
				if (f == NULL)
				{
					INamespace* firstPrefix = NULL;

					for(int i = 0; i < ce.Builder.Module().PrefixCount(); ++i)
					{
						INamespace& prefix = ce.Builder.Module().GetPrefix(i);
						const IMacro* macro = prefix.FindMacro(macroName);
						if (macro != NULL)
						{
							if (f == NULL)
							{
								f = &macro->Implementation();
								firstPrefix = &prefix;
							}
							else
							{
								ThrowNamespaceConflict(s, *firstPrefix, prefix, ("macro"), macroName);
							}
						}
					}
				}

				if (f == NULL)
				{
					ThrowTokenNotFound(s, macroName, ce.Builder.Module().Name(), ("macro"));
				}
			}

			CallMacro(ce, *f, s);

			return true;
		}

		void CompileCommand(CCompileEnvironment& ce, cr_sex s, sexstring token)
		{
			if (TryCompileMacroInvocation(ce, s, token))
			{
				const ISExpression* t = ce.Script.GetTransform(s);
				if (t != NULL)
				{
 					CompileExpression(ce, *t);
				}

				return;
			}

			if (token->Buffer[0] == ('\''))
			{
				// Data expression, skip
				return;
			}

			if (IsCapital(token->Buffer[0]))
			{
				if (TryCompileAsPlainFunctionCall(ce, s))
				{
					return;
				}
				else if (s.NumberOfElements() >= 2)
				{
					CompileAsVariableDeclaration(ce, s);	
					return;
				}
				else
				{
					Throw(s, ("Expression was not recognized as a valid function call, and has too few elements to be a variable declaration"));
				}
			}
			else
			{
				if (TryCompileAsTrivialAssignment(ce, s))
					return;

				if (TryCompileAsImplicitSetDirective(ce, s))
					return;		

				if (TryCompileAsDerivativeFunctionCall(ce, s))
					return;		

				if (TryCompileAsKeyword(ce, token, s))
					return;

				if (TryCompileAsDeltaStatement(ce, s))
					return;
			}
		
			sexstringstream<1024> streamer;

			const IStructure* varStruct = ce.Builder.GetVarStructure(token->Buffer);
			if (NULL == varStruct)
			{
				streamer.sb << ("Unrecognized keyword/variable/function/namespace/syntax in expression: ") << token->Buffer;
			}
			else
			{
				streamer.sb << ("Variable recognized, but the syntax in which it is used was not: ") << GetFriendlyName(*varStruct) << (" ") << token->Buffer;
			}
		
			Throw(s, streamer);
		}

		void StreamSTCEX(StringBuilder& streamer, const STCException& ex)
		{
			streamer << ("Compiler exception code: ") << ex.Code() << (".\n") << ("Source: ") << ex.Source() << ("\n. Message: ") << ex.Message();
		}

		void CompileExpression(CCompileEnvironment& ce, cr_sex s)
		{
			ce.Builder.MarkExpression(&s);

			try
			{
				AssertCompound(s);

				if (s.NumberOfElements() == 0) return;

				cr_sex arg = s.GetElement(0);
				if (IsAtomic(arg))
				{
					sexstring command = arg.String();
					CompileCommand(ce, s, command);
				}
				else
				{
					CompileExpressionSequence(ce, 0, s.NumberOfElements() - 1, s);
				}
			}
			catch (STCException& ex)
			{
				sexstringstream<1024> streamer;
				StreamSTCEX(streamer.sb, ex);
				Throw(s, *streamer.sb);
			}
		}
	}
}

