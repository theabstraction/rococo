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

using namespace Rococo::Variants;

namespace Rococo
{
   namespace Variants
   {
      bool IsAGreaterThanB(const VariantValue& a, const VariantValue& b, VARTYPE type, cr_sex src)
      {
         switch (type)
         {
         case VARTYPE_Derivative:
            Throw(src, ("Cannot compare two derivative types, No 'greater than' operator defined"));
         case VARTYPE_Int32:
            return a.int32Value > b.int32Value;
         case VARTYPE_Int64:
            return a.int64Value > b.int64Value;
         case VARTYPE_Float32:
            return a.floatValue > b.floatValue;
         case VARTYPE_Float64:
            return a.doubleValue > b.doubleValue;
         default:
            Throw(src, ("Cannot compare two values, they are of unknown type"));
            return false;
         }
      }

      bool IsALessThanB(const VariantValue& a, const VariantValue& b, VARTYPE type, cr_sex src)
      {
         switch (type)
         {
         case VARTYPE_Derivative:
            Throw(src, ("Cannot compare two derivative types, No 'less than' operator defined"));
         case VARTYPE_Int32:
            return a.int32Value < b.int32Value;
         case VARTYPE_Int64:
            return a.int64Value < b.int64Value;
         case VARTYPE_Float32:
            return a.floatValue < b.floatValue;
         case VARTYPE_Float64:
            return a.doubleValue < b.doubleValue;
         default:
            Throw(src, ("Cannot compare two values, they are of unknown type"));
            return false;
         }
      }

      bool IsAGreaterThanOrEqualToB(const VariantValue& a, const VariantValue& b, VARTYPE type, cr_sex src)
      {
         switch (type)
         {
         case VARTYPE_Derivative:
            Throw(src, ("Cannot compare two derivative types, No 'greater than or equal to' operator defined"));
         case VARTYPE_Int32:
            return a.int32Value >= b.int32Value;
         case VARTYPE_Int64:
            return a.int64Value >= b.int64Value;
         case VARTYPE_Float32:
            return a.floatValue >= b.floatValue;
         case VARTYPE_Float64:
            return a.doubleValue >= b.doubleValue;
         default:
            Throw(src, ("Cannot compare two values, they are of unknown type"));
            return false;
         }
      }

      bool IsALessThanOrEqualToB(const VariantValue& a, const VariantValue& b, VARTYPE type, cr_sex src)
      {
         switch (type)
         {
         case VARTYPE_Derivative:
            Throw(src, ("Cannot compare two derivative types, No 'less than or equal to' operator defined"));
         case VARTYPE_Int32:
            return a.int32Value <= b.int32Value;
         case VARTYPE_Int64:
            return a.int64Value <= b.int64Value;
         case VARTYPE_Float32:
            return a.floatValue <= b.floatValue;
         case VARTYPE_Float64:
            return a.doubleValue <= b.doubleValue;
         default:
            Throw(src, ("Cannot compare two values, they are of unknown type"));
            return false;
         }
      }

      bool IsANotEqualToB(const VariantValue& a, const VariantValue& b, VARTYPE type, cr_sex src)
      {
         switch (type)
         {
         case VARTYPE_Derivative:
            Throw(src, ("Cannot compare two derivative types, No 'not equal to' operator defined"));
         case VARTYPE_Int32:
            return a.int32Value != b.int32Value;
         case VARTYPE_Int64:
            return a.int64Value != b.int64Value;
         case VARTYPE_Float32:
            return a.floatValue != b.floatValue;
         case VARTYPE_Float64:
            return a.doubleValue != b.doubleValue;
         default:
            Throw(src, ("Cannot compare two values, they are of unknown type"));
            return false;
         }
      }

      bool IsAEqualToB(const VariantValue& a, const VariantValue& b, VARTYPE type, cr_sex src)
      {
         switch (type)
         {
         case VARTYPE_Derivative:
            Throw(src, ("Cannot compare two derivative types, No 'not equal to' operator defined"));
         case VARTYPE_Int32:
            return a.int32Value == b.int32Value;
         case VARTYPE_Int64:
            return a.int64Value == b.int64Value;
         case VARTYPE_Float32:
            return a.floatValue == b.floatValue;
         case VARTYPE_Float64:
            return a.doubleValue == b.doubleValue;
         default:
            Throw(src, ("Cannot compare two values, they are of unknown type"));
            return false;
         }
      }
   }
}

namespace Rococo
{
   namespace Script
   {
      using namespace Rococo::Variants;

      bool Compare(const VariantValue& a, const VariantValue& b, VARTYPE type, CONDITION op, cr_sex src)
      {
         switch (op)
         {
         case CONDITION_IF_GREATER_THAN:			return IsAGreaterThanB(a, b, type, src);
         case CONDITION_IF_LESS_THAN:			   return IsALessThanB(a, b, type, src);
         case CONDITION_IF_GREATER_OR_EQUAL:		return IsAGreaterThanOrEqualToB(a, b, type, src);
         case CONDITION_IF_LESS_OR_EQUAL:		   return IsALessThanOrEqualToB(a, b, type, src);
         case CONDITION_IF_NOT_EQUAL:			   return IsANotEqualToB(a, b, type, src);
         case CONDITION_IF_EQUAL:				   return IsAEqualToB(a, b, type, src);
         default:
            Throw(src, ("Expecting binary boolean operator"));
            return false;
         }
      }

      void CompileBinaryCompareLiteralVsLiteral(CCompileEnvironment& ce, cr_sex parent, cstr leftString, VARTYPE lType, CONDITION op, cstr rightString, VARTYPE rType)
      {
         VariantValue lValue;
         if (Parse::PARSERESULT_GOOD != Parse::TryParse(OUT lValue, lType, leftString))
         {
            sexstringstream<1024> streamer;
            streamer.sb << ("Cannot parse the left part of the expression: ") << leftString;
            Throw(parent, streamer);
         }

         VariantValue rValue;
         if (Parse::PARSERESULT_GOOD != Parse::TryParse(OUT rValue, rType, rightString))
         {
            sexstringstream<1024> streamer;
            streamer.sb << ("Cannot parse the right part of the expression: ") << rightString;
            Throw(parent, streamer);
         }

         VariantValue newLValue, newRValue;
         VARTYPE bestCastType = Variants::GetBestCastType(lType, rType);
         if (Variants::TryRecast(newLValue, lValue, lType, bestCastType))
         {
            if (Variants::TryRecast(newRValue, rValue, rType, bestCastType))
            {
               bool match = Compare(newLValue, newRValue, bestCastType, op, parent);
               VariantValue val;
               val.int32Value = match ? 1 : 0;
               ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D7, val, BITCOUNT_32);
               return;
            }
         }

         Throw(parent, ("Not implemented"));
      }

      enum LOGICAL_OP
      {
         LOGICAL_OP_AND,
         LOGICAL_OP_OR,
         LOGICAL_OP_XOR
      };

      bool Compare(int a, int b, LOGICAL_OP op, cr_sex src)
      {
         switch (op)
         {
         case LOGICAL_OP_AND:		return a != 0 && b != 0;
         case LOGICAL_OP_OR:		return a != 0 || b != 0;
         case LOGICAL_OP_XOR:		return (a != 0 && b == 0) || (a == 0 && b != 0);

         default:
            Throw(src, ("Expecting binary boolean operator"));
            return false;
         }
      }

      void CompileBinaryBooleanLiteralVsLiteral(CCompileEnvironment& ce, cr_sex parent, int32 leftValue, LOGICAL_OP op, int32 rightValue)
      {
         bool match = Compare(leftValue, rightValue, op, parent);
         VariantValue val;
         val.int32Value = match ? 1 : 0;
         ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D7, val, BITCOUNT_32);
      }

      VARTYPE GetAtomicValueAnyNumeric(CCompileEnvironment& ce, cr_sex s, cstr id, int tempdepth)
      {
         VARTYPE type = VARTYPE_Bad;

         if (IsCapital(id[0]))
         {
            IFunctionBuilder& f = MustMatchFunction(ce.Builder.Module(), s, id);
            if (!IsGetAccessor(f) || !IsNumericTypeOrBoolean(f.GetArgument(0).VarType()))
            {
               sexstringstream<1024> streamer;
               streamer.sb << ("Expecting variable or single valued function with no inputs, and return type ") << Parse::VarTypeName(type);
               Throw(s, streamer);
            }

            type = f.GetArgument(0).VarType();
            CompileFunctionCallAndReturnValue(ce, s, f, type, NULL, NULL);

            if (tempdepth > Rococo::ROOT_TEMPDEPTH)
            {
               ce.Builder.Assembler().Append_MoveRegister(VM::REGISTER_D7, VM::REGISTER_D4 + tempdepth, GetBitCount(type));
            }
         }
         else if (Eq(id, "true") || Eq(id, "false"))
         {
             return VARTYPE_Bool;
         }
         else if (isdigit(id[0]))
         {
            VariantValue value;
            if (Parse::PARSERESULT_GOOD == Parse::TryParseDecimal(value.int32Value, id))
            {
               ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D7, value, BITCOUNT_32);
               return VARTYPE_Int32;
            }

            return VARTYPE_Bad;
         }
         else
         {
            NamespaceSplitter splitter(id);
            cstr instance, item;
            if (splitter.SplitTail(instance, item))
            {
               if (IsCapital(item[0]))
               {
                  type = CompileMethodCallWithoutInputAndReturnNumericValue(ce, s, instance, item);
                  if (!IsNumericTypeOrBoolean(type))
                  {
                     sexstringstream<1024> streamer;
                     streamer.sb << ("Expecting method returning ") << Parse::VarTypeName(type);
                     Throw(s, streamer);
                  }

                  if (tempdepth > Rococo::ROOT_TEMPDEPTH)
                  {
                     ce.Builder.Assembler().Append_MoveRegister(VM::REGISTER_D7, VM::REGISTER_D4 + tempdepth, GetBitCount(type));
                  }
               }
               else
               {
                  MemberDef def;
                  if (ce.Builder.TryGetVariableByName(OUT def, id))
                  {
                     ce.Builder.AssignVariableToTemp(id, tempdepth);
                     type = def.ResolvedType->VarType();
                  }
                  else
                  {
                     ThrowTokenNotFound(s, id, ce.Builder.Owner().Name(), "variable");
                  }
               }
            }
            else
            {
               const IStructure* st = ce.Builder.GetVarStructure(id);
               if (st != NULL)
               {
                  ce.Builder.AssignVariableToTemp(id, tempdepth);
                  type = st->VarType();
               }
               else
               {
                  ThrowTokenNotFound(s, id, ce.Builder.Owner().Name(), ("variable"));
               }
            }
         }

         return type;
      }

      bool IsZero(VariantValue value, VARTYPE type)
      {
         bool isZero = false;

         BITCOUNT bits = GetBitCount(type);
         if (bits == BITCOUNT_32)
         {
            return (value.int32Value == 0);
         }
         else
         {
            return (value.int64Value == 0);
         }
      }

      void AddBinaryComparison(cr_sex src, VM::IAssembler& assembler, int booleanTargetId, int sourceA, int sourceB, CONDITION op, VARTYPE type)
      {
         switch (type)
         {
         case VARTYPE_Float32:
            assembler.Append_FloatSubtract(VM::REGISTER_D4 + sourceA, VM::REGISTER_D4 + sourceB, VM::FLOATSPEC_SINGLE);
            break;
         case VARTYPE_Float64:
            assembler.Append_FloatSubtract(VM::REGISTER_D4 + sourceA, VM::REGISTER_D4 + sourceB, VM::FLOATSPEC_DOUBLE);
            break;
         case VARTYPE_Int32:
            assembler.Append_IntSubtract(VM::REGISTER_D4 + sourceA, BITCOUNT_32, VM::REGISTER_D4 + sourceB);
            break;
         case VARTYPE_Int64:
            assembler.Append_IntSubtract(VM::REGISTER_D4 + sourceA, BITCOUNT_64, VM::REGISTER_D4 + sourceB);
            break;
         default:
            Throw(src, ("Cannot find subtraction rule for the given type"));
         }

         assembler.Append_SetIf(op, VM::REGISTER_D4 + booleanTargetId, GetBitCount(type));
      }


      void CompileBinaryCompareVariableVsLiteral(CCompileEnvironment& ce, cr_sex parent, cstr leftVarName, CONDITION op, cstr rightString, VARTYPE rType, cr_sex leftVarExpr)
      {
         VariantValue rValue;
         if (Parse::PARSERESULT_GOOD != Parse::TryParse(OUT rValue, rType, rightString))
         {
            sexstringstream<1024> streamer;
            streamer.sb << ("Cannot parse the right part of the expression: ") << rightString;
            Throw(parent, streamer);
         }

         ICodeBuilder& builder = ce.Builder;

         if (IsZero(rValue, rType))
         {
            VARTYPE varLType = GetAtomicValueAnyNumeric(ce, leftVarExpr, leftVarName, Rococo::ROOT_TEMPDEPTH);

            VariantValue newRValue;
            if (varLType != VARTYPE_Pointer && !Variants::TryRecast(OUT newRValue, IN rValue, rType, varLType))
            {
               Throw(parent, ("Cannot cast RHS to the type of the LHS"));
            }

            builder.Assembler().Append_SetIf(op, VM::REGISTER_D7, GetBitCount(rType));
         }
         else
         {
            VARTYPE varLType = GetAtomicValueAnyNumeric(ce, leftVarExpr, leftVarName, Rococo::ROOT_TEMPDEPTH + 1);

            VariantValue newRValue;
            if (!Variants::TryRecast(OUT newRValue, IN rValue, rType, varLType))
            {
               Throw(parent, ("Cannot cast RHS to the type of the LHS"));
            }

            builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D7 + 2, newRValue, GetBitCount(varLType));
            AddBinaryComparison(parent, builder.Assembler(), Rococo::ROOT_TEMPDEPTH, Rococo::ROOT_TEMPDEPTH + 1, Rococo::ROOT_TEMPDEPTH + 2, op, varLType);
         }
      }

      void AddBinaryBoolean(cr_sex src, VM::IAssembler& assembler, int booleanTargetId, int sourceA, int sourceB, LOGICAL_OP op)
      {
         if (booleanTargetId != sourceA - 1)
         {
            Throw(src, ("Algorthimic error: Expecting [booleanTargetId] to be [sourceA-1]"));
         }

         switch (op)
         {
         case LOGICAL_OP_AND:
            assembler.Append_LogicalAND(VM::REGISTER_D4 + sourceA, BITCOUNT_32, VM::REGISTER_D4 + sourceB);
            break;
         case LOGICAL_OP_OR:
            assembler.Append_LogicalOR(VM::REGISTER_D4 + sourceA, BITCOUNT_32, VM::REGISTER_D4 + sourceB);
            break;
         case LOGICAL_OP_XOR:
            assembler.Append_LogicalXOR(VM::REGISTER_D4 + sourceA, BITCOUNT_32, VM::REGISTER_D4 + sourceB);
            break;
         }
      }

      void CompileBinaryBooleanLiteralVsVariable(CCompileEnvironment& ce, cr_sex parent, int literalValue, LOGICAL_OP op, cstr variableName)
      {
         VARTYPE varType = ce.Builder.GetVarType(variableName);
         if (varType == VARTYPE_Bad)
         {
            Throw(parent, ("Cannot resolve variable as a literal or identifier"));
         }
         else if (varType == VARTYPE_Derivative)
         {
            Throw(parent, ("Cannot compare derived types"));
         }
         else if (varType != VARTYPE_Bool)
         {
            Throw(parent, ("Cannot logically implicitly cast variable to a boolean"));
         }

         VariantValue lValue;
         lValue.int32Value = literalValue;

         ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D7 + 1, lValue, BITCOUNT_32);
         ce.Builder.AssignVariableToTemp(variableName, Rococo::ROOT_TEMPDEPTH + 2);
         AddBinaryBoolean(parent, ce.Builder.Assembler(), Rococo::ROOT_TEMPDEPTH, Rococo::ROOT_TEMPDEPTH + 1, Rococo::ROOT_TEMPDEPTH + 2, op);

      }

      void CompileBinaryCompareLiteralVsVariable(CCompileEnvironment& ce, cr_sex parent, cstr leftString, VARTYPE lType, CONDITION op, cstr rightVarName)
      {
         VariantValue lValue;
         if (Parse::PARSERESULT_GOOD != Parse::TryParse(OUT lValue, lType, leftString))
         {
            sexstringstream<1024> streamer;
            streamer.sb << ("Cannot parse the left part of the expression: ") << leftString;
            Throw(parent, streamer);
         }

         VARTYPE varRType = ce.Builder.GetVarType(rightVarName);
         if (varRType == VARTYPE_Bad)
         {
            Throw(parent, ("Cannot resolve right hand side as a literal or identifier"));
         }
         else if (varRType == VARTYPE_Derivative)
         {
            Throw(parent, ("Cannot compare derived types"));
         }

         VariantValue newLValue;
         if (!Variants::TryRecast(OUT newLValue, IN lValue, lType, varRType))
         {
            Throw(parent, ("Cannot cast LHS to the type of the RHS"));
         }

         ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D7 + 1, newLValue, GetBitCount(varRType));
         ce.Builder.AssignVariableToTemp(rightVarName, Rococo::ROOT_TEMPDEPTH + 2);
         AddBinaryComparison(parent, ce.Builder.Assembler(), Rococo::ROOT_TEMPDEPTH, Rococo::ROOT_TEMPDEPTH + 1, Rococo::ROOT_TEMPDEPTH + 2, op, varRType);
      }

      void CompileBinaryCompareAtomicVsCompound(CCompileEnvironment& ce, cr_sex parent, cstr varName, CONDITION op, cr_sex s, bool leftToRight);

	  void CompareObjects(CCompileEnvironment& ce, cr_sex parent, cr_sex leftExpr, cstr leftVarName, CONDITION op, cr_sex rightExpr, const MemberDef& leftDef, const MemberDef& rightDef)
	  {
		  if (leftDef.ResolvedType->InterfaceCount() == 0 || rightDef.ResolvedType->InterfaceCount() == 0)
		  {
			  Throw(parent, "Cannot compare %s to %s", GetFriendlyName(*leftDef.ResolvedType), GetFriendlyName(*rightDef.ResolvedType));
		  }

		  ce.Builder.PushVariable(leftDef);
		  ce.Builder.PushVariable(rightDef);

		  switch (op)
		  {
		  case CONDITION_IF_EQUAL:
			  ce.Builder.Assembler().Append_Invoke(ce.SS.GetScriptCallbacks().idIsSameObject); // Returns boolean in D7
			  break;
		  case CONDITION_IF_NOT_EQUAL:
			  ce.Builder.Assembler().Append_Invoke(ce.SS.GetScriptCallbacks().idIsDifferentObject); // Returns boolean in D7
			  break;
		  default:
			  Throw(parent, "Only == and != supported for interface comparison");
			  break;
		  }
	  }

	  bool TryCompileAsCompareStruct(CCompileEnvironment& ce, cr_sex parent, cr_sex leftExpr, cstr leftVarName, CONDITION op, cr_sex rightExpr)
	  {
		  MemberDef leftDef;
		  if (ce.Builder.TryGetVariableByName(leftDef, leftVarName))
		  {
			  cstr rightVarName = rightExpr.String()->Buffer;

			  MemberDef rightDef;
			  if (ce.Builder.TryGetVariableByName(rightDef, rightVarName))
			  {
				  if (!leftDef.ResolvedType || !rightDef.ResolvedType) return false;

				  if (leftDef.ResolvedType->VarType() == VARTYPE_Derivative && rightDef.ResolvedType->VarType() == VARTYPE_Derivative)
				  {
					  if (leftDef.ResolvedType->InterfaceCount() != 0 || rightDef.ResolvedType->InterfaceCount() != 0)
					  {
						  CompareObjects(ce, parent, leftExpr, leftVarName, op, rightExpr, leftDef, rightDef);
						  return true;
					  }

					  cstr sop;

					  switch (op)
					  {
					  case CONDITION_IF_EQUAL:
						  sop = "Eq";
						  break;
					  case CONDITION_IF_NOT_EQUAL:
						  sop = "NotEq";
						  break;
					  case CONDITION_IF_GREATER_THAN:
						  sop = "GT";
						  break;
					  case CONDITION_IF_GREATER_OR_EQUAL:
						  sop = "GTE";
						  break;
					  case CONDITION_IF_LESS_THAN:
						  sop = "LT";
						  break;
					  case CONDITION_IF_LESS_OR_EQUAL:
						  sop = "LTE";
						  break;
					  default:
						  Throw(parent, "Operator not supported");
					  }

					  char compareFunction[256];
					  SafeFormat(compareFunction, 256, "Is%s%s%s", sop, leftDef.ResolvedType->Name(), rightDef.ResolvedType->Name());

					  IFunctionBuilder& callee = MustMatchFunction(ce.Builder.Module(), parent, compareFunction);

					  if (callee.NumberOfInputs() != 2 || callee.NumberOfOutputs() != 1)
					  {
						  Throw(parent, "Operator overload function %s must have 2 inputs and 1 output", compareFunction);
					  }

					  if (callee.GetArgument(0).VarType() != VARTYPE_Bool)
					  {
						  Throw(parent, "Operator overload function %s must have Bool as output", compareFunction);
					  }

					  if (&callee.GetArgument(1) != leftDef.ResolvedType)
					  {
						  Throw(parent, "Operator overload function %s first argument must be of type %s of %s. Not %s of %s", compareFunction,
							  leftDef.ResolvedType->Name(), leftDef.ResolvedType->Module().Name(),
							  callee.GetArgument(1).Name(), callee.GetArgument(1).Module().Name());
					  }

					  if (&callee.GetArgument(2) != rightDef.ResolvedType)
					  {
						  Throw(parent, "Operator overload function %s second argument must be of type %s of %s. Not %s of %s", compareFunction,
							  rightDef.ResolvedType->Name(), rightDef.ResolvedType->Module().Name(),
							  callee.GetArgument(2).Name(), callee.GetArgument(2).Module().Name());
					  }
					  
					  const int outputStackAllocCount = AllocFunctionOutput(ce, callee, parent);

					  AddArgVariable("input_left", ce, *leftDef.ResolvedType);
					  ce.Builder.PushVariableRef(leftVarName, -1);

					  AddArgVariable("input_right", ce, *rightDef.ResolvedType);
					  ce.Builder.PushVariableRef(rightVarName, -1);

					  AppendFunctionCallAssembly(ce, callee);

					  ce.Builder.MarkExpression(&parent);
					  RepairStack(ce, parent, callee);

					  int outputOffset = GetOutputSFOffset(ce, sizeof(size_t) * 2, outputStackAllocCount);
					  ReturnOutput(ce, outputOffset, VARTYPE_Bool);

					  return true;
				  }
			  }
		  }

		  return false;
	  }

      void CompileBinaryCompareVariableVsVariable(CCompileEnvironment& ce, cr_sex parent, cr_sex leftExpr, cstr leftVarName, CONDITION op, cr_sex rightExpr)
      {
         ICodeBuilder& builder = ce.Builder;

		 if (TryCompileAsCompareStruct(ce, parent, leftExpr, leftVarName, op, rightExpr)) return;

         VARTYPE varLType = GetAtomicValueAnyNumeric(ce, leftExpr, leftVarName, Rococo::ROOT_TEMPDEPTH + 1);
         VARTYPE varRType = GetAtomicValueAnyNumeric(ce, rightExpr, rightExpr.String()->Buffer, Rococo::ROOT_TEMPDEPTH + 2);

         if (varLType != varRType)
         {
            Throw(parent, ("Cannot compare left with right, they are of different types"));
         }

         AddBinaryComparison(parent, builder.Assembler(), Rococo::ROOT_TEMPDEPTH, Rococo::ROOT_TEMPDEPTH + 1, Rococo::ROOT_TEMPDEPTH + 2, op, varLType);
      }

      void CompileBinaryBooleanVariableVsVariable(CCompileEnvironment& ce, cr_sex parent, cstr leftVarName, LOGICAL_OP op, cstr rightVarName)
      {
         VARTYPE varLType = ce.Builder.GetVarType(leftVarName);
         VARTYPE varRType = ce.Builder.GetVarType(rightVarName);

         if (varLType == VARTYPE_Bad)
         {
            Throw(parent, ("The LHS is neither a literal, nor an identifier"));
         }
         else if (varLType == VARTYPE_Derivative)
         {
            Throw(parent, ("The LHS is a derived type, and cannot be used in boolean comparisons"));
         }
         else if (varLType != VARTYPE_Bool)
         {
            Throw(parent, ("The LHS is not a boolean type"));
         }

         if (varRType == VARTYPE_Bad)
         {
            Throw(parent, ("The RHS is neither a literal, nor an identifier"));
         }
         else if (varRType == VARTYPE_Derivative)
         {
            Throw(parent, ("The RHS is a derived type, and cannot be used in boolean comparisons"));
         }
         else if (varRType != VARTYPE_Bool)
         {
            Throw(parent, ("The RHS is not a boolean type"));
         }

         ce.Builder.AssignVariableToTemp(leftVarName, Rococo::ROOT_TEMPDEPTH + 1);
         ce.Builder.AssignVariableToTemp(rightVarName, Rococo::ROOT_TEMPDEPTH + 2);
         AddBinaryBoolean(parent, ce.Builder.Assembler(), Rococo::ROOT_TEMPDEPTH, Rococo::ROOT_TEMPDEPTH + 1, Rococo::ROOT_TEMPDEPTH + 2, op);
      }

      void CompileBinaryCompareAtomicVsCompound(CCompileEnvironment& ce, cr_sex parent, cstr varName, CONDITION op, cr_sex s, bool leftToRight)
      {
         int A = leftToRight ? 1 : 2;
         int B = leftToRight ? 2 : 1;
         cstr helper = leftToRight ? ("LHS") : ("RHS");

         VARTYPE type = ce.Builder.GetVarType(varName);
         if (type == VARTYPE_Derivative)
         {
            sexstringstream<1024> streamer;
            streamer.sb << helper << (" was of derived type and cannot be directly used in a comparison expression");
            Throw(parent, streamer);
         }
         else if (type == VARTYPE_Bad)
         {
            type = Parse::GetLiteralType(varName);
            if (!IsPrimitiveType(type))
            {
               sexstringstream<1024> streamer;
               streamer.sb << helper << (" was not recognized either as a literal, or as an identifier");
               Throw(parent, streamer);
            }

            VariantValue value;
            if (Parse::TryParse(OUT value, type, varName) != Parse::PARSERESULT_GOOD)
            {
               sexstringstream<1024> streamer;
               streamer.sb << helper << (" was not parsed either as a literal, or as an identifier");
               Throw(parent, streamer);
            }

            ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D7 + A, value, GetBitCount(type));
         }
         else
         {
            ce.Builder.AssignVariableToTemp(varName, Rococo::ROOT_TEMPDEPTH + A);
         }

		 AddArchiveRegister(ce, Rococo::ROOT_TEMPDEPTH + A, Rococo::ROOT_TEMPDEPTH + A, GetBitCount(type));
		 TryCompileArithmeticExpression(ce, s, true, type);
		 ce.Builder.PopLastVariables(1,true);

         ce.Builder.Assembler().Append_MoveRegister(VM::REGISTER_D7, VM::REGISTER_D7 + B, GetBitCount(type));

         AddBinaryComparison(parent, ce.Builder.Assembler(), Rococo::ROOT_TEMPDEPTH, Rococo::ROOT_TEMPDEPTH + 1, Rococo::ROOT_TEMPDEPTH + 2, op, type);
      }

      // Compile code to evaluate a boolean and copy the value to the temp variable #tempDepth
      void CompileBinaryCompareExpression(CCompileEnvironment& ce, cr_sex parent, cr_sex left, CONDITION op, cr_sex right)
      {
         // Types: literal, variable, compound expression
         if (IsAtomic(left))
         {
            cstr leftString = left.String()->Buffer;
            VARTYPE lType = Parse::GetLiteralType(leftString);
            if (IsPrimitiveType(lType))
            {
               if (IsAtomic(right))
               {
                  cstr rightString = right.String()->Buffer;
                  VARTYPE rType = Parse::GetLiteralType(rightString);

                  if (IsPrimitiveType(rType))
                  {
                     CompileBinaryCompareLiteralVsLiteral(ce, parent, leftString, lType, op, rightString, rType);
                  }
                  else
                  {
                     CompileBinaryCompareLiteralVsVariable(ce, parent, leftString, lType, op, rightString);
                  }
               }
               else
               {
                  CompileBinaryCompareAtomicVsCompound(ce, parent, left.String()->Buffer, op, right, false);
               }
            }
            else
            {
               if (IsAtomic(right))
               {
                  cstr rightString = right.String()->Buffer;
                  VARTYPE rType = Parse::GetLiteralType(rightString);

                  if (IsPrimitiveType(rType))
                  {
                     CompileBinaryCompareVariableVsLiteral(ce, parent, leftString, op, rightString, rType, left);
                  }
                  else
                  {
                     CompileBinaryCompareVariableVsVariable(ce, parent, left, leftString, op, right);
                  }
               }
               else if (!IsCompound(right))
               {
                  Throw(parent, ("RHS neither atomic nor compound"));
               }
               else
               {
                  CompileBinaryCompareAtomicVsCompound(ce, parent, left.String()->Buffer, op, right, true);
               }
            }
         }
         else
         {
            if (IsAtomic(right))
            {
               CompileBinaryCompareAtomicVsCompound(ce, parent, right.String()->Buffer, op, left, false);
            }
            else if (!IsCompound(right))
            {
               Throw(parent, ("RHS is neither atomic nor compound"));
            }
            else
            {
               VARTYPE guessType = GuessType(parent, ce.Builder);
               if (IsPrimitiveType(guessType))
               {
                  BITCOUNT bits = GetBitCount(guessType);

                  TryCompileArithmeticExpression(ce, left, true, guessType);

                  AddArchiveRegister(ce, Rococo::ROOT_TEMPDEPTH, Rococo::ROOT_TEMPDEPTH + 1, bits);
                  TryCompileArithmeticExpression(ce, right, true, guessType);
                  ce.Builder.Assembler().Append_MoveRegister(VM::REGISTER_D7, VM::REGISTER_D7 + 2, GetBitCount(guessType));
                  ce.Builder.PopLastVariables(1,true);

                  AddBinaryComparison(parent, ce.Builder.Assembler(), Rococo::ROOT_TEMPDEPTH, Rococo::ROOT_TEMPDEPTH + 1, Rococo::ROOT_TEMPDEPTH + 2, op, guessType);
               }
               else
               {
                  Throw(parent, ("Cannot infer types in the comparison"));
               }
            }
         }
      }

      void CompileBinaryBooleanLiteralVsCompoundExpression(CCompileEnvironment& ce, cr_sex parent, int32 value, LOGICAL_OP op, cr_sex right)
      {
         bool invert = false;

         if (value == 0)
         {
            switch (op)
            {
            case LOGICAL_OP_AND:
               // (0 AND (x)) => (0)
               ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D7, Variants::ValueFalse(), BITCOUNT_32);
               return;
            case LOGICAL_OP_XOR:
            case LOGICAL_OP_OR:
               // (0 OR (x)) => (0 XOR (x)) => (x)
               break;
            }
         }
         else
         {
            switch (op)
            {
            case LOGICAL_OP_AND:
               // (1 AND (x)) => (x)
               break;
            case LOGICAL_OP_OR:
               // (1 OR (x) => (1)
               ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D7, Variants::ValueTrue(), BITCOUNT_32);
               return;
            case LOGICAL_OP_XOR:
               // (1 XOR (x)) => (NOT (x))
               invert = true;
               break;
            }
         }

         int offset = invert ? 2 : 0;

         int nElements = right.NumberOfElements();
         if (nElements == 3)
         {
            bool negate = false;
            if (!TryCompileBooleanExpression(ce, right, true, negate))
            {
               Throw(parent, ("Expecting a boolean valued expression on the RHS"));
            }

            if (invert)
            {
               ce.Builder.Assembler().Append_MoveRegister(VM::REGISTER_D4 + Rococo::ROOT_TEMPDEPTH, VM::REGISTER_D4 + Rococo::ROOT_TEMPDEPTH + offset, BITCOUNT_32);
            }

            if (negate) ce.Builder.Assembler().Append_BooleanNot(VM::REGISTER_D7);

            if (invert)
            {
               // (1 - truth_value) toggles the truth_value
               ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D7 + 1, Variants::FromValue(1), BITCOUNT_32);
               ce.Builder.Assembler().Append_IntSubtract(VM::REGISTER_D7 + 1, BITCOUNT_32, VM::REGISTER_D7 + offset);
            }
         }
         else
         {
            Throw(parent, ("Expecting a boolean valued expression on the RHS"));
         }
      }

      void AppendBooleanLogicalOp(cr_sex src, LOGICAL_OP op, VM::IAssembler& assember, int tempDepthA, int tempDepthB)
      {
         switch (op)
         {
         case LOGICAL_OP_AND:
            assember.Append_LogicalAND(VM::REGISTER_D4 + tempDepthA, BITCOUNT_32, VM::REGISTER_D4 + tempDepthB);
            break;
         case LOGICAL_OP_OR:
            assember.Append_LogicalOR(VM::REGISTER_D4 + tempDepthA, BITCOUNT_32, VM::REGISTER_D4 + tempDepthB);
            break;
         case LOGICAL_OP_XOR:
            assember.Append_LogicalXOR(VM::REGISTER_D4 + tempDepthA, BITCOUNT_32, VM::REGISTER_D4 + tempDepthB);
            break;
         default:
            Throw(src, ("Unhandled logical op type"));
         }
      }

      void CompileBinaryBooleanVariableVsCompoundExpression(CCompileEnvironment& ce, cr_sex parent, cstr leftString, LOGICAL_OP op, cr_sex right)
      {
         VARTYPE type = ce.Builder.GetVarType(leftString);
         if (type != VARTYPE_Bool)
         {
            Throw(parent, ("Expecting boolean variable on LHS"));
         }

         bool negate = false;
         if (TryCompileBooleanExpression(ce, right, true, negate))
         {
            if (negate) ce.Builder.Assembler().Append_BooleanNot(VM::REGISTER_D7);
            ce.Builder.AssignVariableToTemp(leftString, Rococo::ROOT_TEMPDEPTH + 1);
            ce.Builder.Assembler().Append_MoveRegister(VM::REGISTER_D7, VM::REGISTER_D7 + 2, GetBitCount(type));
            AppendBooleanLogicalOp(parent, op, ce.Builder.Assembler(), Rococo::ROOT_TEMPDEPTH + 1, Rococo::ROOT_TEMPDEPTH + 2);
            return;
         }
         else
         {
            Throw(parent, ("Cannot interpret RHS as a binary predicate expression"));
         }
      }

      void CompileBinaryBooleanCompoundVsCompound(CCompileEnvironment& ce, cr_sex parent, cr_sex left, LOGICAL_OP op, cr_sex right)
      {
         bool negate = false;
         if (!TryCompileBooleanExpression(ce, left, true, negate))
         {
            Throw(left, ("Cannot compile LHS as boolean binary predicate"));
         }

         if (negate) ce.Builder.Assembler().Append_BooleanNot(VM::REGISTER_D7);

         AddArchiveRegister(ce, Rococo::ROOT_TEMPDEPTH, Rococo::ROOT_TEMPDEPTH + 1, BITCOUNT_32);
         negate = false;
         if (!TryCompileBooleanExpression(ce, right, true, negate))
         {
            Throw(left, ("Cannot compile RHS as boolean binary predicate"));
         }

         if (negate) ce.Builder.Assembler().Append_BooleanNot(VM::REGISTER_D7);
         ce.Builder.PopLastVariables(1,true);

         ce.Builder.Assembler().Append_MoveRegister(VM::REGISTER_D7, VM::REGISTER_D7 + 2, BITCOUNT_32);

         AppendBooleanLogicalOp(parent, op, ce.Builder.Assembler(), Rococo::ROOT_TEMPDEPTH + 1, Rococo::ROOT_TEMPDEPTH + 2);
      }

      void CompileBinaryBooleanAtomicVsCompoundExpression(CCompileEnvironment& ce, cr_sex parent, cstr leftString, LOGICAL_OP op, cr_sex right)
      {
         VARTYPE lType = ce.Builder.GetVarType(leftString);
         if (lType == VARTYPE_Derivative)
         {
            Throw(parent, ("LHS was of a derived type, and cannot be directly used in boolean expressions"));
         }
         else if (lType == VARTYPE_Bad)
         {
            int32 value;
            if (Parse::TryParseBoolean(OUT value, leftString) != Parse::PARSERESULT_GOOD)
            {
               Throw(parent, ("In the boolean expression the LHS was neither an identifier nor a known boolean value"));
            }

            CompileBinaryBooleanLiteralVsCompoundExpression(ce, parent, value, op, right);
            return;
         }
         else if (lType != VARTYPE_Bool)
         {
            Throw(parent, ("Identifier in the LHS of the boolean expression was not of underlying type Int32"));
         }

         CompileBinaryBooleanVariableVsCompoundExpression(ce, parent, leftString, op, right);
      }

      // Compile code to evaluate a boolean and copy the value to the temp variable #tempDepth
      void CompileBinaryBooleanExpression(CCompileEnvironment& ce, cr_sex parent, cr_sex left, LOGICAL_OP op, cr_sex right)
      {
         // Types: literal, variable, compound expression
         if (IsAtomic(left))
         {
            cstr leftString = left.String()->Buffer;

            if (IsCompound(right))
            {
               CompileBinaryBooleanAtomicVsCompoundExpression(ce, parent, leftString, op, right);
               return;
            }
            if (!IsAtomic(right))
            {
               Throw(parent, ("The RHS in the boolean expression is neither an atomic nor compound expression"));
            }

            cstr rightString = right.String()->Buffer;

            int32 lValue;
            if (Parse::TryParseBoolean(OUT lValue, leftString) == Parse::PARSERESULT_GOOD)
            {
               int32 rValue;
               if (Parse::TryParseBoolean(OUT rValue, rightString) == Parse::PARSERESULT_GOOD)
               {
                  CompileBinaryBooleanLiteralVsLiteral(ce, parent, lValue, op, rValue);
               }
               else
               {
                  CompileBinaryBooleanLiteralVsVariable(ce, parent, lValue, op, rightString);
               }
            }
            else
            {
               int32 rValue;
               if (Parse::TryParseBoolean(OUT rValue, IN rightString) == Parse::PARSERESULT_GOOD)
               {
                  CompileBinaryBooleanLiteralVsVariable(ce, parent, rValue, op, leftString);
               }
               else
               {
                  CompileBinaryBooleanVariableVsVariable(ce, parent, leftString, op, rightString);
               }
            }

            return;
         }
         else if (!IsCompound(left))
         {
            Throw(parent, ("LHS neither atomic nor compound type"));
         }

         if (IsAtomic(right))
         {
            CompileBinaryBooleanAtomicVsCompoundExpression(ce, parent, right.String()->Buffer, op, left);
         }
         else
         {
            CompileBinaryBooleanCompoundVsCompound(ce, parent, left, op, right);
         }
      }

      const IFunction* TryGetMethod(const MemberDef& def, cstr methodName)
      {
         if (Rococo::IsCapital(methodName[0]))
         {
            TokenBuffer localMethodName;
            StringPrint(localMethodName, ("%s.%s"), def.ResolvedType->Name(), methodName);
            return def.ResolvedType->Module().FindFunction(localMethodName);
         }
         else
         {
            return NULL;
         }
      }

      bool TryCompileBooleanValuedFunction(CCompileEnvironment& ce, cr_sex src, bool expected)
      {
         return TryCompileFunctionCallAndReturnValue(ce, src, VARTYPE_Bool, NULL, NULL);
      }

      CONDITION GetBinaryComparisonOp(cr_sex opExpr, bool negate)
      {
         sexstring op = opExpr.String();
         if (AreEqual(op, (">"))) return negate ? CONDITION_IF_LESS_OR_EQUAL : CONDITION_IF_GREATER_THAN;
         if (AreEqual(op, ("<")))	return negate ? CONDITION_IF_GREATER_OR_EQUAL : CONDITION_IF_LESS_THAN;
         if (AreEqual(op, (">="))) return negate ? CONDITION_IF_LESS_THAN : CONDITION_IF_GREATER_OR_EQUAL;
         if (AreEqual(op, ("<="))) return negate ? CONDITION_IF_GREATER_THAN : CONDITION_IF_LESS_OR_EQUAL;
         if (AreEqual(op, ("!="))) return negate ? CONDITION_IF_EQUAL : CONDITION_IF_NOT_EQUAL;
         if (AreEqual(op, ("=="))) return negate ? CONDITION_IF_NOT_EQUAL : CONDITION_IF_EQUAL;

         Throw(opExpr, ("Cannot interpret as a comparison operator"));
         return CONDITION_IF_EQUAL;
      }

      LOGICAL_OP GetBinaryLogicalOp(cr_sex opExpr)
      {
         sexstring op = opExpr.String();
         if (AreEqual(op, ("and"))) return LOGICAL_OP_AND;
         if (AreEqual(op, ("or")))	return LOGICAL_OP_OR;
         if (AreEqual(op, ("xor"))) return LOGICAL_OP_XOR;

         Throw(opExpr, ("Cannot interpret as a binary logical operation"));
         return LOGICAL_OP_AND;
      }

      bool TryCompileBooleanExpression(CCompileEnvironment& ce, cr_sex s, bool expected, bool& negate)
      {
         if (IsCompound(s))
         {
            if (s.NumberOfElements() == 1)
            {
               cr_sex onlyChild = s.GetElement(0);
               return TryCompileBooleanExpression(ce, onlyChild, expected, negate);
            }
            if (s.NumberOfElements() == 2)
            {
               if (TryCompileBooleanValuedFunction(ce, s, expected))
               {
                  return true;
               }

               cr_sex notIndicator = s.GetElement(0);
               AssertAtomic(notIndicator);
               if (AreEqual(notIndicator.String(), ("not")))
               {
                  cr_sex onlyChild = s.GetElement(1);
                  negate = !negate;
                  return TryCompileBooleanExpression(ce, onlyChild, expected, negate);
               }
               else
               {
                  if (expected)
                  {
                     Throw(s, ("Expected 'not' symbol in first argument of a two element binary expression"));
                  }
                  return false;
               }
            }
            if (s.NumberOfElements() == 3)
            {
               cr_sex left = s.GetElement(0);
               cr_sex opExpr = s.GetElement(1);
               cr_sex right = s.GetElement(2);

               if (IsBinaryCompareOperator(opExpr))
               {
                  CONDITION op = GetBinaryComparisonOp(opExpr, negate);
                  // Add code to evaluate an expression and copy the result to the tempIndex
                  CompileBinaryCompareExpression(ce, s, left, op, right);

                  negate = false;
                  return true;
               }
               else if (IsBinaryBooleanOperator(opExpr))
               {
                  LOGICAL_OP op = GetBinaryLogicalOp(opExpr);
                  CompileBinaryBooleanExpression(ce, s, left, op, right);
                  return true;
               }
               else
               {
                  if (TryCompileBooleanValuedFunction(ce, s, expected))
                  {
                     return true;
                  }

                  if (expected)
                  {
                     Throw(s, ("Expected boolean expression, but could not see a binary predicate operator"));
                  }
                  // No binary predicate operator
                  return false;
               }
            }
            else
            {
                if (TryCompileBooleanValuedFunction(ce, s, expected))
                {
                    return true;
                }

                if (expected)
                {
                    sexstringstream<1024> streamer;
                    streamer.sb << ("Expected expression with 3 elements. This expression had ") << s.NumberOfElements() << (" elements.");
                    Throw(s, streamer);
                }
                // All binary predicate expressions have 3 elements
                return false;
            }
         }
         else if (IsAtomic(s))
         {
            cstr token = s.String()->Buffer;

            VariantValue value;
            if (Parse::TryParseBoolean(OUT value.int32Value, IN token) == Parse::PARSERESULT_GOOD)
            {
               if (negate) value.int32Value = !value.int32Value;
               ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D7, value, BITCOUNT_32);
               return true;
            }
            else
            {
               auto varType = ce.Builder.GetVarType(token);
               if (VARTYPE_Bool == varType)
               {
                  ce.Builder.AssignVariableToTemp(token, Rococo::ROOT_TEMPDEPTH);
                  return true;
               } 
               else if (varType == VARTYPE_Derivative)
               {
                   MemberDef def;
                   if (!ce.Builder.TryGetVariableByName(def, token))
                   {
                       if (expected) Throw(s, "Cannot interpret '%s' as a variable. Not implemented", token);
                       else return false;
                   }
        
                   if (Eq(def.ResolvedType->Name(), "_MapNode"))
                   {
                       // Node ref to D7
                       AddSymbol(ce.Builder, "%s", token);
                       ce.Builder.AssignVariableToTemp(token, Rococo::ROOT_TEMPDEPTH);
                       AppendInvoke(ce, GetMapCallbacks(ce).DoesMapNodeExist, s);
                       return true;
                   }

                   if (expected) Throw(s, "(%s %s) -> Bool. Not implemented", GetFriendlyName(*def.ResolvedType), token);
                   else return false;
               }
               else
               {
                  if (TryCompileBooleanValuedFunction(ce, s, expected))
                  {
                     return true;
                  }

                  if (expected)
                  {
                     Throw(s, ("Expected binary expression, but found an identifier/literal not of the boolean type"));
                  }
                  // not a boolean
                  return false;
               }
            }
         }
         else
         {
            if (expected)
            {
               Throw(s, ("Expected binary expression, but expression was neither atomic nor compound"));
            }
            // not an atomic or a compound
            return false;
         }
      }
   } // Script
}//Sexy