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

namespace 
{
	bool IsBinaryCompareOperator(cr_sex s)
	{
		if (s.Type() == EXPRESSION_TYPE_ATOMIC)
		{
			sexstring op = s.String();
			if (AreEqual(op, SEXTEXT(">"))) return true;
			if (AreEqual(op, SEXTEXT("<")))	return true;
			if (AreEqual(op, SEXTEXT(">="))) return true;
			if (AreEqual(op, SEXTEXT("<="))) return true;
			if (AreEqual(op, SEXTEXT("!="))) return true;
			if (AreEqual(op, SEXTEXT("=="))) return true;				
		}

		return false;
	}

	bool IsBinaryBooleanOperator(cr_sex s)
	{
		if (s.Type() == EXPRESSION_TYPE_ATOMIC)
		{
			sexstring op = s.String();
			if (AreEqual(op, SEXTEXT("and"))) return true;
			if (AreEqual(op, SEXTEXT("or")))	return true;
			if (AreEqual(op, SEXTEXT("xor"))) return true;			
		}

		return false;
	}

	bool IsAGreaterThanB(const VariantValue& a, const VariantValue& b, VARTYPE type, cr_sex src)
	{
		switch(type)
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
		switch(type)
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
		switch(type)
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
		switch(type)
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
		switch(type)
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
		switch(type)
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

	CONDITION GetBinaryComparisonOp(cr_sex opExpr, bool negate)
	{
		sexstring op = opExpr.String();
		if (AreEqual(op, SEXTEXT(">"))) return negate? CONDITION_IF_LESS_OR_EQUAL : CONDITION_IF_GREATER_THAN;
		if (AreEqual(op, SEXTEXT("<")))	return negate? CONDITION_IF_GREATER_OR_EQUAL : CONDITION_IF_LESS_THAN;
		if (AreEqual(op, SEXTEXT(">="))) return negate? CONDITION_IF_LESS_THAN : CONDITION_IF_GREATER_OR_EQUAL;
		if (AreEqual(op, SEXTEXT("<="))) return negate? CONDITION_IF_GREATER_THAN : CONDITION_IF_LESS_OR_EQUAL;
		if (AreEqual(op, SEXTEXT("!="))) return negate? CONDITION_IF_EQUAL : CONDITION_IF_NOT_EQUAL;
		if (AreEqual(op, SEXTEXT("=="))) return negate? CONDITION_IF_NOT_EQUAL : CONDITION_IF_EQUAL;
		
		Throw(opExpr, SEXTEXT("Cannot interpret as a comparison operator"));
		return CONDITION_IF_EQUAL;
	}

	bool Compare(const VariantValue& a, const VariantValue& b, VARTYPE type, CONDITION op, cr_sex src)
	{
		switch(op)
		{
		case CONDITION_IF_GREATER_THAN:			return IsAGreaterThanB(a, b, type, src);
		case CONDITION_IF_LESS_THAN:			return IsALessThanB(a, b, type, src);
		case CONDITION_IF_GREATER_OR_EQUAL:		return IsAGreaterThanOrEqualToB(a, b, type, src);
		case CONDITION_IF_LESS_OR_EQUAL:		return IsALessThanOrEqualToB(a, b, type, src);
		case CONDITION_IF_NOT_EQUAL:			return IsANotEqualToB(a, b, type, src);
		case CONDITION_IF_EQUAL:				return IsAEqualToB(a, b, type, src);
		default: 
			Throw(src, SEXTEXT("Expecting binary boolean operator"));
			return false;
		}
	}

	enum LOGICAL_OP
	{
		LOGICAL_OP_AND,
		LOGICAL_OP_OR,
		LOGICAL_OP_XOR
	};

	LOGICAL_OP GetBinaryLogicalOp(cr_sex opExpr)
	{
		sexstring op = opExpr.String();
		if (AreEqual(op, SEXTEXT("and"))) return LOGICAL_OP_AND;
		if (AreEqual(op, SEXTEXT("or")))	return LOGICAL_OP_OR;
		if (AreEqual(op, SEXTEXT("xor"))) return LOGICAL_OP_XOR;

		Throw(opExpr, SEXTEXT("Cannot interpret as a binary logical operation"));
		return LOGICAL_OP_AND;
	}

	bool Compare(int a, int b, LOGICAL_OP op, cr_sex src)
	{
		switch(op)
		{
		case LOGICAL_OP_AND:		return a != 0 && b != 0;
		case LOGICAL_OP_OR:			return a != 0 || b != 0;
		case LOGICAL_OP_XOR:		return (a != 0 && b == 0) || (a == 0 && b != 0);

		default: 
			Throw(src, SEXTEXT("Expecting binary boolean operator"));
			return false;
		}
	}

	void AddBinaryComparison(cr_sex src, VM::IAssembler& assembler, int booleanTargetId, int sourceA, int sourceB, CONDITION op, VARTYPE type)
	{
		switch(type)
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

	void AddBinaryBoolean(cr_sex src, VM::IAssembler& assembler, int booleanTargetId, int sourceA, int sourceB, LOGICAL_OP op)
	{
		if (booleanTargetId != sourceA-1)
		{
			Throw(src, SEXTEXT("Algorthimic error: Expecting [booleanTargetId] to be [sourceA-1]"));
		}
	
		switch(op)
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
}