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
#include "..\STC\stccore\sexy.compiler.helpers.h"
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
	int CALLTYPE_C StringPrint(TokenBuffer& token, const char* format, ...)
	{
		va_list args;
		va_start(args, format);
		return SafeVFormat(token.Text, TokenBuffer::MAX_TOKEN_CHARS, format, args);
	}

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

		void AddAttribute(IStructureBuilder& s, cr_sex attributeDef)
		{
			cr_sex sAttributeType = GetAtomicArg(attributeDef, 1);

			bool isCustom = false;

			if (sAttributeType.Type() == EXPRESSION_TYPE_ATOMIC)
			{
				// A system attribute
			}
			else if (sAttributeType.Type() == EXPRESSION_TYPE_STRING_LITERAL)
			{
				// A user-defined attribute
				isCustom = true;
			}
			else
			{
				Throw(sAttributeType, "Expecting either an atomic or string literal token for the literal type. Atomic tokens are used for system attributes. String literals for user defined custom attributes");
			}

			sexstring attributeType = sAttributeType.String();

			if (!isCustom)
			{
				if (AreEqual(attributeType, "not-serialized"))
				{
					// This is used by the Rococo LoadAsset and SaveAsset API to indicate the object should be replaced by a null-object when serialized
				}
				else
				{
					Throw(sAttributeType, "Unknown system attribute. Expecting 'not-serialized'");
				}
			}

			s.AddAttribute(attributeDef, isCustom);
		}

		void AddMember(IStructureBuilder& s, cr_sex field)
		{
			AssertCompound(field);

			cr_sex argTypeExpr = GetAtomicArg(field, 0);
			sexstring type = argTypeExpr.String();

			AssertNotTooFewElements(field, 2);

			if (AreEqual(type, "attribute"))
			{
				AddAttribute(s, field);
				return;
			}

			if (s.Prototype().IsClass)
			{
				if (AreEqual(type, ("implements")))
				{
					for (int i = 1; i < field.NumberOfElements(); ++i)
					{
						cr_sex nameExpr = field.GetElement(i);
						AssertQualifiedIdentifier(nameExpr);
						s.AddInterface(nameExpr.c_str());
					}

					return;
				}
				else if (AreEqual(type, ("defines")))
				{
					if (field.NumberOfElements() == 2 && IsAtomic(field[1]))
					{
						s.AddInterface(field[1].c_str());
					} 
					else if (field.NumberOfElements() == 4 && IsAtomic(field[1]) && IsAtomic(field[2]) && IsAtomic(field[3]) && AreEqual(field[2].String(), ("extends")))
					{
						s.AddInterface(field[1].c_str());
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
			else if (AreEqual(type, "list"))
			{
				cr_sex elementTypeExpr = GetAtomicArg(field, 1);
				sexstring elementType = elementTypeExpr.String();

				AssertQualifiedIdentifier(elementTypeExpr);

				for(int i = 2; i < field.NumberOfElements(); ++i)
				{
					cr_sex nameExpr = field.GetElement(i);
					AssertLocalIdentifier(nameExpr);			

					s.AddMember(NameString::From(nameExpr.String()), TypeString::From("_List"), elementType->Buffer);
				}	
			}
			else if (AreEqual(type, "map"))
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

					s.AddMember(NameString::From(nameExpr.String()), TypeString::From("_Map"), keyType->Buffer, valueType->Buffer);
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

		cstr Comparative(int delta) { return delta > 0 ? "more" : "fewer"; } 

		void ValidateArchetypeMatchesArchetype(cr_sex s, const IArchetype& f, const IArchetype& requiredArchetype, cstr source)
		{
			int delta = requiredArchetype.NumberOfInputs() - f.NumberOfInputs();
			if (delta != 0)
			{
				Throw(s, "There are %s inputs in %s.%s than in that of %s", Comparative(delta), source, f.Name(), requiredArchetype.Name());
			}

			delta = requiredArchetype.NumberOfOutputs() - f.NumberOfOutputs();
			if (delta != 0)
			{
				Throw(s, "There are %s outputs in %s.%s than in that of %s", Comparative(delta), source, f.Name(), requiredArchetype.Name());
			}

			int32 argCount = ArgCount(f);

			for(int32 i = 0; i < argCount; ++i)
			{
				const IStructure& st = requiredArchetype.GetArgument(i);
				const IStructure& stf = f.GetArgument(i);
				cstr argname = f.GetArgName(i);

				if (requiredArchetype.IsVirtualMethod())
				{
					if (i == 0 && st.VarType() == VARTYPE_Pointer && AreEqual(requiredArchetype.GetArgName(0), "_typeInfo"))
					{
						if (stf.Prototype().IsClass && AreEqual(THIS_POINTER_TOKEN,argname))
						{
							continue;
						}
					}
					else if (st.VarType() == VARTYPE_Pointer && AreEqual(requiredArchetype.GetArgName(i), "_vTable", 7))
					{
						if (stf.Prototype().IsClass && AreEqual(THIS_POINTER_TOKEN,argname))
						{
							continue;
						}
					}
				}
			
				if (&stf != &st)
				{
					auto sf = (const ISExpression*)f.Definition();
					Throw(sf ? *sf : s, "%s.%s: Argument [%d](%s %s). Type did not match the expected '%s'", source, requiredArchetype.Name(), i, GetFriendlyName(stf), argname, GetFriendlyName(st));
				}

				const IStructure* interfGenericArg1 = requiredArchetype.GetGenericArg1(i);
				const IStructure* concreteGenericArg1 = f.GetGenericArg1(i);

				if (interfGenericArg1 != concreteGenericArg1)
				{
					char buf[1024];
					StackStringBuilder ssb(buf, sizeof buf);

					// Not really expecting the generic args to be NULL, as we should already have bailed out above, but handle the case

					ssb << "Error validating concrete method against the interface's specification for (" << f.Name() << "...). \n";
					if (requiredArchetype.GetGenericArg1(i) != NULL)
					{
						ssb << "Interface's method with generic argument type '" << GetFriendlyName(*interfGenericArg1) << "' does not match ";
					}
					else
					{
						ssb << "Interface's method has no generic argument type and so does not match ";
					}
				
					if (f.GetGenericArg1(i) != NULL)
					{
						ssb << "concrete generic argument type '" << GetFriendlyName(*concreteGenericArg1) << "'";
					}
					else
					{
						ssb << "concrete method with no generic argument type";
					}

				
					Throw(f.Definition() != NULL ? *(const ISExpression*)(f.Definition()) : s, "%s", buf);
				}
			}
		}

		void CompileAssignmentDirectiveFromAtomic(CCompileEnvironment& ce, cr_sex directive, const IStructure& varStruct, bool explicitKeyword)
		{
			int offset = explicitKeyword ? 0 : -1; // explicit means the directive begins with the assignment keyword, else it begins with the target variable
			cr_sex targetExpr = GetAtomicArg(directive, 1 + offset);
			cstr targetVariable = targetExpr.c_str();
			cr_sex assignmentChar = GetAtomicArg(directive, 2 + offset);
			cr_sex sourceValue = directive.GetElement(3 + offset);
			VARTYPE targetType = varStruct.VarType();

			cstr sourceText = sourceValue.c_str();

			TokenBuffer symbol;
			StringPrint(symbol, ("%s=%s"), targetVariable, (cstr) sourceValue.c_str());
		
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
						Throw(directive, "The type %s of %s does not match the type %s of %s", Parse::VarTypeName(targetType), targetVariable, Parse::VarTypeName(sourceType), sourceText);
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
							Throw(directive, "Could not copy %s to %s. The target variable accepts regular function references, but not closures.", sourceText, targetVariable);
						}
						
						ce.Builder.AddSymbol(symbol);
						ce.Builder.AssignVariableToVariable(sourceText, targetVariable);
						return;
					}
					else
					{
						Throw(directive, "The type of %s does not match the type of %s", targetVariable, sourceText);
					}
				}
				else if (sourceType == VARTYPE_Derivative)
				{
					MemberDef def;
					if (!ce.Builder.TryGetVariableByName(def, sourceText))
					{
						Throw(directive, "Could not find variable %s", sourceText);
					}

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
			cstr targetVariable = GetAtomicArg(directive, 1 + offset).c_str();
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
					// D7 assembled to now hold the value of the boolean value of the source expression
					ce.Builder.AddSymbol(symbol);
					if (negate) ce.Builder.Assembler().Append_BooleanNot(VM::REGISTER_D7);
					ce.Builder.AssignTempToVariable(Rococo::ROOT_TEMPDEPTH, targetVariable);
					return;
				}
				break;
			case VARTYPE_Float32:
			case VARTYPE_Int32:
			case VARTYPE_Int64:
			case VARTYPE_Float64:
				if (TryCompileArithmeticExpression(ce, sourceValue, true, targetType))
				{
					// D7 assembled to now hold the value of the boolean value of the source expression
					ce.Builder.AddSymbol(symbol);
					ce.Builder.AssignTempToVariable(Rococo::ROOT_TEMPDEPTH, targetVariable);
					return;
				}
				break;
			case VARTYPE_Closure:
				// D7 assembled to now hold the value of the boolean value of the source expression
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
			cstr sourceText = src.c_str();

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
						Throw(src, "The type of %s  does not match the type of %s",targetVariable, sourceText);
					}
				}
				else
				{
					Throw(src, "Cannot assign to type of %s. The source is not a primitive type", targetVariable);
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
				ce.Builder.AssignVariableToVariable(src.c_str(), variableName, true);
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
						ce.Builder.AssignVariableToVariable(src.c_str(), variableName, isConstructing);
						return;
					}
					else if (!IsCompound(src))
					{
						Throw(src, "%s is a derived type, and requires a compound initializer", memberType.Name());
						return;
					}

					if (src.NumberOfElements() != PublicMemberCount(memberType))
					{
						Throw(src, "%s has %d elements. But %d were supplied", member.Name(), memberType.MemberCount(), src.NumberOfElements());
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
				Throw(src, "%s is a bad type, and cannot be initialized", memberType.Name());
				return;
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

		bool CompileAsStructureAssignmentFromCompound(CCompileEnvironment& ce, const IStructure& varStruct, cstr varName, cr_sex value);
	
		void CompileMemberwiseAssignment(CCompileEnvironment& ce, cr_sex directive, const IStructure& varStruct, int offset)
		{
			cstr targetVariable = GetAtomicArg(directive, 1 + offset).c_str();
			
			if (PublicMemberCount(varStruct) + 3 + offset != directive.NumberOfElements())
			{
				if (directive.NumberOfElements() == 4)
				{
					// We may an element look up: (Vec3 w = (array 999))
					cr_sex sValue = directive[3];
					if (sValue.NumberOfElements() == 2)
					{
						if (CompileAsStructureAssignmentFromCompound(ce, varStruct, targetVariable, sValue))
						{
							return;
						}
					}
				}
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
		   if (op[0] == 0 || op[1] != 0)
		   {
			   return nullptr;
		   }

		   switch (*op)
		   {
		   case '+':
			   return "Add";
		   case '-':
			   return "Subtract";
		   case '*':
			   return "Multiply";
		   case '/':
				return "Divide";
		   default:
			   return nullptr;
		   }
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
		  Throw(svalue, "Cannot infer best variable type for atomic expression");
	   }

	   // Infer a type for an expression
	   const IStructure& GetBestType(CCompileEnvironment& ce, cr_sex s, const IStructure* hintStruct)
	   {
		  switch (s.Type())
		  {
		  case EXPRESSION_TYPE_ATOMIC:
			 return GetBestType(ce, s, s.c_str(), hintStruct);
		  case EXPRESSION_TYPE_COMPOUND:
			 Throw(s, ("Cannot infer variable type from compound expression"));
		  case EXPRESSION_TYPE_STRING_LITERAL:
			 return ce.Object.Common().SysTypeIString().NullObjectType();
		  default:
			 Throw(s, "Cannot infer variable type from expression");
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
			 cstr op = sop.c_str();
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

	   void CompileAssignmentToTypeOf(CCompileEnvironment& ce, cr_sex directive, const IStructure& varStruct)
	   {
		   // Example: (IStructure t = typeof Sys.Reflection.IStructure)
		   if (ce.Object.Common().SysReflection().FindInterface("IStructure")->NullObjectType() != varStruct)
		   {
			   Throw(directive, "Expecting the assignment type variable to be Sys.Reflection.IStructure. It was a %s of %s", GetFriendlyName(varStruct), varStruct.Module().Name());
		   }

		   int offset = directive.NumberOfElements() == 4 ? 1 : 0;

		   cstr variableName = GetAtomicArg(directive, offset + 1).c_str();
		   cstr eqOperator = GetAtomicArg(directive, offset + 2).c_str();
		   cstr typeOperator = GetAtomicArg(directive, offset + 3).c_str();
		   cstr fqTypeName = GetAtomicArg(directive, offset + 4).c_str();

		   if (!Eq(typeOperator, "typeof"))
		   {
			   Throw(directive[offset+3], "(invoke ...) Expecting typeof in position %d", offset + 3);
		   }

		   if (!Eq(eqOperator, "="))
		   {
			   Throw(directive[offset+2], "(invoke ...) Expecting assignment operator '='");
		   }

		   cstr nsprefix, type;
		   NamespaceSplitter splitter(fqTypeName);
		   if (!splitter.SplitTail(nsprefix, type))
		   {
			   MemberDef typeDef;
			   if (!ce.Builder.TryGetVariableByName(typeDef, fqTypeName))
			   {
				   Throw(directive[offset + 4], "(IStructure type = <bad-mojo>). Expecting RHS to be fully qualified namespace-and-interface-type or a variable name");
			   }

			   if (typeDef.ResolvedType->InterfaceCount() != 1)
			   {
				   Throw(directive[offset + 4], "(IStructure type = <bad-mojo>). RHS Variable was not of interface type. It was a %s", GetFriendlyName(*typeDef.ResolvedType));
			   }

			   auto& interf0 = typeDef.ResolvedType->GetInterface(0);

			   AddSymbol(ce.Builder, "interface ref '%s'", fqTypeName);
			   ce.Builder.PushVariable(typeDef);
			   AddSymbol(ce.Builder, "class type -> D4");
			   ce.Builder.Assembler().Append_Invoke(ce.SS.GetScriptCallbacks().idVariableRefToType);
			   ce.Builder.AssignTempToVariable(0, variableName);
			   return;
		   }
		   else
		   {
			   auto* ns = ce.RootNS.FindSubspace(nsprefix);
			   if (ns == nullptr)
			   {
				   Throw(directive[offset + 4], "(invoke ...) Could not find a namespace %s", nsprefix);
			   }

			   if (!IsCapital(type[0]))
			   {
				   Throw(directive[offset + 4], "(invoke ...) Expecting type '%s' to begin with a capital letter", type);
			   }

			   auto* sType = ns->FindStructure(type);
			   if (sType != nullptr)
			   {
				   AddSymbol(ce.Builder, "%s = typeof %s", variableName, GetFriendlyName(*sType));

				   CReflectedClass* pStruct = ce.SS.GetReflectedClass(sType);
				   if (pStruct == NULL)
				   {
					   pStruct = ce.SS.CreateReflectionClass("Structure", sType);
				   }

				   ce.Builder.AssignPointer(NameString::From(variableName), &pStruct->header.pVTables[0]);
				   return;
			   }
		   }

		   Throw(directive, "(invoke ...) No type found that matches %s", fqTypeName);
	   }

	   void CompileAssignmentDirective(CCompileEnvironment& ce, cr_sex directive, const IStructure& varStruct, bool explicitKeyword)
	   {
		   int offset = explicitKeyword ? 0 : -1; // explicit means the directive begins with the assignment type, else it begins with the target variable

		   if (directive.NumberOfElements() == 5 + offset)
		   {
			   int typeOfOffset = 3 + offset;
			   if (IsAtomic(directive[typeOfOffset]) && Eq(directive[typeOfOffset].c_str(), "typeof"))
			   {
				   CompileAssignmentToTypeOf(ce, directive, varStruct);
				   return;
			   }
		   }

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

					   if (IsLowerCase(srctext->Buffer[0]))
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
			   Throw(directive, "Bad expression on RHS of assignment: %s", GetAtomicArg(directive, 1 + offset).c_str());
		   }
	   }

		void ValidateUnusedVariable(cr_sex identifierExpr, ICodeBuilder& builder)
		{
			cstr id = identifierExpr.c_str();
			if (builder.GetVarType(id) != VARTYPE_Bad)
			{
				Throw(identifierExpr, "Variable name %s is already defined in the context", id);
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
				Throw(typeExpr, "Cannot find constructor in source module: %s", qualifiedConstructorName);
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
					else if (memberType->VarType() == VARTYPE_Array || memberType->VarType() == VARTYPE_Map || memberType->VarType() == VARTYPE_List)
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

				if (memberType->VarType() == VARTYPE_List)
				{
					return true;
				}

				if (memberType->VarType() == VARTYPE_Map)
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
				Throw(*args.Parent(), "Needs a constructor function %s.Construct inside %s", type.Name(), type.Module().Name());
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
			AddSymbol(ce.Builder, format, (cstr) value.c_str());
		
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

			cstr nodeName = nodeNameExpr.c_str();
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
					Throw(decl, "Maps with value type of an interface do not support the '&' directive.Use node.Value instead");
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
			AddInterfaceVariable(ce, NameString::From(id), st);

			ObjectStub* stub = st.GetInterface(0).UniversalNullInstance();

			VariantValue value;
			value.vPtrValue = GetInterfacePtrFromNullInstancePtr(stub);

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
				OS::TripDebugger();
				cstr enigma = typeExpr.c_str();
				Throw(decl, "%s: Could not match [%s] as either a structure nor an interface in module %s\n"
							"Try specifying %s as a fully qualified type, or add the correct (using <namespace>) directive.", __FUNCTION__, enigma, source.Name(), enigma);
			}

			ValidateLocalDeclarationVariable(*st, idExpr);
			cstr id = idExpr.c_str();
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

			cstr op = operatorExpr.c_str();
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
				cstr commandText = command.c_str();

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

						ce.Builder.Assembler().Append_Invoke(GetArrayCallbacks(ce).ArrayCopyByRef);
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
			 cstr varName = directive.GetElement(0).c_str();
			 cr_sex rhs = directive.GetElement(2);

			 switch (rhs.Type())
			 {
			 case EXPRESSION_TYPE_COMPOUND:
				return CompileAsStructureAssignmentFromCompound(ce, varStruct, varName, rhs);
			 default:
				Throw(directive, "Non-compound structure assignment is not yet implemented");
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
			cstr token = keywordExpr.c_str();

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
				else if (def.Usage == ARGUMENTUSAGE_BYREFERENCE && (def.ResolvedType->VarType() == VARTYPE_Map || def.ResolvedType->VarType() == VARTYPE_List || def.ResolvedType->VarType() == VARTYPE_Array))
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
				auto* sClassDef = classType.Definition();
				auto* sInterfaceDef = thisInterf.NullObjectType().Definition();
				if (sClassDef && sInterfaceDef)
				{
					Throw(s, "The %s class does not implement the interface %s (defined in %s near line %d). The class is defined in %s near line %d", className->Buffer, thisInterf.Name(), sInterfaceDef->Tree().Source().Name(), sInterfaceDef->Start().y, sClassDef->Tree().Source().Name(), sClassDef->Start().y);
				}

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

			ce.Builder.Assembler().Append_PushRegister(VM::REGISTER_D9,  BITCOUNT_POINTER);
			ce.Builder.Assembler().Append_PushRegister(VM::REGISTER_D10, BITCOUNT_POINTER);
			ce.Builder.Assembler().Append_PushRegister(VM::REGISTER_D11, BITCOUNT_POINTER);
			ce.Builder.Assembler().Append_PushRegister(VM::REGISTER_D12, BITCOUNT_POINTER);
			ce.Builder.Assembler().Append_PushRegister(VM::REGISTER_D13, BITCOUNT_POINTER);
			ce.Builder.Assembler().Append_PushRegister(VM::REGISTER_D14, BITCOUNT_POINTER);
			ce.Builder.Assembler().Append_PushRegister(VM::REGISTER_D15, BITCOUNT_POINTER);

			VariantValue v;
			v.vPtrValue = (void*)&ce.SS;
			ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D9, v, BITCOUNT_POINTER); // D9 gets the IPublicScriptSystem ref

			v.vPtrValue = (void*)&s;
			ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D10, v, BITCOUNT_POINTER); // D10 gets the lhs type

			v.vPtrValue = (void*)lhsVariableDef.ResolvedType;
			ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D11, v, BITCOUNT_POINTER); // D1 gets the lhs type

			ce.Builder.AssignVariableRefToTemp(lhsVariableName->Buffer, 8); // D12 gets the lhs reference

			VariantValue name;
			name.vPtrValue = (void*)lhsVariableName->Buffer;
			ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D13, name, BITCOUNT_POINTER); // D13 gets the rhs name

			v.vPtrValue = (void*)rhsVariableDef.ResolvedType;
			ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D14, v, BITCOUNT_POINTER); // D14 gets the rhs type

			ce.Builder.AssignVariableRefToTemp(rhsVariableName->Buffer, 11); // D15 gets the rhs reference

			ce.Builder.Assembler().Append_Invoke(id);

			ce.Builder.Assembler().Append_RestoreRegister(VM::REGISTER_D15, BITCOUNT_POINTER);
			ce.Builder.Assembler().Append_RestoreRegister(VM::REGISTER_D14, BITCOUNT_POINTER);
			ce.Builder.Assembler().Append_RestoreRegister(VM::REGISTER_D13, BITCOUNT_POINTER);
			ce.Builder.Assembler().Append_RestoreRegister(VM::REGISTER_D12, BITCOUNT_POINTER);
			ce.Builder.Assembler().Append_RestoreRegister(VM::REGISTER_D11, BITCOUNT_POINTER);
			ce.Builder.Assembler().Append_RestoreRegister(VM::REGISTER_D10, BITCOUNT_POINTER);
			ce.Builder.Assembler().Append_RestoreRegister(VM::REGISTER_D9, BITCOUNT_POINTER);
		}

		void CompileSerializeFromInterface(cr_sex s, CCompileEnvironment& ce, const MemberDef& srcDef, const MemberDef& trgDef)
		{
			if (trgDef.ResolvedType->VarType() != VARTYPE_Derivative)
			{
				Throw(s, "(serialize <src> -> <targets>): The target was not a class or struct");
			}

			cstr srcName = s[1].c_str();
			ce.Builder.AssignVariableToTemp(srcName, 1); // D5

			cstr trgName = s[3].c_str();
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

		void CompileInvoke(CCompileEnvironment& ce, cr_sex s)
		{
			// (invoke <interface-ref> <method-string> <arg-variable>)
			if (s.NumberOfElements() != 4)
			{
				Throw(s, "(invoke <interface-ref> <method-string> <arg-variable>) has 3 arguments");
			}

			cr_sex sInterfaceRef = s[1];
			cr_sex sMethodRef = s[2];
			cr_sex sArgRef = s[3];

			if (!IsAtomic(sInterfaceRef))
			{
				Throw(sInterfaceRef, "The interface variable must be atomic");
			}

			if (!IsAtomic(sArgRef))
			{
				Throw(sArgRef, "The arg variable must be atomic");
			}

			if (!IsAtomic(sMethodRef) && !IsStringLiteral(sMethodRef))
			{
				Throw(sMethodRef, "The method ref variable must be an IString or string literal, or have an IString as base interface");
			}

			cstr interfaceRef = sInterfaceRef.c_str();
			cstr methodRef = sMethodRef.c_str();
			cstr argRef = sArgRef.c_str();

			MemberDef argDef;
			if (!ce.Builder.TryGetVariableByName(argDef, argRef))
			{
				Throw(sArgRef, "Could not determine type of %s", argRef);
			}

			if (IsPrimitiveType(argDef.ResolvedType->VarType()) || argDef.ResolvedType->InterfaceCount() > 0)
			{
				Throw(sArgRef, "Only structs may be used as args in invocations.");
			}

			MemberDef interfaceDef;
			if (!ce.Builder.TryGetVariableByName(interfaceDef, interfaceRef))
			{
				Throw(sInterfaceRef, "Could not determine type of %s", interfaceRef);
			}

			auto* type = interfaceDef.ResolvedType;
			if (type->InterfaceCount() != 1 || !IsNullType(*type))
			{
				Throw(sInterfaceRef, "Expecting a reference to a class with one interface, but type was %s", GetFriendlyName(*type));
			}

			const void* whatever;
			if (!type->GetInterface(0).Attributes().FindAttribute("dispatch", whatever))
			{
				Throw(sInterfaceRef, "Cannot invoke unless the interface is marked with (attribute dispatch). %s was not so marked", type->GetInterface(0).Name());
			}

			enum {BLACK_MAGIC = -1};
			AddSymbol(ce.Builder, "%s", argRef);
			ce.Builder.PushVariableRef(argRef, BLACK_MAGIC);

			AddSymbol(ce.Builder, "%s", interfaceRef);
			ce.Builder.PushVariable(interfaceDef);

			auto& argType = *argDef.ResolvedType;
			VariantValue vArgType;
			vArgType.sizetValue = (size_t) &argType;
			ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D4, vArgType, BITCOUNT_POINTER);
			AddSymbol(ce.Builder, "typeof(%s) = %s", argRef, GetFriendlyName(argType));
			ce.Builder.Assembler().Append_PushRegister(VM::REGISTER_D4, BITCOUNT_POINTER);

			if (IsStringLiteral(sMethodRef))
			{
				CStringConstant* sc = ce.SS.GetStringReflection(methodRef);
				InterfacePointer ip = sc->header.AddressOfVTable0();
				VariantValue v;
				v.sizetValue = (size_t)ip;
				ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D4, v, BITCOUNT_POINTER);
				AddSymbol(ce.Builder, "%32.32s", methodRef);
				ce.Builder.Assembler().Append_PushRegister(VM::REGISTER_D4, BITCOUNT_POINTER);
			}
			else
			{
				MemberDef methodDef;
				if (!ce.Builder.TryGetVariableByName(methodDef, methodRef))
				{
					Throw(sMethodRef, "Could not determine type of %s", methodRef);
				}

				if (!IsNullType(*methodDef.ResolvedType) || methodDef.ResolvedType->InterfaceCount() == 0)
				{
					Throw(sMethodRef, "%s did not have an IString as base interface", GetFriendlyName(*methodDef.ResolvedType));
				}

				auto& methodInterf0 = methodDef.ResolvedType->GetInterface(0);
				if (!Rococo::Compiler::IsDerivedFrom(methodInterf0, ce.Object.Common().SysTypeIString()))
				{
					Throw(sMethodRef, "%s did not derive from IString", GetFriendlyName(*methodDef.ResolvedType));
				}

				AddSymbol(ce.Builder, "%s", methodRef);
				ce.Builder.PushVariable(methodDef);
			}

			ce.Builder.Assembler().Append_Invoke(ce.SS.GetScriptCallbacks().idInvokeMethodByName);
			ce.Builder.Assembler().Append_Pop(sizeof(size_t) * 2);
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
			cstr nodeName = nodeNameExpr.c_str();
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

				cstr arg = s[1].c_str();

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
				Throw(s, "The global variable type %s does not match the local variable type %s", GetTypeName(g->type), def.ResolvedType->Name());
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

		typedef void (*FN_COMPILE_CE_S)(CCompileEnvironment& ce, cr_sex s);

		TSexyStringMap<FN_COMPILE_CE_S>* functionMap = nullptr;

		void FreeFunctionMap()
		{
			functionMap->~TSexyStringMap<FN_COMPILE_CE_S>();
			Memory::FreeSexyMemory(functionMap, sizeof TSexyStringMap<FN_COMPILE_CE_S>);
		}

		bool TryCompileAsKeyword(CCompileEnvironment& ce, sexstring token, cr_sex s)
		{
			if (functionMap == nullptr)
			{
				auto& allocator = Memory::GetSexyAllocator();
				void* pMemory = allocator.Allocate(sizeof TSexyStringMap<FN_COMPILE_CE_S>);
				functionMap = new (pMemory) TSexyStringMap<FN_COMPILE_CE_S>();	
				allocator.AtRelease(FreeFunctionMap);
				auto& map = *functionMap;
				map["if"] = CompileIfThenElse;
				map["cast"] = CompileCast;
				map["for"] = CompileForLoop;
				map["while"] = CompileWhileLoop;
				map["break"] = CompileBreak;
				map["continue"] = CompileContinue;
				map["do"] = CompileDoWhile;
				map["foreach"] = CompileForEach;
				map["throw"] = CompileThrow;
				map["construct"] = CompileConstructInterfaceCall;
				map["try"] = CompileExceptionBlock;
				map["return"] = CompileReturnFromFunction;
				map["array"] = CompileArrayDeclaration;
				map["invoke"] = CompileInvoke;
				map["list"] = CompileListDeclaration;
				map["map"] = CompileMapDeclaration;
				map["node"] = CompleAsNodeDeclaration;
				map["yield"] = CompileAsYield;
				map["global"] = CompileAsGlobalAccess;
				map["debug"] = CompileTrip;
				map["serialize"] = CompileSerialize;
				map["reflect"] = CompileReflect;
			}

			auto i = functionMap->find(token->Buffer);
			if (i != functionMap->end())
			{
				auto CompileKeywordExpression = i->second;
				CompileKeywordExpression(ce, s);
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

				Throw(exceptionSource, "The RHS type '%s' does not implement interface %s", GetFriendlyName(src), outputInterface.Name());
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

			/*
			if (!StartsWith(lhsString, "this.") && lhs.location != VARLOCATION_TEMP && lhs.location != VARLOCATION_OUTPUT)
			{
				Throw(s, "Could not assign array. The target has to be an output or temporary/local variable");
			}
			*/

			//AddSymbol(ce.Builder, "%s -> D4", lhsString);
			ce.Builder.AssignVariableToTemp(lhsString, 0, 0); // D4 has the LHS reference

			//AddSymbol(ce.Builder, "%s -> D5", rhsString);
			ce.Builder.AssignVariableToTemp(rhsString, 1, 0); // D5 has the RHS reference

			AddSymbol(ce.Builder, "*D5 = *D4. No output");
			AppendInvoke(ce, GetArrayCallbacks(ce).ArrayAssign, s);
		
			UseStackFrameFor(ce.Builder, lhs);

			if (!lhs.IsContained || lhs.Usage == ARGUMENTUSAGE_BYVALUE)
			{
				ce.Builder.Assembler().Append_SetStackFrameValue(lhs.SFOffset + lhs.MemberOffset, VM::REGISTER_D5, BITCOUNT_POINTER);
			}
			else
			{
				ce.Builder.Assembler().Append_SetStackFramePtrFromD5(lhs.SFOffset, lhs.MemberOffset);
			}
			RestoreStackFrameFor(ce.Builder, lhs);
		}

		void AssignListToList(CCompileEnvironment& ce, cr_sex s, const MemberDef& lhs, cstr lhsString, cstr rhsString)
		{
			MemberDef rhs;
			if (!ce.Builder.TryGetVariableByName(OUT rhs, rhsString) || rhs.ResolvedType->VarType() != VARTYPE_List)
			{
				Throw(s, "Expecting list type %s", rhsString);
			}


			const IStructure& lhsType = GetListDef(ce, s, lhsString);
			const IStructure& rhsType = GetListDef(ce, s, rhsString);

			if (&lhsType != &rhsType)
			{
				Throw(s, "Could not assign list. LHS is (list with type %s). RHS is (list with type %s)", GetFriendlyName(lhsType), GetFriendlyName(rhsType));
			}

			ce.Builder.AssignVariableToTemp(lhsString, 0, 0); // D4 has the LHS reference
			ce.Builder.AssignVariableToTemp(rhsString, 1, 0); // D5 has the RHS reference

			AddSymbol(ce.Builder, "*D5 = *D4. No output");
			AppendInvoke(ce, GetListCallbacks(ce).ListAssign, s);

			UseStackFrameFor(ce.Builder, lhs);

			if (!lhs.IsContained || lhs.Usage == ARGUMENTUSAGE_BYVALUE)
			{
				ce.Builder.Assembler().Append_SetStackFrameValue(lhs.SFOffset + lhs.MemberOffset, VM::REGISTER_D5, BITCOUNT_POINTER);
			}
			else
			{
				ce.Builder.Assembler().Append_SetStackFramePtrFromD5(lhs.SFOffset, lhs.MemberOffset);
			}
			RestoreStackFrameFor(ce.Builder, lhs);
		}

		void AssignMapToMap(CCompileEnvironment& ce, cr_sex s, const MemberDef& lhs, cstr lhsString, cstr rhsString)
		{
			MemberDef rhs;
			if (!ce.Builder.TryGetVariableByName(OUT rhs, rhsString) || rhs.ResolvedType->VarType() != VARTYPE_Map)
			{
				Throw(s, "Expecting map type %s", rhsString);
			}

			const IStructure& lhsKeyType = GetKeyTypeForMapVariable(ce, s, lhsString);
			const IStructure& rhsKeyType = GetKeyTypeForMapVariable(ce, s, rhsString);

			const IStructure& lhsValueType = GetValueTypeForMapVariable(ce, s, lhsString);
			const IStructure& rhsValueType = GetValueTypeForMapVariable(ce, s, rhsString);

			if (&lhsKeyType != &rhsKeyType)
			{
				Throw(s, "Could not assign map. LHS is (map with key type %s). RHS is (map with key type %s)", GetFriendlyName(lhsKeyType), GetFriendlyName(rhsKeyType));
			}

			if (&lhsValueType != &rhsValueType)
			{
				Throw(s, "Could not assign map. LHS is (map with value type %s). RHS is (map with value type %s)", GetFriendlyName(lhsValueType), GetFriendlyName(rhsValueType));
			}

			ce.Builder.AssignVariableToTemp(lhsString, 0, 0); // D4 has the LHS reference
			ce.Builder.AssignVariableToTemp(rhsString, 1, 0); // D5 has the RHS reference

			AddSymbol(ce.Builder, "*D5 = *D4. No output");
			AppendInvoke(ce, GetMapCallbacks(ce).MapAssign, s);

			UseStackFrameFor(ce.Builder, lhs);

			if (!lhs.IsContained || lhs.Usage == ARGUMENTUSAGE_BYVALUE)
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
				else if (def.ResolvedType == &ce.StructMap())
				{
					AssignMapToMap(ce, exceptionSource, def, lhs, rhs);
				}
				else if (def.ResolvedType == &ce.StructList())
				{
					AssignListToList(ce, exceptionSource, def, lhs, rhs);
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

			cstr name = lhs.c_str();
			cstr op = ops.c_str();

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

			cstr src = rhs.c_str();

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

					if (AreEqual(middleToken, "="))
					{
						if (IsStringLiteral(rhs))
						{
							CompileTrivialStringAssign(ce, s, lhsToken, rhsToken);
							return true;
						}

						if (IsValidVariableName(lhsToken->Buffer) && IsValidVariableName(rhsToken->Buffer))
						{
							if (AreEqual(rhsToken, "true") || AreEqual(rhsToken, "false"))
							{
								return false;
							}

							MemberDef targetDef;
							if (!ce.Builder.TryGetVariableByName(targetDef, lhsToken->Buffer))
							{
								Throw(lhs, "Unrecognized variable name");
							}

							if (targetDef.CapturesLocalVariables)
							{
								Throw(lhs, "The target variable refers to a closure. It is immutable.");
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
				if (macro == NULL) Throw(s, "Could not find macro %s in namespace %s. The macro was used in function %s defined in %s. Try using a fully qualified name.", macroName, nsBody, ce.Builder.Owner().Name(), ce.Builder.Module().Name());
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
					Throw(s, "Could not find macro %s used in function %s defined in %s. Try using a fully qualified name.", macroName, ce.Builder.Owner().Name(), ce.Builder.Module().Name());
				}
			}

			CallMacro(ce.SS, *f, s);

			return true;
		}

		void CompileCommand(CCompileEnvironment& ce, cr_sex s, sexstring token)
		{
			if (TryCompileMacroInvocation(ce, s, token))
			{
				const ISExpression* t = ce.SS.GetTransform(s);
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
		
			const IStructure* varStruct = ce.Builder.GetVarStructure(token->Buffer);
			if (NULL == varStruct)
			{
				Throw(s, "Unrecognized keyword/variable/function/namespace/syntax in expression: %s",token->Buffer);
			}
			else
			{
				Throw(s, "Variable recognized, but the syntax in which it is used was not: %s %s", GetFriendlyName(*varStruct), token->Buffer);
			}
		}

		void StreamSTCEX(StringBuilder& streamer, const STCException& ex)
		{
			streamer << ("Compiler exception code: ") << ex.Code() << (".\n") << ("Source: ") << ex.Source() << ("\n. Message: ") << ex.Message();
		}

		void CompileTransformableExpressionSequence(CCompileEnvironment& ce, int start, cr_sex sequence);

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
					CompileTransformableExpressionSequence(ce, 0, s);
				}
			}
			catch (STCException& ex)
			{
				char buf[256];
				StackStringBuilder ssb(buf, sizeof buf);
				StreamSTCEX(ssb, ex);
				Throw(s, "%s", buf);
			}
		}
	}
}

