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

#include <sexy.types.h>
#include <Sexy.S-Parser.h>
#include <sexy.compiler.public.h>

using namespace Rococo::Sex;
using namespace Rococo::Compiler;

namespace Rococo
{
	namespace Variants
	{
		inline VariantValue FromValue(int32 value)
		{
			VariantValue v;
			v.int32Value = value;
			return v;
		}

		inline VariantValue Zero() { return FromValue(0);	}
		inline VariantValue ValueTrue() { return FromValue(1); }
		inline VariantValue ValueFalse() { return FromValue(0); }

		bool IsAssignableToBoolean(SexyVarType type)
		{
			return type == SexyVarType_Bool;
		}

		SEXYUTIL_API SexyVarType GetBestCastType(SexyVarType a, SexyVarType b)
		{
			if (a == SexyVarType_Bad || b == SexyVarType_Bad) return SexyVarType_Bad;
			if (a == SexyVarType_Derivative || b == SexyVarType_Derivative) return SexyVarType_Derivative;
			if (a == SexyVarType_Float64 || b == SexyVarType_Float64) return SexyVarType_Float64;
			if (a == SexyVarType_Float32 || b == SexyVarType_Float32) return SexyVarType_Float32;
			if (a == SexyVarType_Int64 || b == SexyVarType_Int64) return SexyVarType_Int64;
			if (a == SexyVarType_Int32 || b == SexyVarType_Int32) return SexyVarType_Int32;
			return SexyVarType_Bad;
		}

		bool TryCastBoolToInt(OUT VariantValue& result, SexyVarType type, cr_sex /* refExpr */, bool value)
		{
			switch(type)
			{
			case SexyVarType_Int32:
				if (value)
				{
					result.int32Value = 1;
				}
				else
				{
					result.int32Value = 0;
				}
				return true;
			case SexyVarType_Int64:
				if (value)
				{
					result.int64Value = 1;
				}
				else
				{
					result.int64Value = 0;
				}
				return true;
			default:
				return false;
			}
		}

		SEXYUTIL_API bool TryRecast(OUT VariantValue& end, IN const VariantValue& original, SexyVarType orignalType, SexyVarType endType)
		{
			if (!IsPrimitiveType(orignalType) || (!IsPrimitiveType(endType)))
			{
				return false;
			}

			if (orignalType == endType)
			{
				end = original;
				return true;
			}

			switch(orignalType)
			{
			case SexyVarType_Int32:
				switch(endType)
				{
				case SexyVarType_Int64:
					end.int64Value = (int64) original.int32Value;
					return true;
				case SexyVarType_Float32:
					end.floatValue = (float) original.int32Value;
					return true;
				case SexyVarType_Float64:
					end.doubleValue = (double) original.int32Value;
					return true;
				default:
					return false;
				}
			case SexyVarType_Int64:
				switch(endType)
				{
				case SexyVarType_Int32:
					end.int32Value = (int32) original.int64Value;
					return true;
				case SexyVarType_Float32:
					end.floatValue = (float) original.int64Value;
					return true;
				case SexyVarType_Float64:
					end.doubleValue = (double) original.int64Value;
					return true;
				default:
					return false;
				}
			case SexyVarType_Float32:
				switch(endType)
				{
				case SexyVarType_Int32:
					end.int32Value = (int32) original.floatValue;
					return true;
				case SexyVarType_Int64:
					end.int64Value = (int64) original.floatValue;
					return true;
				case SexyVarType_Float64:
					end.doubleValue = (double) original.floatValue;
					return true;
				default:
					return false;
				}
			case SexyVarType_Float64:
				switch(endType)
				{
				case SexyVarType_Int32:
					end.int32Value = (int32) original.doubleValue;
					return true;
				case SexyVarType_Int64:
					end.int64Value = (int64) original.doubleValue;
					return true;
				case SexyVarType_Float32:
					end.floatValue = (float) original.doubleValue;
					return true;
				default:
					return false;
				}
         default:
            return false;
			}
		}
	}
}