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
#include <Sexy.S-Parser.h>

#include <sexy.compiler.public.h>
#include <sexy.compiler.helpers.h>
#include <rococo.api.h>
#include <sexy.security.h>
#include "sexy.internal.api.h"
#include <rococo.sexy.api.h>
#include <rococo.os.h>
#include <rococo.io.h>
#include <sexy.types.h>

#include <algorithm>

using namespace Rococo;
using namespace Rococo::Script;
using namespace Rococo::Compiler;
using namespace Rococo::Sex;
using namespace Rococo::VM;
using namespace Rococo::Strings;

#include "sexy.script.util.inl"
#include "sexy.script.asserts.inl"
#include "sexy.script.functions.inl"
#include "sexy.script.macro.inl"
#include "sexy.script.matching.inl"
#include "sexy.script.factory.inl"
#include "sexy.script.closure.inl"
#include "sexy.script.array.inl"
#include "sexy.script.list.inl"
#include "sexy.script.map.inl"
#include "sexy.script.containers.inl"
#include "sexy.script.arithmetic.expression.parser.inl"
#include "sexy.script.predicates.expression.parser.inl"
#include "sexy.script.conditional.expression.parser.inl"
#include "sexy.script.exception.logic.inl"
#include "sexy.script.modules.inl"
#include "sexy.script.exceptions.inl"
#include "sexy.script.casts.inl"
#include "sexy.script.JIT.inl"
#include "sexy.script.stringbuilders.inl"

namespace Rococo::Script
{
	void RegisterMiscAPI(ScriptCallbacks& callbacks, VM::ICore& core, IScriptSystem& ss)
	{
		callbacks.idThrowNullRef = core.RegisterCallback(OnInvokeThrowNullRef, &ss, "ThrowNullRef");
		callbacks.idYieldMicroseconds = core.RegisterCallback(OnInvokeYieldMicroseconds, &ss.ProgramObject().VirtualMachine(), "YieldMicroseconds");
		callbacks.idDynamicDispatch = core.RegisterCallback(OnInvokeDynamicDispatch, &ss, "Dispatch");
		callbacks.idInvokeMethodByName = core.RegisterCallback(OnInvokeInvokeMethodByName, &ss, "InvokeMethod");
		callbacks.idVariableRefToType = core.RegisterCallback(OnInvokeGetTypeOfClassToD4, &ss, "GetTypeOfClassToD4");
		callbacks.idIsSameObject = core.RegisterCallback(OnInvokeIsSameObject, &ss, "IsSameObject");
		callbacks.idIsDifferentObject = core.RegisterCallback(OnInvokeIsDifferentObject, &ss, "IsDifferentObject");
		callbacks.idStringIndexToChar = core.RegisterCallback(OnInvokeStringIndexToChar, &ss, "StringIndexToChar");
		callbacks.idTransformAt_D4D5retIExpressionBuilderD7 = core.RegisterCallback(OnInvokeTransformAt_D4D5retD7, &ss, "TransformAt");
		callbacks.idTransformParent_D4retIExpressionBuilderD7 = core.RegisterCallback(OnInvokeTransformParent_D4retD7, &ss, "TransformParent");
		callbacks.idJumpFromProxyToMethod = core.RegisterCallback(OnInvokeJumpFromProxyToMethod, &ss, "JumpFromProxyToMethod");
	}

	void DefineSysNative(const INamespace& sysNative, IScriptSystem& ss, IStringPool* stringPool, TMemoAllocator& memoAllocator)
	{
		ss.AddNativeCall(sysNative, ::AlignedMalloc, &ss, "AlignedMalloc (Int32 capacity) (Int32 alignment)-> (Pointer data)", __FILE__, __LINE__, false, 0);
		ss.AddNativeCall(sysNative, ::AlignedFree, &ss, "AlignedFree (Pointer data)->", __FILE__, __LINE__, false, 0);
		ss.AddNativeCall(sysNative, DynamicCast, nullptr, "_DynamicCast (Pointer interface) (Pointer instanceRef) ->", __FILE__, __LINE__, false, 0);

		const INamespace& sysNativeStrings = ss.AddNativeNamespace("Sys.Strings.Native");
		ss.AddNativeCall(sysNativeStrings, NewStringBuilder, stringPool, "NewStringBuilder (Int32 capacity) -> (Sys.Type.IStringBuilder sb)", __FILE__, __LINE__, false, 0);
		ss.AddNativeCall(sysNativeStrings, DestructStringBuilder, stringPool, "DestructStringBuilder (Sys.Type.IStringBuilder sb)->", __FILE__, __LINE__, false, 0);
		ss.AddNativeCall(sysNativeStrings, CreateMemoString, &memoAllocator, "CreateMemoString (Sys.Type.IString s) -> (Pointer dest) (Int32 destLength)", __FILE__, __LINE__, false, 0);
		ss.AddNativeCall(sysNativeStrings, FreeMemoString, &memoAllocator, "FreeMemoString (Pointer src) ->", __FILE__, __LINE__, false, 0);
		ss.AddNativeCall(sysNativeStrings, StringCompare, NULL, "StringCompare  (Pointer s) (Pointer t) -> (Int32 diff)", __FILE__, __LINE__, false, 0);
		ss.AddNativeCall(sysNativeStrings, StringCompareI, NULL, "StringCompareI  (Pointer s) (Pointer t) -> (Int32 diff)", __FILE__, __LINE__, false, 0);
		ss.AddNativeCall(sysNativeStrings, StringFindLeft, NULL, "StringFindLeft (Pointer containerBuffer) (Int32 containerLength) (Int32 startPos) (Pointer substringBuffer) (Int32 substringLength) (Bool caseIndependent)-> (Int32 position)", __FILE__, __LINE__, false, 0);
		ss.AddNativeCall(sysNativeStrings, StringFindRight, NULL, "StringFindRight (Pointer containerBuffer) (Int32 containerLength) (Int32 leftPos) (Int32 rightPos) (Pointer substringBuffer) (Int32 substringLength) (Bool caseIndependent)-> (Int32 position)", __FILE__, __LINE__, false, 0);
		ss.AddNativeCall(sysNativeStrings, FastStringBuilderAppendIString, stringPool, "FastStringBuilderAppendIString (Sys.Type.IStringBuilder sb) (Pointer src) (Int32 srclength) ->", __FILE__, __LINE__, false, 0);
		ss.AddNativeCall(sysNativeStrings, FastStringBuilderThrowIfAppendWouldTruncate, stringPool, "FastStringBuilderThrowIfAppendWouldTruncate (Sys.Type.IStringBuilder sb) ->", __FILE__, __LINE__, false, 0);
		ss.AddNativeCall(sysNativeStrings, FastStringBuilderAppendChar, stringPool, "FastStringBuilderAppendChar (Sys.Type.IStringBuilder sb) (Int32 asciiValue) ->", __FILE__, __LINE__, false, 0);
		ss.AddNativeCall(sysNativeStrings, FastStringBuilderAppendInt32, stringPool, "FastStringBuilderAppendInt32 (Sys.Type.IStringBuilder sb) (Int32 x) ->", __FILE__, __LINE__, false, 0);
		ss.AddNativeCall(sysNativeStrings, FastStringBuilderAppendInt64, stringPool, "FastStringBuilderAppendInt64 (Sys.Type.IStringBuilder sb) (Int64 x) ->", __FILE__, __LINE__, false, 0);
		ss.AddNativeCall(sysNativeStrings, FastStringBuilderAppendFloat32, stringPool, "FastStringBuilderAppendFloat32 (Sys.Type.IStringBuilder sb) (Float32 x) ->", __FILE__, __LINE__, false, 0);
		ss.AddNativeCall(sysNativeStrings, FastStringBuilderAppendFloat64, stringPool, "FastStringBuilderAppendFloat64 (Sys.Type.IStringBuilder sb) (Float64 x) ->", __FILE__, __LINE__, false, 0);
		ss.AddNativeCall(sysNativeStrings, FastStringBuilderAppendBool, stringPool, "FastStringBuilderAppendBool (Sys.Type.IStringBuilder sb) (Bool x) ->", __FILE__, __LINE__, false, 0);
		ss.AddNativeCall(sysNativeStrings, FastStringBuilderAppendPointer, stringPool, "FastStringBuilderAppendPointer (Sys.Type.IStringBuilder sb) (Pointer x) ->", __FILE__, __LINE__, false, 0);
		ss.AddNativeCall(sysNativeStrings, FastStringBuilderClear, stringPool, "FastStringBuilderClear (Sys.Type.IStringBuilder sb) ->", __FILE__, __LINE__, false, 0);
		ss.AddNativeCall(sysNativeStrings, FastStringBuilderAppendAsDecimal, stringPool, "FastStringBuilderAppendAsDecimal (Sys.Type.IStringBuilder sb) ->", __FILE__, __LINE__, false, 0);
		ss.AddNativeCall(sysNativeStrings, FastStringBuilderAppendAsHex, stringPool, "FastStringBuilderAppendAsHex (Sys.Type.IStringBuilder sb) -> ", __FILE__, __LINE__, false, 0);
		ss.AddNativeCall(sysNativeStrings, FastStringBuilderAppendAsSpec, stringPool, "FastStringBuilderAppendAsSpec (Sys.Type.IStringBuilder sb) (Int32 type) -> ", __FILE__, __LINE__, false, 0);
		ss.AddNativeCall(sysNativeStrings, FastStringBuilderSetFormat, stringPool, "FastStringBuilderSetFormat (Sys.Type.IStringBuilder sb) (Int32 precision) (Int32 width) (Bool isZeroPrefixed) (Bool isRightAligned)->", __FILE__, __LINE__, false, 0);
		ss.AddNativeCall(sysNativeStrings, FastStringBuilderStripLeft, stringPool, "FastStringBuilderStripLeft (Sys.Type.IStringBuilder sb) (Int32 leftPos)->", __FILE__, __LINE__, false, 0);
		ss.AddNativeCall(sysNativeStrings, FastStringBuilderAppendSubstring, stringPool, "FastStringBuilderAppendSubstring (Sys.Type.IStringBuilder sb) (Pointer s) (Int32 sLen) (Int32 startPos) (Int32 charsToAppend) ->", __FILE__, __LINE__, false, 0);
		ss.AddNativeCall(sysNativeStrings, FastStringBuilderSetLength, stringPool, "FastStringBuilderSetLength (Sys.Type.IStringBuilder sb)(Int32 length) ->", __FILE__, __LINE__, false, 0);
		ss.AddNativeCall(sysNativeStrings, FastStringBuilderSetCase, stringPool, "FastStringBuilderSetCase (Sys.Type.IStringBuilder sb) (Int32 start) (Int32 end) (Bool toUpper)->", __FILE__, __LINE__, false, 0);
		ss.AddNativeCall(sysNativeStrings, StringEndsWith, NULL, "StringEndsWith (IString bigString)(IString suffix) -> (Bool isSo)", __FILE__, __LINE__, false, 0);
		ss.AddNativeCall(sysNativeStrings, StringStartsWith, NULL, "StringStartsWith (IString bigString)(IString prefix) -> (Bool isSo)", __FILE__, __LINE__, false, 0);
		ss.AddNativeCall(sysNativeStrings, FastStringBuilderMakeSysSlashes, stringPool, "MakeSysSlashes (Sys.Type.IStringBuilder sb) ->", __FILE__, __LINE__, false, 0);
		ss.AddNativeCall(sysNativeStrings, FastStringBuilderReplace, stringPool, "FastStringBuilderReplace (Sys.Type.IStringBuilder sb)(Int32 startPosition)(IString from)(IString to) ->", __FILE__, __LINE__, false, 0);
	}

	void DefineSysTypeStrings(const INamespace& sysTypeStrings, IScriptSystem& ss)
	{
		ss.AddNativeCall(sysTypeStrings, SysTypeStrings::IsUpperCase, nullptr, "IsUpperCase (Int32 c)->(Bool isSo)", __FILE__, __LINE__, false, 0);
		ss.AddNativeCall(sysTypeStrings, SysTypeStrings::IsLowerCase, nullptr, "IsLowerCase (Int32 c)->(Bool isSo)", __FILE__, __LINE__, false, 0);
		ss.AddNativeCall(sysTypeStrings, SysTypeStrings::IsAlpha, nullptr, "IsAlpha (Int32 c)->(Bool isSo)", __FILE__, __LINE__, false, 0);
		ss.AddNativeCall(sysTypeStrings, SysTypeStrings::IsNumeric, nullptr, "IsNumeric (Int32 c)->(Bool isSo)", __FILE__, __LINE__, false, 0);
		ss.AddNativeCall(sysTypeStrings, SysTypeStrings::IsAlphaNumeric, nullptr, "IsAlphaNumeric (Int32 c)->(Bool isSo)", __FILE__, __LINE__, false, 0);
		ss.AddNativeCall(sysTypeStrings, AssertPascalCaseNamespace, NULL, "AssertPascalCaseNamespace (Sys.Type.IString s) (Int32 maxLength)->", __FILE__, __LINE__, false, 0);
	}
}