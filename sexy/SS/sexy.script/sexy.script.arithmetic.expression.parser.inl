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

namespace Rococo 
{
   namespace Script
   {
      bool IsBinaryCompareOperator(cr_sex s)
      {
         if (s.Type() == EXPRESSION_TYPE_ATOMIC)
         {
            sexstring op = s.String();
            if (AreEqual(op, (">"))) return true;
            if (AreEqual(op, ("<")))	return true;
            if (AreEqual(op, (">="))) return true;
            if (AreEqual(op, ("<="))) return true;
            if (AreEqual(op, ("!="))) return true;
            if (AreEqual(op, ("=="))) return true;
         }

         return false;
      }

      bool IsBinaryBooleanOperator(cr_sex s)
      {
         if (s.Type() == EXPRESSION_TYPE_ATOMIC)
         {
            sexstring op = s.String();
            if (AreEqual(op, ("and"))) return true;
            if (AreEqual(op, ("or")))	return true;
            if (AreEqual(op, ("xor"))) return true;
         }

         return false;
      }

      bool IsArithmeticOperator(cr_sex s)
      {
         if (!IsAtomic(s))
         {
            return false;
         }

         sexstring opStr = s.String();

         if (AreEqual(opStr, ("+"))) return true;
         if (AreEqual(opStr, ("-"))) return true;
         if (AreEqual(opStr, ("*"))) return true;
         if (AreEqual(opStr, ("/"))) return true;
		 if (AreEqual(opStr, ("|"))) return true;

         return false;
      }

      enum ARITHMETIC_OP
      {
         ARITHMETIC_OP_ADD,
         ARITHMETIC_OP_SUBTRACT,
         ARITHMETIC_OP_MULTIPLY,
         ARITHMETIC_OP_DIVIDE,
		 ARITHMETIC_OP_BITWISE_OR
      };

      enum ARITHMETIC_ORDER
      {
         ARITHMETIC_ORDER_LEFT_TO_RIGHT,
         ARITHMETIC_ORDER_RIGHT_TO_LEFT
      };

      VARTYPE GetBestGuessType(VARTYPE a, VARTYPE b)
      {
         if (a == VARTYPE_Derivative && b == VARTYPE_Derivative) return VARTYPE_Derivative;
         if (a == VARTYPE_Float64 || b == VARTYPE_Float64) return VARTYPE_Float64;
         if (a == VARTYPE_Float32 || b == VARTYPE_Float32) return VARTYPE_Float32;
         if (a == VARTYPE_Int64 || b == VARTYPE_Int64) return VARTYPE_Int64;
         if (a == VARTYPE_Int32 || b == VARTYPE_Int32) return VARTYPE_Int32;
         return VARTYPE_Bad;
      }

      VARTYPE GuessType(cr_sex s, ICodeBuilder& builder)
      {
         if (IsAtomic(s))
         {
            cstr token = s.String()->Buffer;
            VARTYPE type = builder.GetVarType(token);
            if (type != VARTYPE_Bad) return type;
            return Parse::GetLiteralType(token);
         }
         else if (!IsCompound(s))
         {
            return VARTYPE_Bad;
         }
         else
         {
            int nElements = s.NumberOfElements();
            if (nElements == 1)
            {
               return GuessType(s.GetElement(0), builder);
            }
            else if (nElements == 3)
            {
               if (IsArithmeticOperator(s.GetElement(1)) || IsBinaryCompareOperator(s.GetElement(1)))
               {
                  VARTYPE lType = GuessType(s.GetElement(0), builder);
                  VARTYPE rType = GuessType(s.GetElement(2), builder);
                  return GetBestGuessType(lType, rType);
               }
               else if (IsBinaryBooleanOperator(s.GetElement(1)))
               {
                  return VARTYPE_Bool;
               }
               else
               {
                  return VARTYPE_Bad;
               }
            }
            else
            {
               return VARTYPE_Bad;
            }
         }
      }

      ARITHMETIC_OP GetArithmeticOp(cr_sex s)
      {
         if (!IsAtomic(s))
         {
            Throw(s, ("Expecting arithmetic operator, but found a compound expression"));
         }

         sexstring opStr = s.String();

         if (AreEqual(opStr, ("+"))) return ARITHMETIC_OP_ADD;
         if (AreEqual(opStr, ("-"))) return ARITHMETIC_OP_SUBTRACT;
         if (AreEqual(opStr, ("*"))) return ARITHMETIC_OP_MULTIPLY;
         if (AreEqual(opStr, ("/"))) return ARITHMETIC_OP_DIVIDE;
		 if (AreEqual(opStr, ("|"))) return ARITHMETIC_OP_BITWISE_OR;

         Throw(s, ("Expecting arithmetic operator"));
         return ARITHMETIC_OP_ADD;
      }

      void ComputeArithmeticLiteralVsLiteral(OUT VariantValue& result, cr_sex parent, const VariantValue& a, ARITHMETIC_OP op, const VariantValue& b, VARTYPE type)
      {
         switch (type)
         {
         case VARTYPE_Int32:
            switch (op)
            {
            case ARITHMETIC_OP_ADD:
               result.int32Value = a.int32Value + b.int32Value;
               return;
            case ARITHMETIC_OP_MULTIPLY:
               result.int32Value = a.int32Value * b.int32Value;
               return;
            case ARITHMETIC_OP_DIVIDE:
               result.int32Value = a.int32Value / b.int32Value;
               return;
            case ARITHMETIC_OP_SUBTRACT:
               result.int32Value = a.int32Value - b.int32Value;
               return;
			case ARITHMETIC_OP_BITWISE_OR:
				result.int32Value = a.int32Value | b.int32Value;
				return;
            default:
               goto Error;
            }
         case VARTYPE_Int64:
            switch (op)
            {
            case ARITHMETIC_OP_ADD:
               result.int64Value = a.int64Value + b.int64Value;
               return;
            case ARITHMETIC_OP_MULTIPLY:
               result.int64Value = a.int64Value * b.int64Value;
               return;
            case ARITHMETIC_OP_DIVIDE:
               result.int64Value = a.int64Value / b.int64Value;
               return;
            case ARITHMETIC_OP_SUBTRACT:
               result.int64Value = a.int64Value - b.int64Value;
               return;
			case ARITHMETIC_OP_BITWISE_OR:
				result.int64Value = a.int64Value | b.int64Value;
				return;
            default:
               goto Error;
            }
         case VARTYPE_Float32:
            switch (op)
            {
            case ARITHMETIC_OP_ADD:
               result.floatValue = a.floatValue + b.floatValue;
               return;
            case ARITHMETIC_OP_MULTIPLY:
               result.floatValue = a.floatValue * b.floatValue;
               return;
            case ARITHMETIC_OP_DIVIDE:
               result.floatValue = a.floatValue / b.floatValue;
               return;
            case ARITHMETIC_OP_SUBTRACT:
               result.floatValue = a.floatValue - b.floatValue;
               return;
            default:
               goto Error;
            }
         case VARTYPE_Float64:
            switch (op)
            {
            case ARITHMETIC_OP_ADD:
               result.doubleValue = a.doubleValue + b.doubleValue;
               return;
            case ARITHMETIC_OP_MULTIPLY:
               result.doubleValue = a.doubleValue * b.doubleValue;
               return;
            case ARITHMETIC_OP_DIVIDE:
               result.doubleValue = a.doubleValue / b.doubleValue;
               return;
            case ARITHMETIC_OP_SUBTRACT:
               result.doubleValue = a.doubleValue - b.doubleValue;
               return;
            default:
               goto Error;
            }
         default:
            Throw(parent, ("Unsupported type in literal arithmetic"));
         }

      Error:
         Throw(parent, ("Unsupported operator in literal arithmetic"));
      }

      cstr GetTypeName(VARTYPE type)
      {
         switch (type)
         {
         case VARTYPE_Bad: return ("bad");
         case VARTYPE_Derivative: return ("derived");
         case VARTYPE_Int32: return ("Int32");
         case VARTYPE_Int64: return ("Int64");
         case VARTYPE_Float32: return ("Float32");
         case VARTYPE_Float64: return ("Float64");
         case VARTYPE_Bool: return ("Bool");
         case VARTYPE_Pointer: return ("Pointer");
         case VARTYPE_Closure: return ("Archetype");
         default: return ("unknown-type");
         }
      }

      void ValidateArithmeticVariable(cr_sex parent, cstr token, ICodeBuilder& builder, VARTYPE type, cstr helper)
      {
         VARTYPE rType = builder.GetVarType(token);
         if (rType == VARTYPE_Bad)
         {
            sexstringstream<1024> streamer;

            VARTYPE literalType = Parse::GetLiteralType(token);
            if (IsPrimitiveType(literalType))
            {
               streamer.sb << token << (" was ") << GetTypeName(literalType) << (" Expecting ") << GetTypeName(type);
            }
            else
            {
               streamer.sb << token << (" was not a literal type or a known identifier");
            }
            Throw(parent, streamer);
         }
         else if (rType == VARTYPE_Derivative)
         {
            sexstringstream<1024> streamer;
            streamer.sb << token << (" was a derived type, it cannot be used directly in arithmetic expressions");
            Throw(parent, streamer);
         }

         if (rType != type)
         {
            sexstringstream<1024> streamer;
            streamer.sb << token << (" was not the same type as that of the arithmetic expression in which it was referenced");
            Throw(parent, streamer);
         }
      }

      void AppendArithmeticOp(VM::IAssembler& assembler, cr_sex src, ARITHMETIC_OP op, VARTYPE type, int tempDepth)
      {
         VM::DINDEX reg = VM::REGISTER_D4 + tempDepth;
         VM::FLOATSPEC spec = VM::FLOATSPEC_DOUBLE;

         switch (type)
         {
         case VARTYPE_Int32:
         case VARTYPE_Int64:
            switch (op)
            {
            case ARITHMETIC_OP_ADD:
               assembler.Append_IntAdd(reg + 1, GetBitCount(type), reg + 2);
               return;
            case ARITHMETIC_OP_SUBTRACT:
               assembler.Append_IntSubtract(reg + 1, GetBitCount(type), reg + 2);
               return;
            case ARITHMETIC_OP_MULTIPLY:
               assembler.Append_IntMultiply(reg + 1, GetBitCount(type), reg + 2);
               return;
            case ARITHMETIC_OP_DIVIDE:
               assembler.Append_IntDivide(reg + 1, GetBitCount(type), reg + 2); // N.B saves result in quotient in reg, but also remainder in reg+3
               return;
			case ARITHMETIC_OP_BITWISE_OR:
				assembler.Append_LogicalOR(reg + 1, GetBitCount(type), reg + 2); 
				return;
            default:
               goto Error;
            }
         case VARTYPE_Float32:
            spec = VM::FLOATSPEC_SINGLE;
         case VARTYPE_Float64:
            switch (op)
            {
            case ARITHMETIC_OP_ADD:
               assembler.Append_FloatAdd(reg + 1, reg + 2, spec);
               return;
            case ARITHMETIC_OP_SUBTRACT:
               assembler.Append_FloatSubtract(reg + 1, reg + 2, spec);
               return;
            case ARITHMETIC_OP_MULTIPLY:
               assembler.Append_FloatMultiply(reg + 1, reg + 2, spec);
               return;
            case ARITHMETIC_OP_DIVIDE:
               assembler.Append_FloatDivide(reg + 1, reg + 2, spec); // N.B saves result in quotient in reg, but also remainder in reg+3
               return;
            default:
               goto Error;
            }
         default:
            Throw(src, ("Unhandled type in arithmetic expression"));
         }

      Error:
         Throw(src, ("Unhandled operator in arithmetic expression"));
      }

      void GetAtomicValue(CCompileEnvironment& ce, cr_sex parent, cstr id, VARTYPE type)
      {
         if (IsCapital(id[0]))
         {
            IFunctionBuilder& f = MustMatchFunction(ce.Builder.Module(), parent, id);
            if (!IsGetAccessor(f) && f.GetArgument(0).VarType() != type)
            {
               sexstringstream<1024> streamer;
               streamer.sb << ("Expecting variable or single valued function with no inputs of return type ") << Parse::VarTypeName(type);
               Throw(parent, streamer);
            }

            CompileFunctionCallAndReturnValue(ce, parent, f, type, NULL, NULL);
         }
         else
         {
            NamespaceSplitter splitter(id);
            cstr instance, item;
            if (splitter.SplitTail(instance, item))
            {
               if (IsCapital(item[0]))
               {
                  if (!TryCompileMethodCallWithoutInputAndReturnValue(ce, parent, instance, item, type, NULL, NULL))
                  {
                     sexstringstream<1024> streamer;
                     streamer.sb << ("Expecting method call ") << Parse::VarTypeName(type);
                     Throw(parent, streamer);
                  }
               }
               else
               {
                  MemberDef def;
                  if (ce.Builder.TryGetVariableByName(OUT def, id))
                  {
                     ce.Builder.AssignVariableToTemp(id, Rococo::ROOT_TEMPDEPTH);
                  }
                  else
                  {
                     ThrowTokenNotFound(parent, instance, ce.Builder.Owner().Name(), ("variable"));
                  }
               }
            }
            else
            {
               if (ce.Builder.GetVarStructure(id) != NULL)
               {
                  ce.Builder.AssignVariableToTemp(id, Rococo::ROOT_TEMPDEPTH);
               }
               else
               {
                  ThrowTokenNotFound(parent, id, ce.Builder.Owner().Name(), ("variable"));
               }
            }
         }
      }

      void CompileArithmeticLiteralVsVariable(CCompileEnvironment& ce, cr_sex parent, const VariantValue& value, ARITHMETIC_OP op, cstr varname, VARTYPE type, ARITHMETIC_ORDER order, cr_sex atomicExpr)
      {
         // ValidateArithmeticVariable(parent, varname, ce.Builder, type, order == ARITHMETIC_ORDER_LEFT_TO_RIGHT ? ("RHS") : ("LHS"));

         int A = order == ARITHMETIC_ORDER_LEFT_TO_RIGHT ? 1 : 2;
         int B = order == ARITHMETIC_ORDER_LEFT_TO_RIGHT ? 2 : 1;

         ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D7 + A, value, GetBitCount(type));

         GetAtomicValue(ce, atomicExpr, varname, type);
         ce.Builder.Assembler().Append_MoveRegister(VM::REGISTER_D7, VM::REGISTER_D7 + B, GetBitCount(type));
         AppendArithmeticOp(ce.Builder.Assembler(), parent, op, type, Rococo::ROOT_TEMPDEPTH);
      }

      void CompileArithmeticAtomicVsAtomic(CCompileEnvironment& ce, cr_sex parent, cstr left, ARITHMETIC_OP op, cstr right, VARTYPE type, cr_sex leftExpr, cr_sex rightExpr)
      {
         VariantValue A;
         if (Parse::TryParse(OUT A, IN type, IN left) == Parse::PARSERESULT_GOOD)
         {
            VariantValue B;
            if (Parse::TryParse(OUT B, IN type, IN right) == Parse::PARSERESULT_GOOD)
            {
               VariantValue result;
               ComputeArithmeticLiteralVsLiteral(OUT result, parent, A, op, B, type);
               ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D7, result, GetBitCount(type));
               return;
            }

            CompileArithmeticLiteralVsVariable(ce, parent, A, op, right, type, ARITHMETIC_ORDER_LEFT_TO_RIGHT, rightExpr);
            return;
         }

         // ValidateArithmeticVariable(parent, left, ce.Builder, type, ("LHS"));

         VariantValue B;
         if (Parse::TryParse(OUT B, IN type, IN right) == Parse::PARSERESULT_GOOD)
         {
            CompileArithmeticLiteralVsVariable(ce, parent, B, op, left, type, ARITHMETIC_ORDER_RIGHT_TO_LEFT, leftExpr);
         }
         else
         {
            BITCOUNT bits = GetBitCount(type);
            GetAtomicValue(ce, leftExpr, left, type);

            AddArchiveRegister(ce, Rococo::ROOT_TEMPDEPTH, Rococo::ROOT_TEMPDEPTH + 1, bits);
            GetAtomicValue(ce, rightExpr, right, type);
            ce.Builder.Assembler().Append_MoveRegister(VM::REGISTER_D7, VM::REGISTER_D7 + 2, GetBitCount(type));
            ce.Builder.PopLastVariables(1);

            AppendArithmeticOp(ce.Builder.Assembler(), parent, op, type, Rococo::ROOT_TEMPDEPTH);
         }
      }

      void CompileAtomicSide(CCompileEnvironment& ce, cr_sex parent, cstr token, int tempDepth, ARITHMETIC_ORDER order, VARTYPE type)
      {
         TokenBuffer symbol;
         StringPrint(symbol, (" = %s"), token);

         VariantValue value;
         if (Parse::TryParse(OUT value, type, token) == Parse::PARSERESULT_GOOD)
         {
            ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D4 + tempDepth, value, GetBitCount(type));
         }
         else
         {
            ValidateArithmeticVariable(parent, token, ce.Builder, type, order == ARITHMETIC_ORDER_LEFT_TO_RIGHT ? ("LHS") : ("RHS"));
            ce.Builder.AssignVariableToTemp(token, tempDepth);
         }
      }

      void CompileArithmeticAtomicVsCompound(CCompileEnvironment& ce, cr_sex parent, cstr token, ARITHMETIC_OP op, cr_sex s, ARITHMETIC_ORDER order, VARTYPE type)
      {
         int A = order == ARITHMETIC_ORDER_LEFT_TO_RIGHT ? 1 : 2;
         int B = order == ARITHMETIC_ORDER_LEFT_TO_RIGHT ? 2 : 1;

         BITCOUNT bits = GetBitCount(type);

         if (B > A)
         {
            CompileAtomicSide(ce, parent, token, Rococo::ROOT_TEMPDEPTH + A, order, type);

            AddArchiveRegister(ce, Rococo::ROOT_TEMPDEPTH + A, Rococo::ROOT_TEMPDEPTH + A, bits);
            TryCompileArithmeticExpression(ce, s, true, type);
            ce.Builder.PopLastVariables(1);

            ce.Builder.Assembler().Append_MoveRegister(VM::REGISTER_D7, VM::REGISTER_D7 + B, bits);
         }
         else
         {
            TryCompileArithmeticExpression(ce, s, true, type);
            ce.Builder.Assembler().Append_MoveRegister(VM::REGISTER_D7, VM::REGISTER_D7 + B, bits);
            CompileAtomicSide(ce, parent, token, Rococo::ROOT_TEMPDEPTH + A, order, type);
         }

         AppendArithmeticOp(ce.Builder.Assembler(), s, op, type, Rococo::ROOT_TEMPDEPTH);
      }

      void CompileArithmeticCompoundVsCompound(CCompileEnvironment& ce, cr_sex parent, cr_sex left, ARITHMETIC_OP op, cr_sex right, VARTYPE type)
      {
         BITCOUNT bits = GetBitCount(type);

         TryCompileArithmeticExpression(ce, left, true, type);
         AddArchiveRegister(ce, Rococo::ROOT_TEMPDEPTH, Rococo::ROOT_TEMPDEPTH + 1, bits);
         TryCompileArithmeticExpression(ce, right, true, type);
         ce.Builder.Assembler().Append_MoveRegister(VM::REGISTER_D7, VM::REGISTER_D7 + 2, bits);
         ce.Builder.PopLastVariables(1);
         AppendArithmeticOp(ce.Builder.Assembler(), parent, op, type, Rococo::ROOT_TEMPDEPTH);
      }

      void CompileBinaryArithmeticExpression(CCompileEnvironment& ce, cr_sex parent, cr_sex left, ARITHMETIC_OP op, cr_sex right, VARTYPE type)
      {
         if (IsAtomic(left))
         {
            cstr lToken = left.String()->Buffer;

            if (IsAtomic(right))
            {
               cstr rToken = right.String()->Buffer;
               CompileArithmeticAtomicVsAtomic(ce, parent, lToken, op, rToken, type, left, right);
               return;
            }
            else if (!IsCompound(right))
            {
               Throw(parent, ("The RHS was neither an atomic or a compound expression"));
            }

            CompileArithmeticAtomicVsCompound(ce, parent, lToken, op, right, ARITHMETIC_ORDER_LEFT_TO_RIGHT, type);
            return;
         }
         else if (!IsCompound(left))
         {
            Throw(parent, ("The LHS was neither an atomic or a compound expression"));
         }

         if (IsAtomic(right))
         {
            cstr rToken = right.String()->Buffer;
            CompileArithmeticAtomicVsCompound(ce, parent, rToken, op, left, ARITHMETIC_ORDER_RIGHT_TO_LEFT, type);
            return;
         }
         else if (!IsCompound(right))
         {
            Throw(parent, ("The RHS was neither an atomic or a compound expression"));
         }

         CompileArithmeticCompoundVsCompound(ce, parent, left, op, right, type);
      }

      const IArgument& GetOutput(cr_sex s, IFunction& f, int index)
      {
         int argIndex = index;
         if (argIndex > f.NumberOfOutputs())
         {
            Throw(s, ("Bad argument. Algorithmic error"));
         }

         return f.Arg(argIndex);
      }

      const IArgument& GetInput(cr_sex s, IFunction& f, int index)
      {
         int argIndex = index + f.NumberOfOutputs();
         if (argIndex > ArgCount(f))
         {
            Throw(s, ("Bad argument. Algorithmic error"));
         }

         return f.Arg(argIndex);
      }

      const IArchetype* GetArchetype(cr_sex s, IFunction& f, int index)
      {
         const IArgument& input = GetInput(s, f, index);

         if (input.ResolvedType()->VarType() != VARTYPE_Closure)
         {
            return NULL;
         }

         cstr name = input.Name();
         const IArchetype* a = input.ResolvedType()->Archetype();
         if (a == NULL)
         {
            sexstringstream<1024> streamer;
            streamer.sb << ("Error, expecting archetype for variable ") << name << (" in ") << f.Name();
            Throw(s, streamer);
         }

         return a;
      }

      int GetFirstOutputOffset(ICodeBuilder& builder)
      {
         int outputOffset = 0;

         int nTempVariables = builder.GetVariableCount() - ArgCount(builder.Owner());
         if (nTempVariables > 0)
         {
            MemberDef lastDef;
            cstr lastName;
            builder.GetVariableByIndex(OUT lastDef, lastName, builder.GetVariableCount() - 1);

            outputOffset = lastDef.SFOffset + lastDef.AllocSize;
         }

         return outputOffset;
      }

      void CompileSizeOfVariable(CCompileEnvironment& ce, cr_sex valueExpr)
      {
         AssertLocalIdentifier(valueExpr);

         cstr value = valueExpr.String()->Buffer;

         MemberDef def;
         if (!ce.Builder.TryGetVariableByName(OUT def, value))
         {
            Throw(valueExpr, ("Unknown variable"));
         }

         if (!def.ResolvedType->Prototype().IsClass)
         {
            VariantValue v;
            v.int32Value = def.AllocSize;
            ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D7, v, BITCOUNT_32);
         }
         else
         {
			 TokenBuffer ref;
			 GetRefName(ref, value);

			 ce.Builder.AssignVariableToTemp(ref, Rococo::ROOT_TEMPDEPTH);
			 ce.Builder.Append_GetAllocSize();
         }
      }

      void CompileSizeOfType(CCompileEnvironment& ce, cr_sex valueExpr)
      {
         cstr value = valueExpr.String()->Buffer;

         IStructure* s = MatchStructure(valueExpr, ce.Builder.Module());

         if (s == NULL)
         {
            Throw(valueExpr, ("Token began with a capital letter. Expected type name."));
         }

         VariantValue sizeOfType;
         sizeOfType.int32Value = s->SizeOfStruct();
         ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D7, sizeOfType, BITCOUNT_32);
      }

      void CompileSizeOfExpression(CCompileEnvironment& ce, cr_sex valueExpr)
      {
         AssertAtomic(valueExpr);

         cstr value = valueExpr.String()->Buffer;

         if (IsLowerCase(value[0]))
         {
            CompileSizeOfVariable(ce, valueExpr);
         }
         else
         {
            CompileSizeOfType(ce, valueExpr);
         }
      }

      bool TryCompileAssignArchetype(CCompileEnvironment& ce, cr_sex s, const IStructure& type, bool allowClosures)
      {
         if (IsCompound(s))
         {
            if (TryCompileFunctionCallAndReturnValue(ce, s, VARTYPE_Closure, &type, type.Archetype()))
            {
               // Functions and methods cannot return closures, so no need to check allowClosures flag
               return true;
            }
            else
            {
               cr_sex firstItem = s.GetElement(0);
               if (IsAtomic(firstItem))
               {
                  if (TryCompileMacroInvocation(ce, s, firstItem.String()))
                  {
                     const ISExpression* transform = s.GetTransform();
                     if (transform != NULL)
                     {
                        return TryCompileAssignArchetype(ce, *transform, type, allowClosures);
                     }
                     else
                     {
                        Throw(s, ("Macro expansion did not yield an arithmetic expression"));
                     }
                  }
               }

               return false;
            }

            if (s.NumberOfElements() == 1)
            {
               cr_sex onlyChild = s.GetElement(0);
               return TryCompileAssignArchetype(ce, onlyChild, type, allowClosures);
            }
         }
         else if (IsAtomic(s))
         {
            cstr token = s.String()->Buffer;

            MemberDef def;
            if (ce.Builder.TryGetVariableByName(def, token))
            {
               if (def.ResolvedType != &type)
               {
                  sexstringstream<1024> streamer;
                  streamer.sb << ("Cannot assign from (") << GetFriendlyName(*def.ResolvedType) << (" ") << token << (")");
                  streamer.sb << (". Exepcted type: ") << GetFriendlyName(type);
                  Throw(s, streamer);
               }

               if (!allowClosures && def.CapturesLocalVariables)
               {
                  sexstringstream<1024> streamer;
                  streamer.sb << ("Cannot assign from (") << GetFriendlyName(*def.ResolvedType) << (" ") << token << (")");
                  streamer.sb << (". Closures cannot be persisted.");
                  Throw(s, streamer);
               }

               ce.Builder.AssignVariableToTemp(token, Rococo::ROOT_TEMPDEPTH);
               return true;
            }
            else
            {
               const IFunction& fCandidate = MustMatchFunction(GetModule(ce.Script), s, token);

               CodeSection section;
               fCandidate.Code().GetCodeSection(section);

               VariantValue fnRef;
               fnRef.byteCodeIdValue = section.Id;
               ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D7, fnRef, BITCOUNT_64);

               VariantValue zeroRef;
               zeroRef.charPtrValue = nullptr;
               ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D14, zeroRef, BITCOUNT_64);

               return true;
            }
         }

         return false;
      }

      bool TryCompileArithmeticExpression(CCompileEnvironment& ce, cr_sex s, bool expected, VARTYPE type)
      {
         if (type == VARTYPE_Closure)
         {
            Throw(s, ("Internal compiler error. TryAssignToArchetype was the correct call."));
         }

         if (IsCompound(s))
         {
            if (TryCompileFunctionCallAndReturnValue(ce, s, type, NULL, NULL))
            {
               return true;
            }

            if (s.NumberOfElements() > 0)
            {
               cr_sex firstItem = s.GetElement(0);
               if (IsAtomic(firstItem))
               {
                  if (TryCompileMacroInvocation(ce, s, firstItem.String()))
                  {
                     const ISExpression* transform = s.GetTransform();
                     if (transform != NULL)
                     {
                        return TryCompileArithmeticExpression(ce, *transform, expected, type);
                     }
                     else
                     {
                        Throw(s, ("Macro expansion did not yield an arithmetic expression"));
                     }
                  }
               }
            }

            if (s.NumberOfElements() == 1)
            {
               cr_sex onlyChild = s.GetElement(0);
               return TryCompileArithmeticExpression(ce, onlyChild, expected, type);
            }

            if (s.NumberOfElements() == 3)
            {
               cr_sex left = s.GetElement(0);
               cr_sex opExpr = s.GetElement(1);
               cr_sex right = s.GetElement(2);

               if (IsArithmeticOperator(opExpr))
               {
                  ARITHMETIC_OP op = GetArithmeticOp(opExpr);
                  // Add code to evaluate an expression and copy the result to the tempIndex
                  CompileBinaryArithmeticExpression(ce, s, left, op, right, type);
                  return true;
               }
               else
               {
                  if (expected)
                  {
                     Throw(s, ("Cannot parse as a numeric valued expression"));
                  }
                  // No arithmetic operator
                  return false;
               }
            }
            else if (s.NumberOfElements() == 2)
            {
               cr_sex commandExpr = GetAtomicArg(s, 0);
               sexstring command = commandExpr.String();

               if (AreEqual(command, ("sizeof")))
               {
                  if (type != VARTYPE_Int32)
                  {
                     sexstringstream<1024> streamer;
                     streamer.sb << ("Type mismatch. The 'sizeof' operator evaluates to an Int32");
                     Throw(s, *streamer.sb);
                  }

                  cr_sex valueExpr = GetAtomicArg(s, 1);
                  CompileSizeOfExpression(ce, valueExpr);
                  return true;
               }

               MemberDef def;
               if (ce.Builder.TryGetVariableByName(OUT def, command->Buffer))
               {
                  if (def.ResolvedType == &ce.StructArray())
                  {
                     CompileGetArrayElement(ce, s.GetElement(1), command->Buffer, type, NULL);
                     return true;
                  }

                  if (def.ResolvedType == &ce.StructMap())
                  {
                     CompileGetMapElement(ce, s.GetElement(1), command->Buffer, type, NULL);
                     return true;
                  }

                  if (expected)
                  {
                     sexstringstream<1024> streamer;
                     streamer.sb << ("'") << command->Buffer << ("' recognized as ") << GetFriendlyName(*def.ResolvedType) << (" which does not yield an arithmetical value in this context");
                     Throw(s, streamer);
                  }
               }

               if (expected)
               {
                  sexstringstream<1024> streamer;
                  streamer.sb << ("Cannot interpet token as arithmetic valued expression");
                  Throw(commandExpr, streamer);
               }

               return false;
            }
            else
            {
               if (expected)
               {
                  sexstringstream<1024> streamer;
                  streamer.sb << ("Could not determine meaning of expression. Check identifiers and syntax are valid");
                  Throw(s, streamer);
               }
               // All arithmetic expressions have 3 elements
               return false;
            }
         }
         else if (IsAtomic(s))
         {
            cstr token = s.String()->Buffer;

            VariantValue value;
            if (Parse::TryParse(OUT value, IN type, IN token) == Parse::PARSERESULT_GOOD)
            {
               ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D7, value, GetBitCount(type));
               return true;
            }
            else
            {
               VARTYPE tokenType = ce.Builder.GetVarType(token);
               if (type == tokenType)
               {
                  ce.Builder.AssignVariableToTemp(token, Rococo::ROOT_TEMPDEPTH);
                  return true;
               }
               else
               {
                  if (tokenType == VARTYPE_Bad)
                  {
                     if (TryCompileFunctionCallAndReturnValue(ce, s, type, NULL, NULL))
                     {
                        return true;
                     }
                  }

                  if (expected)
                  {
                     sexstringstream<1024> streamer;
                     if (type == VARTYPE_Derivative)
                     {
                        streamer.sb << ("Expected arithmetic expression, but found a derived type not of the same type as the assignment");
                     }
                     else if (tokenType != VARTYPE_Bad)
                     {
                        streamer.sb << ("'") << token << ("' was a ") << GetTypeName(tokenType) << (" but expression requires ") << GetTypeName(type);
                     }
                     else
                     {
                        streamer.sb << ("Expected arithmetic expression, but found an unknown identifier");
                     }
                     Throw(s, streamer);
                  }
                  // not a boolean
                  return false;
               }
            }
         }
         else if (s.Type() == EXPRESSION_TYPE_NULL)
         {
            if (expected)
            {
               sexstringstream<1024> streamer;
               streamer.sb << ("Expected numeric expression, of type ") << GetTypeName(type) << (" but saw a null expression");
               Throw(s, streamer);
            }
            // not an atomic or a compound
            return false;
         }
         else
         {
            if (expected)
            {
               sexstringstream<1024> streamer;
               streamer.sb << ("Expected numeric expression, of type ") << GetTypeName(type);
               Throw(s, streamer);
            }
            // not an atomic or a compound
            return false;
         }
      }//Script
   }//Sexy
}