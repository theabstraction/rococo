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

using namespace Sexy::Variants;

namespace Sexy
{
   namespace Variants
   {
      bool IsAGreaterThanB(const VariantValue& a, const VariantValue& b, VARTYPE type, cr_sex src)
      {
         switch (type)
         {
         case VARTYPE_Derivative:
            Throw(src, SEXTEXT("Cannot compare two derivative types, No 'greater than' operator defined"));
         case VARTYPE_Int32:
            return a.int32Value > b.int32Value;
         case VARTYPE_Int64:
            return a.int64Value > b.int64Value;
         case VARTYPE_Float32:
            return a.floatValue > b.floatValue;
         case VARTYPE_Float64:
            return a.doubleValue > b.doubleValue;
         default:
            Throw(src, SEXTEXT("Cannot compare two values, they are of unknown type"));
            return false;
         }
      }

      bool IsALessThanB(const VariantValue& a, const VariantValue& b, VARTYPE type, cr_sex src)
      {
         switch (type)
         {
         case VARTYPE_Derivative:
            Throw(src, SEXTEXT("Cannot compare two derivative types, No 'less than' operator defined"));
         case VARTYPE_Int32:
            return a.int32Value < b.int32Value;
         case VARTYPE_Int64:
            return a.int64Value < b.int64Value;
         case VARTYPE_Float32:
            return a.floatValue < b.floatValue;
         case VARTYPE_Float64:
            return a.doubleValue < b.doubleValue;
         default:
            Throw(src, SEXTEXT("Cannot compare two values, they are of unknown type"));
            return false;
         }
      }

      bool IsAGreaterThanOrEqualToB(const VariantValue& a, const VariantValue& b, VARTYPE type, cr_sex src)
      {
         switch (type)
         {
         case VARTYPE_Derivative:
            Throw(src, SEXTEXT("Cannot compare two derivative types, No 'greater than or equal to' operator defined"));
         case VARTYPE_Int32:
            return a.int32Value >= b.int32Value;
         case VARTYPE_Int64:
            return a.int64Value >= b.int64Value;
         case VARTYPE_Float32:
            return a.floatValue >= b.floatValue;
         case VARTYPE_Float64:
            return a.doubleValue >= b.doubleValue;
         default:
            Throw(src, SEXTEXT("Cannot compare two values, they are of unknown type"));
            return false;
         }
      }

      bool IsALessThanOrEqualToB(const VariantValue& a, const VariantValue& b, VARTYPE type, cr_sex src)
      {
         switch (type)
         {
         case VARTYPE_Derivative:
            Throw(src, SEXTEXT("Cannot compare two derivative types, No 'less than or equal to' operator defined"));
         case VARTYPE_Int32:
            return a.int32Value <= b.int32Value;
         case VARTYPE_Int64:
            return a.int64Value <= b.int64Value;
         case VARTYPE_Float32:
            return a.floatValue <= b.floatValue;
         case VARTYPE_Float64:
            return a.doubleValue <= b.doubleValue;
         default:
            Throw(src, SEXTEXT("Cannot compare two values, they are of unknown type"));
            return false;
         }
      }

      bool IsANotEqualToB(const VariantValue& a, const VariantValue& b, VARTYPE type, cr_sex src)
      {
         switch (type)
         {
         case VARTYPE_Derivative:
            Throw(src, SEXTEXT("Cannot compare two derivative types, No 'not equal to' operator defined"));
         case VARTYPE_Int32:
            return a.int32Value != b.int32Value;
         case VARTYPE_Int64:
            return a.int64Value != b.int64Value;
         case VARTYPE_Float32:
            return a.floatValue != b.floatValue;
         case VARTYPE_Float64:
            return a.doubleValue != b.doubleValue;
         default:
            Throw(src, SEXTEXT("Cannot compare two values, they are of unknown type"));
            return false;
         }
      }

      bool IsAEqualToB(const VariantValue& a, const VariantValue& b, VARTYPE type, cr_sex src)
      {
         switch (type)
         {
         case VARTYPE_Derivative:
            Throw(src, SEXTEXT("Cannot compare two derivative types, No 'not equal to' operator defined"));
         case VARTYPE_Int32:
            return a.int32Value == b.int32Value;
         case VARTYPE_Int64:
            return a.int64Value == b.int64Value;
         case VARTYPE_Float32:
            return a.floatValue == b.floatValue;
         case VARTYPE_Float64:
            return a.doubleValue == b.doubleValue;
         default:
            Throw(src, SEXTEXT("Cannot compare two values, they are of unknown type"));
            return false;
         }
      }
   }
}

namespace
{
   using namespace Sexy::Variants;

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
         Throw(src, SEXTEXT("Expecting binary boolean operator"));
         return false;
      }
   }

	void CompileBinaryCompareLiteralVsLiteral(CCompileEnvironment& ce, cr_sex parent, csexstr leftString, VARTYPE lType, CONDITION op, csexstr rightString, VARTYPE rType)
	{
		VariantValue lValue;
		if (Parse::PARSERESULT_GOOD != Parse::TryParse(OUT lValue, lType, leftString))
		{
			sexstringstream streamer;
			streamer << SEXTEXT("Cannot parse the left part of the expression: ") << leftString;
			Throw(parent, streamer);
		}			

		VariantValue rValue;
		if (Parse::PARSERESULT_GOOD != Parse::TryParse(OUT rValue, rType, rightString))
		{
			sexstringstream streamer;
			streamer << SEXTEXT("Cannot parse the right part of the expression: ") << rightString;
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

		Throw(parent, SEXTEXT("Not implemented"));
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
         Throw(src, SEXTEXT("Expecting binary boolean operator"));
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

	VARTYPE GetAtomicValueAnyNumeric(CCompileEnvironment& ce, cr_sex s, csexstr id, int tempdepth)
	{		
		VARTYPE type = VARTYPE_Bad;

		if (IsCapital(id[0]))
		{
			IFunctionBuilder& f = MustMatchFunction(ce.Builder.Module(), s, id);
			if (!IsGetAccessor(f) || !IsNumericTypeOrBoolean(f.GetArgument(0).VarType()))
			{
				sexstringstream streamer;
				streamer << SEXTEXT("Expecting variable or single valued function with no inputs, and return type ") << Parse::VarTypeName(type);
				Throw(s, streamer);
			}

			type = f.GetArgument(0).VarType();
			CompileFunctionCallAndReturnValue(ce, s, f, type, NULL, NULL);	

			if (tempdepth > Sexy::ROOT_TEMPDEPTH)
			{
				ce.Builder.Assembler().Append_MoveRegister(VM::REGISTER_D7, VM::REGISTER_D4 + tempdepth, GetBitCount(type));
			}
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
			csexstr instance, item;
			if (splitter.SplitTail(instance, item))
			{
				if (IsCapital(item[0]))
				{
					type = CompileMethodCallWithoutInputAndReturnNumericValue(ce, s, instance, item);
					if (!IsNumericTypeOrBoolean(type))
					{
						sexstringstream streamer;
						streamer << SEXTEXT("Expecting method returning ") << Parse::VarTypeName(type);
						Throw(s, streamer);
					}

					if (tempdepth > Sexy::ROOT_TEMPDEPTH)
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
						ThrowTokenNotFound(s, instance, ce.Builder.Owner().Name(), SEXTEXT("variable"));
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
					ThrowTokenNotFound(s, id, ce.Builder.Owner().Name(), SEXTEXT("variable"));
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
         Throw(src, SEXTEXT("Cannot find subtraction rule for the given type"));
      }

      assembler.Append_SetIf(op, VM::REGISTER_D4 + booleanTargetId, GetBitCount(type));
   }


	void CompileBinaryCompareVariableVsLiteral(CCompileEnvironment& ce, cr_sex parent, csexstr leftVarName, CONDITION op, csexstr rightString, VARTYPE rType, cr_sex leftVarExpr)
	{
		VariantValue rValue;
		if (Parse::PARSERESULT_GOOD != Parse::TryParse(OUT rValue, rType, rightString))
		{
			sexstringstream streamer;
			streamer << SEXTEXT("Cannot parse the left part of the expression: ") << rightString;
			Throw(parent, streamer);
		}		

		ICodeBuilder& builder = ce.Builder;

		if (IsZero(rValue, rType))
		{
			VARTYPE varLType = GetAtomicValueAnyNumeric(ce, leftVarExpr, leftVarName, Sexy::ROOT_TEMPDEPTH);

			VariantValue newRValue;
			if (varLType != VARTYPE_Pointer && !Variants::TryRecast(OUT newRValue, IN rValue, rType, varLType))
			{
				Throw(parent, SEXTEXT("Cannot cast RHS to the type of the LHS"));
			}

			builder.Assembler().Append_SetIf(op, VM::REGISTER_D7, GetBitCount(rType));
		}
		else
		{
			VARTYPE varLType = GetAtomicValueAnyNumeric(ce, leftVarExpr, leftVarName, Sexy::ROOT_TEMPDEPTH + 1);

			VariantValue newRValue;
			if (!Variants::TryRecast(OUT newRValue, IN rValue, rType, varLType))
			{
				Throw(parent, SEXTEXT("Cannot cast RHS to the type of the LHS"));
			}

			builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D7 + 2, newRValue, GetBitCount(varLType));
			AddBinaryComparison(parent, builder.Assembler(), Sexy::ROOT_TEMPDEPTH, Sexy::ROOT_TEMPDEPTH + 1, Sexy::ROOT_TEMPDEPTH + 2, op, varLType);
		}
	}

   void AddBinaryBoolean(cr_sex src, VM::IAssembler& assembler, int booleanTargetId, int sourceA, int sourceB, LOGICAL_OP op)
   {
      if (booleanTargetId != sourceA - 1)
      {
         Throw(src, SEXTEXT("Algorthimic error: Expecting [booleanTargetId] to be [sourceA-1]"));
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

	void CompileBinaryBooleanLiteralVsVariable(CCompileEnvironment& ce, cr_sex parent, int literalValue, LOGICAL_OP op, csexstr variableName)
	{
		VARTYPE varType = ce.Builder.GetVarType(variableName);
		if (varType == VARTYPE_Bad)
		{
			Throw(parent, SEXTEXT("Cannot resolve variable as a literal or identifier"));
		}
		else if (varType == VARTYPE_Derivative)
		{
			Throw(parent, SEXTEXT("Cannot compare derived types"));
		}
		else if (varType != VARTYPE_Bool)
		{
			Throw(parent, SEXTEXT("Cannot logically implicitly cast variable to a boolean"));
		}

		VariantValue lValue;
		lValue.int32Value = literalValue;

		ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D7 + 1, lValue, BITCOUNT_32);
		ce.Builder.AssignVariableToTemp(variableName, Sexy::ROOT_TEMPDEPTH + 2);
		AddBinaryBoolean(parent, ce.Builder.Assembler(), Sexy::ROOT_TEMPDEPTH, Sexy::ROOT_TEMPDEPTH + 1, Sexy::ROOT_TEMPDEPTH + 2, op);

	}

	void CompileBinaryCompareLiteralVsVariable(CCompileEnvironment& ce, cr_sex parent, csexstr leftString, VARTYPE lType, CONDITION op, csexstr rightVarName)
	{
		VariantValue lValue;
		if (Parse::PARSERESULT_GOOD != Parse::TryParse(OUT lValue, lType, leftString))
		{
			sexstringstream streamer;
			streamer << SEXTEXT("Cannot parse the left part of the expression: ") << leftString;
			Throw(parent, streamer);
		}		

		VARTYPE varRType = ce.Builder.GetVarType(rightVarName);
		if (varRType == VARTYPE_Bad)
		{
			Throw(parent, SEXTEXT("Cannot resolve right hand side as a literal or identifier"));
		}
		else if (varRType == VARTYPE_Derivative)
		{
			Throw(parent, SEXTEXT("Cannot compare derived types"));
		}

		VariantValue newLValue;
		if (!Variants::TryRecast(OUT newLValue, IN lValue, lType, varRType))
		{
			Throw(parent, SEXTEXT("Cannot cast LHS to the type of the RHS"));
		}

		ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D7 + 1, newLValue, GetBitCount(varRType));
		ce.Builder.AssignVariableToTemp(rightVarName, Sexy::ROOT_TEMPDEPTH + 2);
		AddBinaryComparison(parent, ce.Builder.Assembler(), Sexy::ROOT_TEMPDEPTH, Sexy::ROOT_TEMPDEPTH + 1, Sexy::ROOT_TEMPDEPTH + 2, op, varRType);
	}

	void CompileBinaryCompareAtomicVsCompound(CCompileEnvironment& ce, cr_sex parent, csexstr varName, CONDITION op, cr_sex s, bool leftToRight);

	void CompileBinaryCompareVariableVsVariable(CCompileEnvironment& ce, cr_sex parent, csexstr leftVarName, CONDITION op, cr_sex rightExpr)
	{
		ICodeBuilder& builder = ce.Builder;

		csexstr rightVarName = rightExpr.String()->Buffer;

		VARTYPE varLType = builder.GetVarType(leftVarName);
		VARTYPE varRType = builder.GetVarType(rightVarName);	

		if (varLType == VARTYPE_Bad)
		{
			Throw(parent, SEXTEXT("The LHS is neither a literal, nor an identifier"));
		}
		else if (varLType == VARTYPE_Derivative)
		{
			Throw(parent, SEXTEXT("The LHS is a derived type, and cannot be used in boolean comparisons"));
		}

		if (varRType == VARTYPE_Bad)
		{
			CompileBinaryCompareAtomicVsCompound(ce, parent, leftVarName, op, rightExpr, true);
			return;
		}
		else if (varRType == VARTYPE_Derivative)
		{
			Throw(parent, SEXTEXT("The RHS is a derived type, and cannot be used in boolean comparisons"));
		}

		if (varLType != varRType)
		{
			Throw(parent, SEXTEXT("The LHS is not the same type as the RHS"));
		}

		builder.AssignVariableToTemp(leftVarName, Sexy::ROOT_TEMPDEPTH + 1);
		builder.AssignVariableToTemp(rightVarName, Sexy::ROOT_TEMPDEPTH + 2);
		AddBinaryComparison(parent, builder.Assembler(), Sexy::ROOT_TEMPDEPTH, Sexy::ROOT_TEMPDEPTH + 1, Sexy::ROOT_TEMPDEPTH + 2, op, varLType);
	}

	void CompileBinaryBooleanVariableVsVariable(CCompileEnvironment& ce, cr_sex parent, csexstr leftVarName, LOGICAL_OP op, csexstr rightVarName)
	{
		VARTYPE varLType = ce.Builder.GetVarType(leftVarName);
		VARTYPE varRType = ce.Builder.GetVarType(rightVarName);	

		if (varLType == VARTYPE_Bad)
		{
			Throw(parent, SEXTEXT("The LHS is neither a literal, nor an identifier"));
		}
		else if (varLType == VARTYPE_Derivative)
		{
			Throw(parent, SEXTEXT("The LHS is a derived type, and cannot be used in boolean comparisons"));
		}
		else if (varLType != VARTYPE_Bool)
		{
			Throw(parent, SEXTEXT("The LHS is not a boolean type"));
		}

		if (varRType == VARTYPE_Bad)
		{
			Throw(parent, SEXTEXT("The RHS is neither a literal, nor an identifier"));
		}
		else if (varRType == VARTYPE_Derivative)
		{
			Throw(parent, SEXTEXT("The RHS is a derived type, and cannot be used in boolean comparisons"));
		}
		else if (varRType != VARTYPE_Bool)
		{
			Throw(parent, SEXTEXT("The RHS is not a boolean type"));
		}				

		ce.Builder.AssignVariableToTemp(leftVarName, Sexy::ROOT_TEMPDEPTH + 1);
		ce.Builder.AssignVariableToTemp(rightVarName, Sexy::ROOT_TEMPDEPTH + 2);
		AddBinaryBoolean(parent, ce.Builder.Assembler(), Sexy::ROOT_TEMPDEPTH, Sexy::ROOT_TEMPDEPTH + 1, Sexy::ROOT_TEMPDEPTH + 2, op);
	}

	void CompileBinaryCompareAtomicVsCompound(CCompileEnvironment& ce, cr_sex parent, csexstr varName, CONDITION op, cr_sex s, bool leftToRight)
	{
		int A = leftToRight ? 1 : 2;
		int B = leftToRight ? 2 : 1;
		csexstr helper = leftToRight ? SEXTEXT("LHS") : SEXTEXT("RHS");

		VARTYPE type = ce.Builder.GetVarType(varName);
		if (type == VARTYPE_Derivative)
		{
			sexstringstream streamer;
			streamer << helper << SEXTEXT(" was of derived type and cannot be directly used in a comparison expression");
			Throw(parent, streamer);
		}
		else if (type == VARTYPE_Bad)
		{
			type = Parse::GetLiteralType(varName);
			if (!IsPrimitiveType(type))
			{
				sexstringstream streamer;
				streamer << helper << SEXTEXT(" was not recognized either as a literal, or as an identifier");
				Throw(parent, streamer);
			}

			VariantValue value;
			if (Parse::TryParse(OUT value, type, varName) != Parse::PARSERESULT_GOOD)
			{
				sexstringstream streamer;
				streamer << helper << SEXTEXT(" was not parsed either as a literal, or as an identifier");
				Throw(parent, streamer);
			}

			ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D7 + A, value, GetBitCount(type));
		}
		else
		{
			ce.Builder.AssignVariableToTemp(varName, Sexy::ROOT_TEMPDEPTH + A);
		}

		TryCompileArithmeticExpression(ce, s, true, type);
		ce.Builder.Assembler().Append_MoveRegister(VM::REGISTER_D7, VM::REGISTER_D7 + B, GetBitCount(type));

		AddBinaryComparison(parent, ce.Builder.Assembler(), Sexy::ROOT_TEMPDEPTH, Sexy::ROOT_TEMPDEPTH + 1, Sexy::ROOT_TEMPDEPTH + 2, op, type);
	}

	// Compile code to evaluate a boolean and copy the value to the temp variable #tempDepth
	void CompileBinaryCompareExpression(CCompileEnvironment& ce, cr_sex parent, cr_sex left, CONDITION op, cr_sex right)
	{
		// Types: literal, variable, compound expression
		if (IsAtomic(left))
		{
			csexstr leftString = left.String()->Buffer;
			VARTYPE lType = Parse::GetLiteralType(leftString);
			if (IsPrimitiveType(lType))
			{
				if (IsAtomic(right))
				{
					csexstr rightString = right.String()->Buffer;
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
					csexstr rightString = right.String()->Buffer;
					VARTYPE rType = Parse::GetLiteralType(rightString);

					if (IsPrimitiveType(rType))
					{
						CompileBinaryCompareVariableVsLiteral(ce, parent, leftString, op, rightString, rType, left);
					}
					else
					{
						CompileBinaryCompareVariableVsVariable(ce, parent, leftString, op, right);							
					}
				}
				else if (!IsCompound(right))
				{
					Throw(parent, SEXTEXT("RHS neither atomic nor compound"));
				}
				else
				{
					CompileBinaryCompareAtomicVsCompound(ce, parent, right.String()->Buffer, op, left, true);
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
				Throw(parent, SEXTEXT("RHS is neither atomic nor compound"));
			}
			else
			{
				VARTYPE guessType = GuessType(parent, ce.Builder);
				if (IsPrimitiveType(guessType))
				{
					BITCOUNT bits = GetBitCount(guessType);

					TryCompileArithmeticExpression(ce, left, true, guessType);

					AddArchiveRegister(ce, Sexy::ROOT_TEMPDEPTH, Sexy::ROOT_TEMPDEPTH + 1, bits);
						TryCompileArithmeticExpression(ce, right, true, guessType);
						ce.Builder.Assembler().Append_MoveRegister(VM::REGISTER_D7, VM::REGISTER_D7 + 2, GetBitCount(guessType));
					ce.Builder.PopLastVariables(1);

					AddBinaryComparison(parent, ce.Builder.Assembler(), Sexy::ROOT_TEMPDEPTH, Sexy::ROOT_TEMPDEPTH+1, Sexy::ROOT_TEMPDEPTH+2, op, guessType);
				}
				else
				{
					Throw(parent, SEXTEXT("Cannot infer types in the comparison"));
				}
			}
		}
	}

	void CompileBinaryBooleanLiteralVsCompoundExpression(CCompileEnvironment& ce, cr_sex parent, int32 value, LOGICAL_OP op, cr_sex right)
	{
		bool invert = false;

		if (value == 0)
		{
			switch(op)
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
			switch(op)
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
				Throw(parent, SEXTEXT("Expecting a boolean valued expression on the RHS"));
			}

			if (invert)
			{
				ce.Builder.Assembler().Append_MoveRegister(VM::REGISTER_D4 +  Sexy::ROOT_TEMPDEPTH, VM::REGISTER_D4 +  Sexy::ROOT_TEMPDEPTH + offset, BITCOUNT_32);
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
			Throw(parent, SEXTEXT("Expecting a boolean valued expression on the RHS"));
		}
	}

	void AppendBooleanLogicalOp(cr_sex src, LOGICAL_OP op, VM::IAssembler& assember, int tempDepthA, int tempDepthB)
	{
		switch(op)
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
			Throw(src, SEXTEXT("Unhandled logical op type"));
		}
	}

	void CompileBinaryBooleanVariableVsCompoundExpression(CCompileEnvironment& ce, cr_sex parent, csexstr leftString, LOGICAL_OP op, cr_sex right)
	{
		VARTYPE type = ce.Builder.GetVarType(leftString);
		if (type != VARTYPE_Bool)
		{
			Throw(parent, SEXTEXT("Expecting boolean variable on LHS"));
		}

		bool negate = false;
		if (TryCompileBooleanExpression(ce, right, true, negate))
		{
			if (negate) ce.Builder.Assembler().Append_BooleanNot( VM::REGISTER_D7);
			ce.Builder.AssignVariableToTemp(leftString, Sexy::ROOT_TEMPDEPTH + 1);
			ce.Builder.Assembler().Append_MoveRegister(VM::REGISTER_D7, VM::REGISTER_D7 +2, GetBitCount(type));
			AppendBooleanLogicalOp(parent, op, ce.Builder.Assembler(), Sexy::ROOT_TEMPDEPTH + 1, Sexy::ROOT_TEMPDEPTH + 2);
			return;
		}
		else
		{
			Throw(parent, SEXTEXT("Cannot interpret RHS as a binary predicate expression"));
		}
	}

	void CompileBinaryBooleanCompoundVsCompound(CCompileEnvironment& ce, cr_sex parent, cr_sex left, LOGICAL_OP op, cr_sex right)
	{
		bool negate = false;
		if (!TryCompileBooleanExpression(ce, left, true, negate))
		{
			Throw(left, SEXTEXT("Cannot compile LHS as boolean binary predicate"));
		}

		if (negate) ce.Builder.Assembler().Append_BooleanNot(VM::REGISTER_D7);

		AddArchiveRegister(ce, Sexy::ROOT_TEMPDEPTH, Sexy::ROOT_TEMPDEPTH + 1, BITCOUNT_32);		
			negate = false;
			if (!TryCompileBooleanExpression(ce, right, true, negate))
			{
				Throw(left, SEXTEXT("Cannot compile RHS as boolean binary predicate"));
			}

			if (negate) ce.Builder.Assembler().Append_BooleanNot(VM::REGISTER_D7);
		ce.Builder.PopLastVariables(1);

		ce.Builder.Assembler().Append_MoveRegister(VM::REGISTER_D7, VM::REGISTER_D7 + 2, BITCOUNT_32);

		AppendBooleanLogicalOp(parent, op, ce.Builder.Assembler(), Sexy::ROOT_TEMPDEPTH + 1, Sexy::ROOT_TEMPDEPTH + 2);
	}

	void CompileBinaryBooleanAtomicVsCompoundExpression(CCompileEnvironment& ce, cr_sex parent, csexstr leftString, LOGICAL_OP op, cr_sex right)
	{
		VARTYPE lType = ce.Builder.GetVarType(leftString);
		if (lType == VARTYPE_Derivative)
		{
			Throw(parent, SEXTEXT("LHS was of a derived type, and cannot be directly used in boolean expressions"));
		}
		else if(lType == VARTYPE_Bad)
		{
			int32 value;
			if (Parse::TryParseBoolean(OUT value, leftString) != Parse::PARSERESULT_GOOD)
			{
				Throw(parent, SEXTEXT("In the boolean expression the LHS was neither an identifier nor a known boolean value"));
			}

			CompileBinaryBooleanLiteralVsCompoundExpression(ce, parent, value, op, right);
			return;
		}
		else if (lType != VARTYPE_Bool)
		{
			Throw(parent, SEXTEXT("Identifier in the LHS of the boolean expression was not of underlying type Int32"));
		}

		CompileBinaryBooleanVariableVsCompoundExpression(ce, parent, leftString, op, right);
	}

	// Compile code to evaluate a boolean and copy the value to the temp variable #tempDepth
	void CompileBinaryBooleanExpression(CCompileEnvironment& ce, cr_sex parent, cr_sex left, LOGICAL_OP op, cr_sex right)
	{
		// Types: literal, variable, compound expression
		if (IsAtomic(left))
		{
			csexstr leftString = left.String()->Buffer;

			if (IsCompound(right))
			{
				CompileBinaryBooleanAtomicVsCompoundExpression(ce, parent, leftString, op, right);
				return;
			}
			if (!IsAtomic(right))
			{
				Throw(parent, SEXTEXT("The RHS in the boolean expression is neither an atomic nor compound expression"));
			}

			csexstr rightString = right.String()->Buffer;

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
			Throw(parent, SEXTEXT("LHS neither atomic nor compound type"));
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

	const IFunction* TryGetMethod(const MemberDef& def, csexstr methodName)
	{
		if (Sexy::IsCapital(methodName[0]))
		{
			TokenBuffer localMethodName;
			StringPrint(localMethodName, SEXTEXT("%s.%s"), def.ResolvedType->Name(), methodName);
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
      if (AreEqual(op, SEXTEXT(">"))) return negate ? CONDITION_IF_LESS_OR_EQUAL : CONDITION_IF_GREATER_THAN;
      if (AreEqual(op, SEXTEXT("<")))	return negate ? CONDITION_IF_GREATER_OR_EQUAL : CONDITION_IF_LESS_THAN;
      if (AreEqual(op, SEXTEXT(">="))) return negate ? CONDITION_IF_LESS_THAN : CONDITION_IF_GREATER_OR_EQUAL;
      if (AreEqual(op, SEXTEXT("<="))) return negate ? CONDITION_IF_GREATER_THAN : CONDITION_IF_LESS_OR_EQUAL;
      if (AreEqual(op, SEXTEXT("!="))) return negate ? CONDITION_IF_EQUAL : CONDITION_IF_NOT_EQUAL;
      if (AreEqual(op, SEXTEXT("=="))) return negate ? CONDITION_IF_NOT_EQUAL : CONDITION_IF_EQUAL;

      Throw(opExpr, SEXTEXT("Cannot interpret as a comparison operator"));
      return CONDITION_IF_EQUAL;
   }

   LOGICAL_OP GetBinaryLogicalOp(cr_sex opExpr)
   {
      sexstring op = opExpr.String();
      if (AreEqual(op, SEXTEXT("and"))) return LOGICAL_OP_AND;
      if (AreEqual(op, SEXTEXT("or")))	return LOGICAL_OP_OR;
      if (AreEqual(op, SEXTEXT("xor"))) return LOGICAL_OP_XOR;

      Throw(opExpr, SEXTEXT("Cannot interpret as a binary logical operation"));
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
				if (AreEqual(notIndicator.String(), SEXTEXT("not")))
				{
					cr_sex onlyChild = s.GetElement(1);
					negate = !negate;
					return TryCompileBooleanExpression(ce, onlyChild, expected, negate);
				}
				else
				{
					if (expected)
					{
						Throw(s, SEXTEXT("Expected 'not' symbol in first argument of a two element binary expression"));
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
						Throw(s, SEXTEXT("Expected boolean expression, but could not see a binary predicate operator"));
					}
					// No binary predicate operator
					return false;
				}
			}
			else
			{
				if (expected)
				{
					sexstringstream streamer;
					streamer << SEXTEXT("Expected expression with 3 elements. This expression had ") << s.NumberOfElements() << SEXTEXT(" elements.");
					Throw(s, streamer);
				}
				// All binary predicate expressions have 3 elements
				return false;
			}
		}
		else if (IsAtomic(s))
		{
			csexstr token = s.String()->Buffer;

			VariantValue value;
			if (Parse::TryParseBoolean(OUT value.int32Value, IN token) == Parse::PARSERESULT_GOOD)
			{
				if (negate) value.int32Value = !value.int32Value;
				ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D7, value, BITCOUNT_32);
				return true;
			}
			else
			{
				if (VARTYPE_Bool == ce.Builder.GetVarType(token))
				{
					ce.Builder.AssignVariableToTemp(token, Sexy::ROOT_TEMPDEPTH);
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
						Throw(s, SEXTEXT("Expected binary expression, but found an identifier/literal not of the boolean type"));
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
				Throw(s, SEXTEXT("Expected binary expression, but expression was neither atomic nor compound"));
			}
			// not an atomic or a compound
			return false;
		}
	}
}