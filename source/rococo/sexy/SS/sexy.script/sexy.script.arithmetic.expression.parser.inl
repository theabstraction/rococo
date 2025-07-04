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

		SexyVarType GetBestGuessType(SexyVarType a, SexyVarType b)
		{
			if (a == SexyVarType_Derivative && b == SexyVarType_Derivative) return SexyVarType_Derivative;
			if (a == SexyVarType_Float64 || b == SexyVarType_Float64) return SexyVarType_Float64;
			if (a == SexyVarType_Float32 || b == SexyVarType_Float32) return SexyVarType_Float32;
			if (a == SexyVarType_Int64 || b == SexyVarType_Int64) return SexyVarType_Int64;
			if (a == SexyVarType_Int32 || b == SexyVarType_Int32) return SexyVarType_Int32;
			return SexyVarType_Bad;
		}

		SexyVarType GuessType(cr_sex s, ICodeBuilder& builder)
		{
			if (IsAtomic(s))
			{
				cstr token = s.c_str();
				SexyVarType type = builder.GetVarType(token);
				if (type != SexyVarType_Bad) return type;
				return Parse::GetLiteralType(token);
			}
			else if (!IsCompound(s))
			{
				return SexyVarType_Bad;
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
						SexyVarType lType = GuessType(s.GetElement(0), builder);
						SexyVarType rType = GuessType(s.GetElement(2), builder);
						return GetBestGuessType(lType, rType);
					}
					else if (IsBinaryBooleanOperator(s.GetElement(1)))
					{
						return SexyVarType_Bool;
					}
					else
					{
						return SexyVarType_Bad;
					}
				}
				else
				{
					return SexyVarType_Bad;
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
		}

		void ComputeArithmeticLiteralVsLiteral(OUT VariantValue& result, cr_sex parent, const VariantValue& a, ARITHMETIC_OP op, const VariantValue& b, SexyVarType type)
		{
			switch (type)
			{
			case SexyVarType_Int32:
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
			case SexyVarType_Int64:
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
			case SexyVarType_Float32:
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
			case SexyVarType_Float64:
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

		SCRIPTEXPORT_API cstr GetTypeName(SexyVarType type)
		{
			switch (type)
			{
			case SexyVarType_Bad: return ("bad");
			case SexyVarType_Derivative: return ("derived");
			case SexyVarType_Int32: return ("Int32");
			case SexyVarType_Int64: return ("Int64");
			case SexyVarType_Float32: return ("Float32");
			case SexyVarType_Float64: return ("Float64");
			case SexyVarType_Bool: return ("Bool");
			case SexyVarType_Pointer: return ("Pointer");
			case SexyVarType_Closure: return ("Archetype");
			case SexyVarType_Array: return "Array";
			case SexyVarType_List: return "List";
			case SexyVarType_Map: return "Map";
			default: return ("unknown-type");
			}
		}

		void ValidateArithmeticVariable(cr_sex parent, cstr token, ICodeBuilder& builder, SexyVarType type, cstr helper)
		{
			SexyVarType rType = builder.GetVarType(token);
			if (rType == SexyVarType_Bad)
			{
				SexyVarType literalType = Parse::GetLiteralType(token);
				if (IsPrimitiveType(literalType))
				{
					Throw(parent, "%s was %s. Expecting %s", token, GetTypeName(literalType), GetTypeName(type));
				}
				else
				{
					Throw(parent, "%s was not a literal type or a known identifier", token);
				}
			}
			else if (rType == SexyVarType_Derivative)
			{
				Throw(parent, "%s was a derived type, it cannot be used directly in arithmetic expressions", token);
			}

			if (rType != type)
			{
				Throw(parent, "%s was not the same type as that of the arithmetic expression in which it was referenced", token);
			}
		}

		void AppendArithmeticOp(VM::IAssembler& assembler, cr_sex src, ARITHMETIC_OP op, SexyVarType type, int tempDepth)
		{
			VM::DINDEX reg = VM::REGISTER_D4 + tempDepth;
			VM::FLOATSPEC spec = VM::FLOATSPEC_DOUBLE;

			switch (type)
			{
			case SexyVarType_Int32:
			case SexyVarType_Int64:
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
			case SexyVarType_Float32:
				spec = VM::FLOATSPEC_SINGLE;
			case SexyVarType_Float64:
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
				Throw(src, "Unhandled type in arithmetic expression");
			}

		Error:
			Throw(src, "Unhandled operator in arithmetic expression");
		}

		void GetAtomicValue(CCompileEnvironment& ce, cr_sex parent, cstr id, SexyVarType type)
		{
			if (IsCapital(id[0]))
			{
				IFunctionBuilder& f = MustMatchFunction(ce.Builder.Module(), parent, id);
				if (!IsGetAccessor(f) && f.GetArgument(0).VarType() != type)
				{
					Throw(parent, "Expecting variable or single valued function with no inputs of return type %s", Parse::VarTypeName(type));
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
							Throw(parent, "Expecting method call %s", Parse::VarTypeName(type));
						}
					}
					else
					{
						MemberDef def;
						if (ce.Builder.TryGetVariableByName(OUT def, id))
						{
							if (def.ResolvedType->VarType() != type)
							{
								Throw(parent, "Exepecting %s, but %s is of type %s", Rococo::Script::GetTypeName(type), id, def.ResolvedType->Name());
							}
							ce.Builder.AssignVariableToTemp(id, Rococo::ROOT_TEMPDEPTH);
						}
						else
						{
							ThrowTokenNotFound(parent, instance, ce.Builder.Owner().Name(), ("variable"));
						}
					}
				}
				else if (ce.Builder.GetVarStructure(id) != NULL)
				{
					ce.Builder.AssignVariableToTemp(id, Rococo::ROOT_TEMPDEPTH);
				}
				else if (*id == '-')
				{
					AddSymbol(ce.Builder, "%s -> D7 and negate", id+1);

					switch (ce.Builder.GetVarType(id + 1))
					{
					case SexyVarType_Int32:
						ce.Builder.AssignVariableToTemp(id + 1, Rococo::ROOT_TEMPDEPTH);
						ce.Builder.Assembler().Append_IntNegate(VM::REGISTER_D7, BITCOUNT_32);
						break;
					case SexyVarType_Float32:
						ce.Builder.AssignVariableToTemp(id + 1, Rococo::ROOT_TEMPDEPTH);
						ce.Builder.Assembler().Append_FloatNegate32(VM::REGISTER_D7);
						break;
					case SexyVarType_Int64:
						ce.Builder.AssignVariableToTemp(id + 1, Rococo::ROOT_TEMPDEPTH);
						ce.Builder.Assembler().Append_IntNegate(VM::REGISTER_D7, BITCOUNT_64);
						break;
					case SexyVarType_Float64:
						ce.Builder.AssignVariableToTemp(id + 1, Rococo::ROOT_TEMPDEPTH);
						ce.Builder.Assembler().Append_FloatNegate64(VM::REGISTER_D7);
						break;
					case SexyVarType_Bool:
						ce.Builder.AssignVariableToTemp(id + 1, Rococo::ROOT_TEMPDEPTH);
						ce.Builder.Assembler().Append_BooleanNot(VM::REGISTER_D7);
						break;
					case SexyVarType_Bad:
						Throw(parent, "Negation parsed, but the successive token characters did not parse as a known type");
						break;
					default:
						Throw(parent, "Negation parsed, but the variable %s was of type %s, which is not numeric.", GetFriendlyName(*ce.Builder.GetVarStructure(id + 1)));
					}
				}
				else
				{
					ThrowTokenNotFound(parent, id, ce.Builder.Owner().Name(), ("variable"));
				}				
			}
		}

		void CompileArithmeticLiteralVsVariable(CCompileEnvironment& ce, cr_sex parent, const VariantValue& value, ARITHMETIC_OP op, cstr varname, SexyVarType type, ARITHMETIC_ORDER order, cr_sex atomicExpr)
		{
			// ValidateArithmeticVariable(parent, varname, ce.Builder, type, order == ARITHMETIC_ORDER_LEFT_TO_RIGHT ? ("RHS") : ("LHS"));

			int A = order == ARITHMETIC_ORDER_LEFT_TO_RIGHT ? 1 : 2;
			int B = order == ARITHMETIC_ORDER_LEFT_TO_RIGHT ? 2 : 1;

			ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D7 + A, value, GetBitCount(type));

			GetAtomicValue(ce, atomicExpr, varname, type);
			ce.Builder.Assembler().Append_MoveRegister(VM::REGISTER_D7, VM::REGISTER_D7 + B, GetBitCount(type));
			AppendArithmeticOp(ce.Builder.Assembler(), parent, op, type, Rococo::ROOT_TEMPDEPTH);
		}

		void CompileArithmeticAtomicVsAtomic(CCompileEnvironment& ce, cr_sex parent, cstr left, ARITHMETIC_OP op, cstr right, SexyVarType type, cr_sex leftExpr, cr_sex rightExpr)
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
				ce.Builder.PopLastVariables(1, true);

				AppendArithmeticOp(ce.Builder.Assembler(), parent, op, type, Rococo::ROOT_TEMPDEPTH);
			}
		}

		void CompileAtomicSide(CCompileEnvironment& ce, cr_sex parent, cstr token, int tempDepth, ARITHMETIC_ORDER order, SexyVarType type)
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
				if (*token == '-')
				{
					// Negation
					ValidateArithmeticVariable(parent, token + 1, ce.Builder, type, order == ARITHMETIC_ORDER_LEFT_TO_RIGHT ? ("LHS") : ("RHS"));
					ce.Builder.AssignVariableToTemp(token + 1, tempDepth);

					int dx = VM::REGISTER_D4 + tempDepth;

					switch (type)
					{
					case SexyVarType_Int32:
						ce.Builder.Assembler().Append_IntNegate(dx, BITCOUNT_32);
						break;
					case SexyVarType_Int64:
						ce.Builder.Assembler().Append_IntNegate(dx, BITCOUNT_64);
						break;
					case SexyVarType_Float32:
						ce.Builder.Assembler().Append_FloatNegate32(dx);
						break;
					case SexyVarType_Float64:
						ce.Builder.Assembler().Append_FloatNegate64(dx);
						break;
					default:
						Throw(parent, "Cannot handle negation for variable %s of type %s", token + 1, GetFriendlyName(*ce.Builder.GetVarStructure(token + 1)));
					}
				}
				else
				{
					ValidateArithmeticVariable(parent, token, ce.Builder, type, order == ARITHMETIC_ORDER_LEFT_TO_RIGHT ? ("LHS") : ("RHS"));
					ce.Builder.AssignVariableToTemp(token, tempDepth);
				}
			}
		}

		void CompileArithmeticAtomicVsCompound(CCompileEnvironment& ce, cr_sex parent, cstr token, ARITHMETIC_OP op, cr_sex s, ARITHMETIC_ORDER order, SexyVarType type)
		{
			int A = order == ARITHMETIC_ORDER_LEFT_TO_RIGHT ? 1 : 2;
			int B = order == ARITHMETIC_ORDER_LEFT_TO_RIGHT ? 2 : 1;

			BITCOUNT bits = GetBitCount(type);

			if (B > A)
			{
				CompileAtomicSide(ce, parent, token, Rococo::ROOT_TEMPDEPTH + A, order, type);

				AddArchiveRegister(ce, Rococo::ROOT_TEMPDEPTH + A, Rococo::ROOT_TEMPDEPTH + A, bits);
				TryCompileArithmeticExpression(ce, s, true, type);
				ce.Builder.PopLastVariables(1, true);

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

		void CompileArithmeticCompoundVsCompound(CCompileEnvironment& ce, cr_sex parent, cr_sex left, ARITHMETIC_OP op, cr_sex right, SexyVarType type)
		{
			BITCOUNT bits = GetBitCount(type);

			TryCompileArithmeticExpression(ce, left, true, type);
			AddArchiveRegister(ce, Rococo::ROOT_TEMPDEPTH, Rococo::ROOT_TEMPDEPTH + 1, bits);
			TryCompileArithmeticExpression(ce, right, true, type);
			ce.Builder.Assembler().Append_MoveRegister(VM::REGISTER_D7, VM::REGISTER_D7 + 2, bits);
			ce.Builder.PopLastVariables(1, true);
			AppendArithmeticOp(ce.Builder.Assembler(), parent, op, type, Rococo::ROOT_TEMPDEPTH);
		}

		void CompileBinaryArithmeticExpression(CCompileEnvironment& ce, cr_sex parent, cr_sex left, ARITHMETIC_OP op, cr_sex right, SexyVarType type)
		{
			if (IsAtomic(left))
			{
				cstr lToken = left.c_str();

				if (IsAtomic(right))
				{
					cstr rToken = right.c_str();
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
				cstr rToken = right.c_str();
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

			if (input.ResolvedType()->VarType() != SexyVarType_Closure)
			{
				return NULL;
			}

			cstr name = input.Name();
			const IArchetype* a = input.ResolvedType()->Archetype();
			if (a == NULL)
			{
				Throw(s, "Error, expecting archetype for variable %s in %s", name, f.Name());
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

			cstr value = valueExpr.c_str();

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
				ce.Builder.AssignVariableToTemp(value, Rococo::ROOT_TEMPDEPTH);
				ce.Builder.Append_GetAllocSize();
			}
		}

		void CompileSizeOfType(CCompileEnvironment& ce, cr_sex valueExpr)
		{
			cstr value = valueExpr.c_str();

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

			cstr value = valueExpr.c_str();

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
				if (TryCompileFunctionCallAndReturnValue(ce, s, SexyVarType_Closure, &type, type.Archetype()))
				{
					// Functions and methods cannot return closures, so no need to check allowClosures flag
					return true;
				}
			}
			else if (IsAtomic(s))
			{
				cstr token = s.c_str();

				MemberDef def;
				if (ce.Builder.TryGetVariableByName(def, token))
				{
					if (def.ResolvedType != &type)
					{
						Throw(s, "Cannot assign from %s %s. Exepcted type: %s", GetFriendlyName(*def.ResolvedType), token, GetFriendlyName(type));
					}

					if (!allowClosures && def.CapturesLocalVariables)
					{
						Throw(s, "Cannot assign from %s. Closures cannot be persisted.", GetFriendlyName(*def.ResolvedType));
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

		VM_CALLBACK(StringIndexToChar)
		{
			int32 index = registers[7].int32Value;

			auto interf = (InterfacePointer)registers[4].vPtrValue;
			auto* strInstance = (CStringConstant*) InterfaceToInstance(interf);

			cstr text = strInstance->pointer;

			if (index < 0 || index >= strInstance->length)
			{
				auto* expression = (ISExpression*) registers[5].vPtrValue;
				if (strInstance->length <= 0)
				{
					Throw(*expression, "Index %d out of bounds. The string length was zero", index);
				}
				else
				{
					Throw(*expression, "Index %d out of bounds. Valid range at the point in execution is 0 <= index < %d", index, strInstance->length);
				}
			}

			registers[7].int32Value = text[index];
		}

		void CompileStringIndexToChar(CCompileEnvironment& ce, cr_sex s)
		{
			// (<string_variable> <index>) returns an Int32 to D7

			if (s.NumberOfElements() != 2)
			{
				Throw(s, "Expecting two elements ( (<string_variable> <index>) )");
			}

			cr_sex sName = s[0];

			AssertAtomic(sName);

			auto name = sName.c_str();

			MemberDef def;
			if (!ce.Builder.TryGetVariableByName(def, name))
			{
				Throw(sName, "Expecting variable");
			}

			if (def.ResolvedType != &ce.Object.Common().SysTypeIString().NullObjectType())
			{
				Throw(sName, "Expecting IString");
			}

			if (!TryCompileArithmeticExpression(ce, s[1], true, SexyVarType_Int32))
			{
				Throw(s[1], "Expecting Int32 valued expression as index for string");
			}

			// Code will now have put the index in D7
			ce.Builder.AssignVariableToTemp(name, 0); // String ref is now in D4

			VariantValue sValue;
			sValue.vPtrValue = const_cast<ISExpression*>(&s);
			ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D5, sValue, BITCOUNT_POINTER);

			ce.Builder.Assembler().Append_Invoke(ce.SS.GetScriptCallbacks().idStringIndexToChar);
		}

		void CompileTernaryExpression(CCompileEnvironment& ce, cr_sex s, SexyVarType type)
		{
			// s0  s1        s2                s3
			// (?? <boolean> <truth-expression><false-expression>)
			cr_sex sBoolean = s[1];
			cr_sex sForTrue = s[2];
			cr_sex sForFalse = s[3];

			bool negate = false;
			TryCompileBooleanExpression(ce, sBoolean, true, OUT negate);
			// boolean condition is now assembled to be a 32-bit value in D7
			
			ce.Builder.Assembler().Append_Test(VM::REGISTER_D7, BITCOUNT_32);

			size_t postConditionPos = ce.Builder.Assembler().WritePosition();
			ce.Builder.Assembler().Append_BranchIf(CONDITION_IF_NOT_EQUAL, 0);

			TryCompileArithmeticExpression(ce, sForTrue, true, type);
			// boolean condition is now assembled to be a 32-bit value in D7

			// If we have come here, it means we need to skip evaluating sForFalse
			size_t branchPos = ce.Builder.Assembler().WritePosition();
			ce.Builder.Assembler().Append_Branch(0);

			size_t falsePos = ce.Builder.Assembler().WritePosition();

			TryCompileArithmeticExpression(ce, sForFalse, false, type);
			// boolean condition is now assembled to be a 32-bit value in D7

			// Now we know the size of branches we can correct the branch statements
			size_t exitPos = ce.Builder.Assembler().WritePosition();
			ce.Builder.Assembler().SetWriteModeToOverwrite(branchPos);
			ce.Builder.Assembler().Append_Branch((int)(exitPos - branchPos));
			ce.Builder.Assembler().SetWriteModeToOverwrite(postConditionPos);
			ce.Builder.Assembler().Append_BranchIf(negate ? CONDITION_IF_NOT_EQUAL : CONDITION_IF_EQUAL, (int)(falsePos - postConditionPos));
			ce.Builder.Assembler().SetWriteModeToAppend();
		}

		bool TryCompileCompoundArithmeticExpression(CCompileEnvironment& ce, cr_sex s, bool expected, SexyVarType type)
		{
			if (s.NumberOfElements() == 3 && IsArithmeticOperator(s[1]))
			{
				// (x OP y) will never be a function call as OP is never a legal argument to a method
			}
			else if (TryCompileFunctionCallAndReturnValue(ce, s, type, NULL, NULL))
			{
				return true;
			}

			cr_sex firstItem = s.GetElement(0);
			
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
						Throw(s, "Cannot parse as a numeric valued expression");
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
					if (type != SexyVarType_Int32)
					{
						Throw(s, "Type mismatch. The 'sizeof' operator evaluates to an Int32");
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

					if (def.ResolvedType == &ce.Object.Common().SysTypeIString().NullObjectType())
					{
						if (type != SexyVarType_Int32)
						{
							Throw(s, "(<IString> <index>) returns an Int32, but return type was %s", GetTypeName(type));
						}

						CompileStringIndexToChar(ce, s);
						return true;
					}

					if (expected)
					{
						Throw(s, "'%s' recognized as a %s which does not yield an arithmetical value in this context", command->Buffer, GetFriendlyName(*def.ResolvedType));
					}
				}

				if (expected)
				{
					Throw(commandExpr, "Cannot interpet token as arithmetic valued expression");
				}

				return false;
			}
			else if (s.NumberOfElements() == 4)
			{
				if (IsAtomic(s[0]) && Eq(s[0].c_str(), "??"))
				{
					// Ternary expression (?? <boolean> <truth-expression><false-expression>)
					CompileTernaryExpression(ce, s, type);
					return true;
				}
			}

			if (s.NumberOfElements() > 3)
			{
				// Potentially we have (a + b + c + d...)
				if (IsArithmeticOperator(s[1]))
				{
					// use an expression proxy to covert to (a + (b + c + d...))
					auto& root = CreateExpressionProxy(ce, s, 3);
					root.SetChild(0, s[0]);
					root.SetChild(1, s[1]);
					auto& rhs = CreateExpressionProxy(ce, s[1], s.NumberOfElements() - 2);
					for (int i = 2; i < s.NumberOfElements(); i++)
					{
						rhs.SetChild(i - 2, s[i]);
					}
					root.SetChild(2, rhs.Outer());
					return TryCompileCompoundArithmeticExpression(ce, root.Outer(), expected, type);
				}
			}

			if (expected)
			{
				Throw(s, "Could not determine meaning of expression. Check identifiers and syntax are valid");
			}
			// All arithmetic expressions have 3 elements
			return false;
		}

		bool TryCompileAtomicArithmeticExpression(CCompileEnvironment& ce, cr_sex s, bool expected, SexyVarType type)
		{
			cstr token = s.c_str();

			VariantValue value;
			if (Parse::TryParse(OUT value, IN type, IN token) == Parse::PARSERESULT_GOOD)
			{
				ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D7, value, GetBitCount(type));
				return true;
			}
			else
			{
				SexyVarType tokenType = ce.Builder.GetVarType(token);
				if (type == tokenType)
				{
					ce.Builder.AssignVariableToTemp(token, Rococo::ROOT_TEMPDEPTH);
					return true;
				}
				else
				{
					if (tokenType == SexyVarType_Bad)
					{
						if (TryCompileFunctionCallAndReturnValue(ce, s, type, NULL, NULL))
						{
							return true;
						}
					}

					if (expected)
					{
						if (type == SexyVarType_Derivative)
						{
							Throw(s, "Expected arithmetic expression, but found a derived type not of the same type as the assignment");
						}
						else if (tokenType != SexyVarType_Bad)
						{
							Throw(s, "'%s' was a %s but expression requires %s", token, GetTypeName(tokenType), GetTypeName(type));
						}
						else
						{
							Throw(s, "Expected arithmetic expression, but found an unknown identifier");
						}
					}
					// not a boolean
					return false;
				}
			}
		}

		bool TryCompileArithmeticExpression(CCompileEnvironment& ce, cr_sex s, bool expected, SexyVarType type)
		{
			if (type == SexyVarType_Closure)
			{
				Throw(s, "Internal compiler error. TryAssignToArchetype was the correct call.");
			}

			if (IsCompound(s))
			{
				return TryCompileCompoundArithmeticExpression(ce, s, expected, type);
			}
			else if (IsAtomic(s))
			{
				return TryCompileAtomicArithmeticExpression(ce, s, expected, type);
			}
			else if (expected)
			{
				Throw(s, "Expected numeric expression, of type %s", GetTypeName(type));
			}
			
			// not an atomic or a compound
			return false;
		}//Script
	}//Sexy
}