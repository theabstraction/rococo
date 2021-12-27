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

#include "sexy.vm.stdafx.h"
#include "sexy.vm.cpu.h"

#define ROCOCO_USE_SAFE_V_FORMAT
#include <rococo.strings.h>

#include <stdarg.h>

using namespace Rococo;
using namespace Rococo::VM;

namespace
{
	typedef void (*FN_FormatInstruction)(const Ins& ins, OUT IDisassembler::Rep& rep);

	struct FormatBinding
	{
		FN_FormatInstruction Formatter;
		cstr OpcodeText;
	};

	FormatBinding CreateBinding(FN_FormatInstruction f, cstr text)
	{
		FormatBinding fb;
		fb.Formatter = f;
		fb.OpcodeText = text;
		return fb;
	}

	void format(IDisassembler::Rep& rep, cstr fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		SafeVFormat(rep.ArgText, sizeof(rep.ArgText), fmt, args);
		va_end(args);
	}

   FormatBinding s_formatters[Opcodes::MAX_OPCODES] = { {0} };

#define EnableFormatter(x) s_formatters[Opcodes::x] = CreateBinding(Format##x, (#x));

	struct REGNAME
	{
		char buf[8];
	};

   REGNAME names[256] = { { {0} } };

	cstr RegisterName(DINDEX index)
	{
		if (names[0].buf[0] == 0)
		{
			for(int i = 0; i < 256; ++i)
			{
				SafeFormat(names[i].buf, 6, "D%d", i);
			}
		}

		switch(index)
		{
		case 0: return ("PC");
		case 1: return ("SP");
		case 2: return ("SF");
		case 3: return ("SR");
		default: return names[index].buf;
		}
	}

	void FormatSetRegisterImmediate64(const Ins& I, OUT IDisassembler::Rep& rep)
	{		
		auto& args = (ArgsSetRegister64&) I;
		format(rep, ("%s = #0x%I64x (%I64d)"), RegisterName(args.reg), args.value, args.value);
		rep.ByteCount = sizeof(args);
	}

	void FormatSetRegisterImmediate32(const Ins& I, OUT IDisassembler::Rep& rep)
	{		
		auto& args = (ArgsSetRegister32&)I;
		format(rep, ("%s = #0x%x (%d)"), RegisterName(args.reg), args.value, args.value);
		rep.ByteCount = sizeof(args);
	}

	void FormatExit(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		format(rep, ("%s"), RegisterName(I.Opmod1));
		rep.ByteCount = 2;
	}

	void FormatYield(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		format(rep, (""));
		rep.ByteCount = 1;
	}

	void FormatNoOperation(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		format(rep, (""));
		rep.ByteCount = 1;
	}

	void FormatTest64(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		auto& args = (ArgsOperateOnRegister&)I;
		format(rep, ("%s"), RegisterName(args.reg));
		rep.ByteCount = sizeof(args);
	}

	void FormatMove32(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		format(rep, ("%s=%s"), RegisterName(I.Opmod2), RegisterName(I.Opmod1));
		rep.ByteCount = 3;
	}

	void FormatMove64(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		format(rep, ("%s=%s"), RegisterName(I.Opmod2), RegisterName(I.Opmod1));
		rep.ByteCount = 3;
	}

	void FormatPoke64(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		int offset = *((int32*) (I.ToPC() + 3));
		format(rep, ("(@%s+%d) = %s"), RegisterName(I.Opmod2), offset, RegisterName(I.Opmod1));
		rep.ByteCount = 7;
	}

	void FormatPoke32(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		int offset = *((int32*) (I.ToPC() + 3));
		format(rep, ("(@%s+%d) = %s"), RegisterName(I.Opmod2), offset, RegisterName(I.Opmod1));
		rep.ByteCount = 7;
	}

	void FormatPushIndirect(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		size_t nBytes = *((size_t*) (I.ToPC() + 3));

		format(rep, ("*%s=*%s %Iu"), RegisterName(I.Opmod2), RegisterName(I.Opmod1), nBytes);

		rep.ByteCount = 3 + sizeof(size_t);
	}

	void FormatPushImmediate32(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		const uint8* pc = (const uint8*) &I;
		const int32* pArg = (const int32*) (pc+1);

		format(rep, ("*SP=%x"), *pArg);

		rep.ByteCount = 5;
	}

	void FormatPushImmediate64(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		const uint8* pc = (const uint8*)&I;
		const int64* pArg = (const int64*)(pc + 1);

		format(rep, ("*SP=%llx"), *pArg);

		rep.ByteCount = 9;
	}

	void FormatPushStackVariable32(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		auto& args = (ArgsPushStackVariable&)I;
		format(rep, ("SF(%d)"), args.sfOffset);
		rep.ByteCount = sizeof(args);
	}

	void FormatPushStackVariable64(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		auto& args = (ArgsPushStackVariable&)I;
		format(rep, ("SF(%d)"), args.sfOffset);
		rep.ByteCount = sizeof(args);
	}

	void FormatPushStackFrameMemberPtr(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		auto& args = (ArgsPushStackFrameMemberPtr&)I;
		format(rep, ("SF(%d.%d)"), args.sfOffset, args.memberOffset);
		rep.ByteCount = sizeof(args);
	}

	void FormatPushStackAddress(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		auto& args = (ArgsPushStackVariable&)I;
		format(rep, ("SF(%d)"), args.sfOffset);
		rep.ByteCount = sizeof(ArgsPushStackVariable);
	}

	void FormatPushRegister64(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		format(rep, ("%s"), RegisterName(I.Opmod1));
		rep.ByteCount = 2;
	}

	void FormatPushRegister32(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		format(rep, ("%s"), RegisterName(I.Opmod1));
		rep.ByteCount = 2;
	}

	void FormatPushAddress(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		format(rep, ("%s+%s"), RegisterName(I.Opmod1), RegisterName(I.Opmod2));
		rep.ByteCount = 3;
	}

	void FormatAddImmediate(const Ins& I, OUT IDisassembler::Rep& rep)
	{	
		const uint8* arg = I.ToPC() + 4;

		switch((BITCOUNT) I.Opmod2)
		{
		case BITCOUNT_32:
			{
				int32 value = *((int32*) arg);
				format(rep, ("%s.%d=%s+%d"), RegisterName(I.Opmod3), I.Opmod2, RegisterName(I.Opmod1), value);
				rep.ByteCount = 8;
			}
			break;
		case BITCOUNT_64:
			{
				int64 value = *((int64*) arg);
				format(rep, ("%s.%d=%s+%I64d"), RegisterName(I.Opmod3), I.Opmod2, RegisterName(I.Opmod1),value);
				rep.ByteCount = 12;
			}
		default:
			return; // Bad
		}		
	}

	void FormatLogicalAND32(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		format(rep, ("%s=%s %s"), RegisterName(I.Opmod2-1), RegisterName(I.Opmod2), RegisterName(I.Opmod1));
		rep.ByteCount = 3;
	}

	void FormatLogicalOR32(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		format(rep, ("%s=%s %s"), RegisterName(I.Opmod2-1), RegisterName(I.Opmod2), RegisterName(I.Opmod1));
		rep.ByteCount = 3;
	}

	void FormatLogicalXOR32(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		format(rep, ("%s=%s %s"), RegisterName(I.Opmod2-1), RegisterName(I.Opmod2), RegisterName(I.Opmod1));
		rep.ByteCount = 3;
	}

	void FormatLogicalNOT32(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		format(rep, ("%s=%s"), RegisterName(I.Opmod2), RegisterName(I.Opmod1));
		rep.ByteCount = 3;
	}

	void FormatLogicalAND64(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		format(rep, ("%s=%s %s"), RegisterName(I.Opmod2-1), RegisterName(I.Opmod2), RegisterName(I.Opmod1));
		rep.ByteCount = 3;
	}

	void FormatLogicalOR64(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		format(rep, ("%s=%s %s"), RegisterName(I.Opmod2-1), RegisterName(I.Opmod2), RegisterName(I.Opmod1));
		rep.ByteCount = 3;
	}

	void FormatLogicalXOR64(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		format(rep, ("%s=%s %s"), RegisterName(I.Opmod2-1), RegisterName(I.Opmod2), RegisterName(I.Opmod1));
		rep.ByteCount = 3;
	}

	void FormatLogicalNOT64(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		format(rep, ("%s=%s"), RegisterName(I.Opmod2), RegisterName(I.Opmod1));
		rep.ByteCount = 3;
	}

	void FormatShiftLeft32(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		format(rep, ("%s=%s %u"), RegisterName(I.Opmod1-1), RegisterName(I.Opmod1), (uint32) I.Opmod2);
		rep.ByteCount = 3;
	}

	void FormatShiftLeft64(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		format(rep, ("%s=%s %u"), RegisterName(I.Opmod1-1), RegisterName(I.Opmod1), (uint32) I.Opmod2);
		rep.ByteCount = 3;
	}

	void FormatShiftRight32(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		format(rep, ("%s=%s %u"), RegisterName(I.Opmod1-1), RegisterName(I.Opmod1), (uint32) I.Opmod2);
		rep.ByteCount = 3;
	}

	void FormatShiftRight64(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		format(rep, ("%s=%s %u"), RegisterName(I.Opmod1-1), RegisterName(I.Opmod1), (uint32) I.Opmod2);
		rep.ByteCount = 3;
	}

	void FormatPop(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		format(rep, ("%u"), (size_t) I.Opmod1);
		rep.ByteCount = 2;
	}

	void FormatStackAllocBig(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		int32 nBytes = *(int32*) (I.ToPC()+1);
		format(rep, ("%Id"), (ptrdiff_t) nBytes);
		rep.ByteCount = 5;
	}

	void FormatStackAlloc(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		format(rep, ("%Iu"), (size_t) I.Opmod1);
		rep.ByteCount = 2;
	}

	void FormatIntAdd64(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		format(rep, ("%s=%s %s"), RegisterName(I.Opmod1-1), RegisterName(I.Opmod2), RegisterName(I.Opmod1));
		rep.ByteCount = 4;
	}

	void FormatIntAdd32(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		format(rep, ("%s=%s %s"), RegisterName(I.Opmod1-1), RegisterName(I.Opmod2), RegisterName(I.Opmod1));
		rep.ByteCount = 3;
	}

	void FormatIntSubtract64(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		format(rep, ("%s=%s-%s"), RegisterName(I.Opmod1-1), RegisterName(I.Opmod1), RegisterName(I.Opmod2));
		rep.ByteCount = 3;
	}

	void FormatIntSubtract32(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		format(rep, ("%s=%s-%s"), RegisterName(I.Opmod1-1), RegisterName(I.Opmod1), RegisterName(I.Opmod2));
		rep.ByteCount = 3;
	}

	void FormatIntMultiply32(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		format(rep, ("%s=%s %s"), RegisterName(I.Opmod1-1), RegisterName(I.Opmod2), RegisterName(I.Opmod1));
		rep.ByteCount = 3;
	}

	void FormatIntMultiply64(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		format(rep, ("%=%s %s"), RegisterName(I.Opmod1-1), RegisterName(I.Opmod2), RegisterName(I.Opmod1));
		rep.ByteCount = 4;
	}

	void FormatIntDivide64(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		format(rep, ("%s=%s/%s"), RegisterName(I.Opmod1-1), RegisterName(I.Opmod1), RegisterName(I.Opmod2));
		rep.ByteCount = 4;
	}

	void FormatIntDivide32(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		format(rep, ("%s=%s/%s"), RegisterName(I.Opmod1-1), RegisterName(I.Opmod1), RegisterName(I.Opmod2));
		rep.ByteCount = 3;
	}

	void FormatIntNegate64(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		format(rep, ("%s=-%s"), RegisterName(I.Opmod2), RegisterName(I.Opmod1));
		rep.ByteCount = 4;
	}

	void FormatIntNegate32(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		format(rep, ("%s=-%s"), RegisterName(I.Opmod2), RegisterName(I.Opmod1));
		rep.ByteCount = 4;
	}

	void FormatFloatAdd(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		format(rep, ("%s=%s %s "), RegisterName(I.Opmod1-1), RegisterName(I.Opmod2), RegisterName(I.Opmod1));
		rep.ByteCount = 4;
	}

	void FormatFloatSubtract(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		format(rep, ("%s=%s-%s "), RegisterName(I.Opmod1-1), RegisterName(I.Opmod1), RegisterName(I.Opmod2));
		rep.ByteCount = 4;
	}

	void FormatFloatMultiply(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		format(rep, ("%s=%s %s "), RegisterName(I.Opmod1-1), RegisterName(I.Opmod2), RegisterName(I.Opmod1));
		rep.ByteCount = 4;
	}

	void FormatFloatDivide(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		format(rep, ("%s=%s/%s "), RegisterName(I.Opmod1-1), RegisterName(I.Opmod1), RegisterName(I.Opmod2));
		rep.ByteCount = 4;
	}

	void FormatDoubleAdd(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		format(rep, ("%s=%s %s "), RegisterName(I.Opmod1-1), RegisterName(I.Opmod2), RegisterName(I.Opmod1));
		rep.ByteCount = 4;
	}

	void FormatDoubleSubtract(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		format(rep, ("%s=%s %s "), RegisterName(I.Opmod1-1), RegisterName(I.Opmod2), RegisterName(I.Opmod1));
		rep.ByteCount = 4;
	}

	void FormatDoubleMultiply(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		format(rep, ("%s=%s %s "), RegisterName(I.Opmod1-1), RegisterName(I.Opmod2), RegisterName(I.Opmod1));
		rep.ByteCount = 4;
	}

	void FormatDoubleDivide(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		format(rep, ("%s=%s %s "), RegisterName(I.Opmod1-1), RegisterName(I.Opmod2), RegisterName(I.Opmod1));
		rep.ByteCount = 4;
	}

	void FormatCallBy(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		rep.ByteCount = 2;
		format(rep, (" %s"), RegisterName(I.Opmod1));
	}

	void FormatCallById(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		rep.ByteCount = 1 + sizeof(ID_BYTECODE);
		ID_BYTECODE* pID = (ID_BYTECODE*) (I.ToPC() + 1);
		format(rep, (" %u"), *pID);
	}

	void FormatCallByIdIndirect(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		rep.ByteCount = 2;
		format(rep, (" %s"), RegisterName(I.Opmod1));
	}

	void FormatCallVitualFunctionViaRefOnStack(const Ins& I, OUT IDisassembler::Rep& rep)
	{		
		int32 sfOffset = *((int32*) (I.ToPC() + 1));
		int32 methodIndex = *((int32*) (I.ToPC() + 5));
		int32 memberOffset = *((int32*) (I.ToPC() + 9));
		format(rep, (" SF(%d.%d) #%d"), sfOffset, memberOffset, methodIndex);
		
		rep.ByteCount = 13;
	}

	void FormatCallVirtualFunctionByAddress(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		rep.ByteCount = 9;
		int32 sfOffset = *((int32*) (I.ToPC() + 1));
		int32 methodIndex = *((int32*) (I.ToPC() + 5));
		format(rep, (" SF+%d #%d"), sfOffset, methodIndex);
	}

	void FormatInvokeBy(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		rep.ByteCount = 2;
		format(rep, (" %s"), RegisterName(I.Opmod1));
	}

	void FormatCall(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		rep.ByteCount = 5;
		int32* offsetPtr = (int32*)(I.ToPC() + 1);
		format(rep, ("%d"), *offsetPtr);
	}

	void FormatReturn(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		rep.ByteCount = 1;
		format(rep, (""));
	}

	void FormatSetIf32(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		rep.ByteCount = 3;

		cstr thecase;

		switch((CONDITION) I.Opmod1)
		{
		case CONDITION_IF_EQUAL:
			thecase = ("=");
			break;
		case CONDITION_IF_GREATER_OR_EQUAL:
			thecase=(">=");
			break;
		case CONDITION_IF_GREATER_THAN:
			thecase=(">");
			break;
		case CONDITION_IF_LESS_OR_EQUAL:
			thecase=(">=");
			break;
		case CONDITION_IF_LESS_THAN:
			thecase=("<");
			break;
		case CONDITION_IF_NOT_EQUAL:
			thecase=("!=");
			break;
		default:
			rep.ByteCount = 0;
			return;
		}

		format(rep, ("%s %s"), thecase, RegisterName(I.Opmod2));
	}

	void FormatSetIf64(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		rep.ByteCount = 3;

		cstr thecase;

		switch((CONDITION) I.Opmod1)
		{
		case CONDITION_IF_EQUAL:
			thecase = ("=");
			break;
		case CONDITION_IF_GREATER_OR_EQUAL:
			thecase=(">=");
			break;
		case CONDITION_IF_GREATER_THAN:
			thecase=(">");
			break;
		case CONDITION_IF_LESS_OR_EQUAL:
			thecase=(">=");
			break;
		case CONDITION_IF_LESS_THAN:
			thecase=("<");
			break;
		case CONDITION_IF_NOT_EQUAL:
			thecase=("!=");
			break;
		default:
			rep.ByteCount = 0;
			return;
		}

		format(rep, ("%s %s"), thecase, RegisterName(I.Opmod2));
	}

	void FormatBranch(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		rep.ByteCount = 5;
		const int32* pIntArg = (const int32*)(I.ToPC() + 1); // Look beyond the 2nd byte of the test instruction for the offset value
		int32 offset = *pIntArg;

		format(rep, ("%Id"), (ptrdiff_t) offset);
	}

	void FormatBranchIf(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		rep.ByteCount = 6;
		const int32* pIntArg = (const int32*)(I.ToPC() + 2); // Look beyond the 2nd byte of the test instruction for the offset value
		int32 offset = *pIntArg;

		cstr thecase;

		switch((CONDITION) I.Opmod1)
		{
		case CONDITION_IF_EQUAL:
			thecase = ("=");
			break;
		case CONDITION_IF_GREATER_OR_EQUAL:
			thecase=(">=");
			break;
		case CONDITION_IF_GREATER_THAN:
			thecase=(">");
			break;
		case CONDITION_IF_LESS_OR_EQUAL:
			thecase=(">=");
			break;
		case CONDITION_IF_LESS_THAN:
			thecase=("<");
			break;
		case CONDITION_IF_NOT_EQUAL:
			thecase=("!=");
			break;
		default:
			rep.ByteCount = 0;
			return;
		}

		format(rep, ("%s %Id"), thecase, (ptrdiff_t) offset);
	}

	void FormatBranchIfGTE(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		auto& args = (ArgsBranchIf&) I;

		rep.ByteCount = sizeof(ArgsBranchIf);
		int32 offset = args.PCoffset;

		format(rep, ("%d"), offset);
	}

	void FormatBranchIfGT(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		auto& args = (ArgsBranchIf&)I;
		rep.ByteCount = sizeof(ArgsBranchIf);
		int32 offset = args.PCoffset;

		format(rep, ("%d"), offset);
	}

	void FormatBranchIfLT(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		auto& args = (ArgsBranchIf&)I;
		rep.ByteCount = sizeof(ArgsBranchIf);
		int32 offset = args.PCoffset;

		format(rep, ("%d"), offset);
	}

	void FormatBranchIfNE(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		auto& args = (ArgsBranchIf&)I;
		rep.ByteCount = sizeof(ArgsBranchIf);
		int32 offset = args.PCoffset;
		format(rep, ("%d"), offset);
	}

	void FormatAddQuick32(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		format(rep, ("%s %d"), RegisterName(I.Opmod1), (int32)(int8)(I.Opmod2));
		rep.ByteCount = 3;
	}

	void FormatIncrement32(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		auto& args = (ArgsOperateOnRegister&)I;
		format(rep, ("%s"), RegisterName(args.reg));
		rep.ByteCount = sizeof(args);
	}

	void FormatDecrement32(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		auto& args = (ArgsOperateOnRegister&)I;
		format(rep, ("%s"), RegisterName(args.reg));
		rep.ByteCount = sizeof(args);
	}

	void FormatSetStackFrameImmediate32(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		int32 offset = (int32) (int8) I.Opmod1;
		uint32 value = *(uint32*) (I.ToPC()+2);
		format(rep, ("%d=#%X"), offset, value);
		rep.ByteCount = 6;
	}

	void FormatSetSFValueFromSFValue32(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		int32 trgOffset = (int32) (int8) I.Opmod1;
		int32 srcOffset = (int32) (int8) I.Opmod2;

		format(rep, ("*[SF%+d]=*[SF%+d]"), trgOffset, srcOffset);
		rep.ByteCount = 3;
	}

	void FormatSetStackFrameImmediate64(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		int32 offset = (int32) (int8) I.Opmod1;
		uint64 value = *(uint64*) (I.ToPC()+2);
		format(rep, ("%d=#%I64x"), offset, value);
		rep.ByteCount = 10;
	}

	void FormatSetStackFrameImmediateFar(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		BITCOUNT bits = (BITCOUNT) I.Opmod1;
		int32 offset = *(int32*) (I.ToPC() + 2);

		const uint8* arg = I.ToPC() + 6;

		switch(bits)
		{
		case BITCOUNT_32:
			{
				rep.ByteCount = 10;
				uint32 value = *(uint32*)arg;
				format(rep, ("%d=#%x"), offset, value);
				break;
			}
		case BITCOUNT_64:
			{
				rep.ByteCount = 14;
				uint64 value = *(uint64*)arg;
				format(rep, ("%d=#%I64x"), offset, value);
				break;
			}
      default:
         format(rep, ("Bad bitcount"));
         break;
		}
	}

	void FormatGetStackFrameValue32(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		int32 offset = (int32) (int8) I.Opmod1;
		format(rep, ("%s=@%d"), RegisterName(I.Opmod2), offset);
		rep.ByteCount = 3;
	}

	void FormatSetSFMemberPtrFromD5(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		int32 offset = *(int32*)(I.ToPC() + 1);
		int32 memberOffset = *(int32*)(I.ToPC() + 5);
		format(rep, ("@%d.*%d=D5"), offset, memberOffset);
		rep.ByteCount = 9;
	}

	void FormatGetStackFrameValueAndExtendToPointer(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		const int32* pOffset = (const int32*) (I.ToPC() + 2);
		format(rep, ("%s=@%d"), RegisterName(I.Opmod1), *pOffset);
		rep.ByteCount = 6;
	}

	void FormatGetStackFrameValue64(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		int32 offset = (int32) (int8) I.Opmod1;
		format(rep, ("%s=@%d"), RegisterName(I.Opmod2), offset);
		rep.ByteCount = 3;
	}

	void FormatGetStackFrameValueFar(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		BITCOUNT bits = (BITCOUNT) I.Opmod1;
		cstr regname =  RegisterName(I.Opmod2);
		int32 offset = *(int32*) (I.ToPC() + 3);
		rep.ByteCount = 7;

		format(rep, ("%s.%d=@%d"), regname, bits, offset);
	}

	void FormatSetStackFrameValue32(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		int32 offset = (int32) (int8) I.Opmod1;
		format(rep, ("@%d=%s"), offset, RegisterName(I.Opmod2));
		rep.ByteCount = 3;
	}

	void FormatSetStackFrameValue64(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		int32 offset = (int32) (int8) I.Opmod1;
		format(rep, ("@%d=%s"), offset, RegisterName(I.Opmod2));
		rep.ByteCount = 3;
	}

	void FormatSetSFMemberByRefFromSFByValue32(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		int32 targetSF = (int8) I.Opmod1;
		int32 targetMemberSF =(int8) I.Opmod2;
		int32 sourceSF = (int8) I.Opmod3;

		format(rep, ("SF(%d.%d)=SF(%d) 32-bit"), targetSF, targetMemberSF, sourceSF);

		rep.ByteCount += 4;
	}

	void FormatSetSFMemberByRefFromSFByValue64(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		int32 targetSF = (int8)I.Opmod1;
		int32 targetMemberSF = (int8)I.Opmod2;
		int32 sourceSF = (int8)I.Opmod3;

		format(rep, ("SF(%d.%d)=SF(%d) 64-bit"), targetSF, targetMemberSF, sourceSF);

		rep.ByteCount += 4;
	}

	void FormatSetSFValueFromSFMemberByRef32(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		int32 sourceSF = (int8) I.Opmod1;		
		int32 sourceMemberSF = (int8) I.Opmod2;
		int32 targetSF = (int8) I.Opmod3;

		format(rep, ("SF(%d)=SF(%d.%d)"), targetSF, sourceSF, sourceMemberSF);

		rep.ByteCount += 4;
	}

	void FormatSetSFMemberByRefFromRegister32(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		int32 SFoffset = (int8) I.Opmod2;		
		int32 memberOffset = (int8) I.Opmod3;

		format(rep, ("SF(%d.%d)=%s"), SFoffset, memberOffset, RegisterName(I.Opmod1));

		rep.ByteCount += 4;
	}

	void FormatSetSFMemberByRefFromRegisterLong(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		int8 srcRegister = (int8)I.Opmod1;
		int32 bitcount = (int8)I.Opmod2;

		int32 SFoffset = *(int32*)(I.ToPC() + 3);
		int32 memberOffset = *(int32*)(I.ToPC() + 7);

		format(rep, ("SF(%d.%d)=%s (%d bits"), SFoffset, memberOffset, RegisterName(srcRegister), bitcount);

		rep.ByteCount += 11;
	}

	void FormatSetStackFrameValueFar(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		BITCOUNT bits = (BITCOUNT) I.Opmod1;
		cstr regname =  RegisterName(I.Opmod2);
		int32 offset = *(int32*) (I.ToPC() + 3);
		rep.ByteCount = 7;

		format(rep, ("%u.%d=%s"), offset, bits, regname);
	}

	void FormatSwap(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		format(rep, ("%s %s"), RegisterName(I.Opmod1), RegisterName(I.Opmod2));
		rep.ByteCount = 3;
	}

	void FormatTest32(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		auto& args = (ArgsOperateOnRegister&)I;
		format(rep, ("%s"), RegisterName(args.reg));
		rep.ByteCount = sizeof(args);
	}

	void FormatSaveRegister32(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		format(rep, ("%s"), RegisterName(I.Opmod1));
		rep.ByteCount = 2;
	}

	void FormatRestoreRegister32(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		format(rep, ("%s"), RegisterName(I.Opmod1));
		rep.ByteCount = 2;
	}

	void FormatSaveRegister64(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		format(rep, ("%s"), RegisterName(I.Opmod1));
		rep.ByteCount = 2;
	}

	void FormatRestoreRegister64(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		format(rep, ("%s"), RegisterName(I.Opmod1));
		rep.ByteCount = 2;
	}

	struct MemCopyInfo
	{
		int TargetOffset;
		int SourceOffset;
		size_t ByteCount;
	};

	void GetMemCopyInfo(const Ins& ins, OUT MemCopyInfo& info, OUT IDisassembler::Rep& rep)
	{
		const uint8* pc = (const uint8*) &ins;
		pc++;

		const int32* pOffset = (const int32*) pc;
		info.TargetOffset = *pOffset;
		pc += 4;

		pOffset = (const int32*) pc;
		info.SourceOffset = *pOffset;

		pc += 4;

		const size_t* pByteCount = (const size_t*) pc;
		pc += sizeof(size_t);

		info.ByteCount = *pByteCount;

		rep.ByteCount = 9 + sizeof(size_t);
	}

	void GetMemCopyNearInfo(const Ins& ins, OUT MemCopyInfo& info, OUT IDisassembler::Rep& rep)
	{
		info.TargetOffset = (int32)(int8) ins.Opmod1;
		info.SourceOffset = (int32)(int8) ins.Opmod2;
		info.ByteCount = (size_t) ins.Opmod3;			
		rep.ByteCount = 4;
	}

	void FormatCopySFMemory(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		MemCopyInfo mci;
		GetMemCopyInfo(I, OUT mci, OUT rep);
		format(rep, ("SF+%d to SF+%d %Id bytes"), mci.SourceOffset, mci.TargetOffset, mci.ByteCount);
	}

	void FormatCopySFMemoryNear(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		MemCopyInfo mci;
		GetMemCopyNearInfo(I, OUT mci, OUT rep);
		format(rep, ("SF+%d to SF+%d %Id bytes"), mci.SourceOffset, mci.TargetOffset, mci.ByteCount);
	}

	void FormatCopyMemory(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		format(rep, ("*%s to *%s, %Id bytes"), RegisterName(I.Opmod2), RegisterName(I.Opmod1), (size_t) I.Opmod3);
		rep.ByteCount = 4;
	}

	void FormatCopy32Bits(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		format(rep, ("*%s to *%s"), RegisterName(I.Opmod2), RegisterName(I.Opmod1));
		rep.ByteCount = 3;
	}

	void FormatCopy64Bits(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		format(rep, ("*%s to *%s"), RegisterName(I.Opmod2), RegisterName(I.Opmod1));
		rep.ByteCount = 3;
	}

	void FormatCopyMemoryBig(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		const size_t* pByteCount = (const size_t*) (((const uint8*) &I)+3);
		format(rep, ("*%s to *%s, %Id bytes"), RegisterName(I.Opmod2), RegisterName(I.Opmod1), *pByteCount);
		rep.ByteCount = 3 + sizeof(size_t);
	}

	void FormatIncrementPtrBig(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		const int32* pDelta = (const int32*)((const uint8*) &I)+3;
		format(rep, ("%s by %d"), RegisterName(I.Opmod1), *pDelta);
		rep.ByteCount = 3;
	}

	void FormatBooleanNot(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		format(rep, ("BooleanNot %s"), RegisterName(I.Opmod1));
		rep.ByteCount = 2;
	}

	void FormatIncrementPtr(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		format(rep, ("%s by %d"), RegisterName(I.Opmod1), (int32)(int8) I.Opmod2);
		rep.ByteCount = 3;
	}

	void FormatGetStackFrameMemberPtr(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		int SFoffset =  (int32)(int8) I.Opmod2;
		int memberOffset = (int32)(int8) I.Opmod3;
	//	char sign = SFoffset >= 0 ? '+' : '-';

		format(rep, ("%s=SF(%d->%d)"), RegisterName(I.Opmod1), SFoffset, memberOffset);
		rep.ByteCount = 4;
	}

	void FormatGetStackFrameMember32(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		int SFoffset =  *(const int32*)(((const uint8*) &I)+2);
		int memberOffset = *(const int32*)(((const uint8*) &I)+6);
		format(rep, ("%s=*SF(%d->%d)"), RegisterName(I.Opmod1), SFoffset, memberOffset);
		rep.ByteCount = 10;
	}

	void FormatGetStackFrameMember64(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		int SFoffset =  *(const int32*)(((const uint8*) &I)+2);
		int memberOffset = *(const int32*)(((const uint8*) &I)+6);

		format(rep, ("%s=*SF(%d->%d)"), RegisterName(I.Opmod1), SFoffset, memberOffset);
		rep.ByteCount = 10;
	}

	void FormatGetStackFrameMemberPtrFar(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		int SFoffset =  *(const int32*)(((const uint8*) &I)+2);
		int memberOffset = *(const int32*)(((const uint8*) &I)+6);
		format(rep, ("%s=SF(%d->%d)"), RegisterName(I.Opmod1), SFoffset, memberOffset);
		rep.ByteCount = 10;	
	}

	void FormatGetStackFrameAddress(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		int offset =  *(const int32*)(((const uint8*) &I)+2);
		format(rep, ("%s=SF(%d)"), RegisterName(I.Opmod1), offset);
		rep.ByteCount = 6;	
	}

	void FormatDebug(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		format(rep, "");
		rep.ByteCount = 1;	
	}

	void FormatGetGlobal(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		int offset = *(const int32*)(((const uint8*)&I) + 2);
		format(rep, ("D4.%d=global+%d"), I.Opmod1, offset);
		rep.ByteCount = 6;
	}

	void FormatSetGlobal(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		int offset = *(const int32*)(((const uint8*)&I) + 2);
		format(rep, ("D4.%d=global+%d"), I.Opmod1, offset);
		rep.ByteCount = 6;
	}

	void FormatSetSFMemberRefFromSFMemberByRef64(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		auto& args = *(IAssemblerBuilder::Args_SetMemberRefFromSFMemberByRef*) (&I);
		format(rep, ("SF %s.%s= SF %s.%s"), args.targetSFOffset, args.targetMemberOffset, args.sourceSFOffset, args.sourceMemberOffset);
		rep.ByteCount = sizeof(args);
	}

	void FormatCallVitualFunctionViaMemberOffsetOnStack(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		auto& args = (ArgsCallVirtualFunctionViaMemberOffsetOnStack&)I;
		format(rep, ("(%d.%d) #%d"), args.SFoffsetToStruct, args.memberOffsetToInterfaceRef, args.vTableOffset);
		rep.ByteCount = sizeof(args);
	}

	void FormatGetStackFrameMemberPtrAndDeref(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		auto& args = (ArgsGetStackFrameMemberPtrAndDeref&)I;
		format(rep, ("D%d = *(%d.%d)"), args.dtarget, args.sfToStructRef, args.structToMemberOffset);
		rep.ByteCount = sizeof(args);
	}

	void FormatCopySFVariableFromRef(const Ins& I, OUT IDisassembler::Rep& rep)
	{
#pragma	pack(push, 1)
		struct Args // This is how OPCODES should have been implemented. MAT
		{
			uint8 instruction;
			int32 targetSFOffset;
			int32 sourcePtrSFOffset;
			int32 sourceMemberOffset;
			size_t nBytesSource;
		};
#pragma pack(pop)
		const Args& args = (const Args&) I;
		format(rep, ("SF(%d)=(*SF(%d).%d). %lu bytes"), args.targetSFOffset, args.sourcePtrSFOffset, args.sourceMemberOffset, args.nBytesSource);
		rep.ByteCount = sizeof(Args);
	}

	void FormatDereferenceD4(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		format(rep, (""));
		rep.ByteCount = 1;
	}

	void FormatSetSFValueFromSFValueLong(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		auto& args = (ArgsSetSFValueFromSFValue&)I;
		format(rep, "SF(%d)=SF(%d) %d bytes", args.sfTargetOffset, args.sfSourceOffset, args.byteCount);
		rep.ByteCount = sizeof(ArgsSetSFValueFromSFValue);
	}

	void FormatSetSFValueFromSFMemberByRef(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		auto& args = (ArgsSetSFValueFromSFMemberRef&)I;
		format(rep, "SF(%d)=SF(%d.%d) %d bytes", args.targetSFOffset, args.srcSFOffset, args.srcMemberOffset, args.nBytesSource);
		rep.ByteCount = sizeof(ArgsSetSFValueFromSFMemberRef);
	}

	void FormatSetSFMemberRefFromSFValue(const Ins& I, OUT IDisassembler::Rep& rep)
	{
		auto& args = (ArgsSetSFMemberRefFromSFValue&)I;
		format(rep, "SF(%d.%d)=SF(%d) %d bytes", args.targetSFOffset, args.targetMemberOffset, args.SFSourceValueOffset, args.nBytesSource);
		rep.ByteCount = sizeof(ArgsSetSFMemberRefFromSFValue);
	}

	void BuildFormatTable()
	{
		EnableFormatter(SetSFValueFromSFMemberByRef);
		EnableFormatter(BooleanNot);
		EnableFormatter(CopySFMemory);
		EnableFormatter(CopySFMemoryNear);
		EnableFormatter(CopyMemoryBig);
		EnableFormatter(CopyMemory);
		EnableFormatter(Copy32Bits);
		EnableFormatter(Copy64Bits);
		EnableFormatter(CopySFVariableFromRef);
		EnableFormatter(SetRegisterImmediate64);
		EnableFormatter(SetRegisterImmediate32);
		EnableFormatter(IncrementPtr);
		EnableFormatter(IncrementPtrBig);
		EnableFormatter(Exit);
		EnableFormatter(Yield);
		EnableFormatter(NoOperation);
		EnableFormatter(Test64);
		EnableFormatter(Move32);
		EnableFormatter(Move64);
		EnableFormatter(Poke64);
		EnableFormatter(Poke32);
		EnableFormatter(PushIndirect);
		EnableFormatter(PushRegister64);
		EnableFormatter(PushRegister32);
		EnableFormatter(PushAddress);
		EnableFormatter(AddImmediate);
		EnableFormatter(LogicalAND32);
		EnableFormatter(LogicalOR32);
		EnableFormatter(LogicalXOR32);
		EnableFormatter(LogicalNOT32);
		EnableFormatter(LogicalAND64);
		EnableFormatter(LogicalOR64);
		EnableFormatter(LogicalXOR64);
		EnableFormatter(LogicalNOT64);
		EnableFormatter(ShiftLeft32);
		EnableFormatter(ShiftLeft64);
		EnableFormatter(ShiftRight32);		
		EnableFormatter(ShiftRight64);
		EnableFormatter(PushImmediate32);
		EnableFormatter(PushImmediate64);
		EnableFormatter(PushStackVariable32);
		EnableFormatter(PushStackVariable64);
		EnableFormatter(PushStackFrameMemberPtr);
		EnableFormatter(PushStackAddress);
		EnableFormatter(Pop);
		EnableFormatter(StackAllocBig);
		EnableFormatter(StackAlloc);
		EnableFormatter(IntAdd64);
		EnableFormatter(IntAdd32);
		EnableFormatter(IntSubtract64);
		EnableFormatter(IntSubtract32);
		EnableFormatter(IntMultiply64);
		EnableFormatter(IntMultiply32);
		EnableFormatter(IntDivide64);
		EnableFormatter(IntDivide32);
		EnableFormatter(IntNegate64);
		EnableFormatter(IntNegate32);
		EnableFormatter(FloatAdd);
		EnableFormatter(FloatSubtract);
		EnableFormatter(FloatMultiply);
		EnableFormatter(FloatDivide);
		EnableFormatter(DoubleAdd);
		EnableFormatter(DoubleSubtract);
		EnableFormatter(DoubleMultiply);
		EnableFormatter(DoubleDivide);
		EnableFormatter(CallBy);
		EnableFormatter(CallByIdIndirect);
		EnableFormatter(CallVitualFunctionViaRefOnStack);
		EnableFormatter(CallVirtualFunctionByAddress);
		EnableFormatter(CallById);
		EnableFormatter(Call);
		EnableFormatter(InvokeBy);
		EnableFormatter(Return);
		EnableFormatter(BranchIf);
		EnableFormatter(BranchIfGTE);
		EnableFormatter(BranchIfGT);
		EnableFormatter(BranchIfLT);
		EnableFormatter(BranchIfNE);
		EnableFormatter(AddQuick32);
		EnableFormatter(Increment32);
		EnableFormatter(Decrement32);
		EnableFormatter(SetIf32);
		EnableFormatter(SetIf64);
		EnableFormatter(Test32);
		EnableFormatter(Swap);
		EnableFormatter(SetStackFrameImmediate32);
		EnableFormatter(SetSFValueFromSFValue32);
		EnableFormatter(SetStackFrameImmediate64);
		EnableFormatter(SetStackFrameImmediateFar);
		EnableFormatter(GetStackFrameValue32);
		EnableFormatter(GetStackFrameValue64);
		EnableFormatter(GetStackFrameValueFar);
		EnableFormatter(GetStackFrameMemberPtr);
		EnableFormatter(GetStackFrameMember32);
		EnableFormatter(GetStackFrameMember64);
		EnableFormatter(GetStackFrameMemberPtrFar);
		EnableFormatter(GetStackFrameAddress);
		EnableFormatter(SetStackFrameValue32);
		EnableFormatter(SetStackFrameValue64);
		EnableFormatter(SetStackFrameValueFar);
		EnableFormatter(SetSFMemberByRefFromSFByValue32);
		EnableFormatter(SetSFValueFromSFMemberByRef32);
		EnableFormatter(SetSFMemberByRefFromRegister32);
		EnableFormatter(SetSFMemberByRefFromRegisterLong);
		EnableFormatter(SaveRegister32);
		EnableFormatter(RestoreRegister32);
		EnableFormatter(SaveRegister64);
		EnableFormatter(RestoreRegister64);
		EnableFormatter(Branch);
		EnableFormatter(Debug);
		EnableFormatter(GetGlobal);
		EnableFormatter(SetGlobal);
		EnableFormatter(GetStackFrameValueAndExtendToPointer);
		EnableFormatter(SetSFMemberByRefFromSFByValue64);
		EnableFormatter(SetSFMemberRefFromSFMemberByRef64);
		EnableFormatter(DereferenceD4);
		EnableFormatter(CallVitualFunctionViaMemberOffsetOnStack);
		EnableFormatter(GetStackFrameMemberPtrAndDeref);
		EnableFormatter(SetSFValueFromSFValueLong);
		EnableFormatter(SetSFMemberRefFromSFValue);
		EnableFormatter(SetSFMemberPtrFromD5);
	}

	class Disassembler final: public IDisassembler
	{
	private:
		ICore& core;

	public:
		Disassembler(ICore& _core): core(_core)
		{	
			if (s_formatters[0].Formatter == NULL)
			{
				BuildFormatTable();				
			}
		}

		virtual void Free() 
		{
			delete this;
		}

		virtual void Disassemble(const uint8* PC, OUT Rep& rep) 
		{
			const Ins* ins = (const Ins*) PC;
			memset(&rep, 0, sizeof(Rep));

			if (ins->Opcode == Opcodes::Invoke)
			{
				rep.ByteCount = 1 + sizeof(ID_API_CALLBACK);
				ID_API_CALLBACK *pID = (ID_API_CALLBACK *) (ins->ToPC() + 1);
				cstr symbol = core.GetCallbackSymbolName(*pID);
				CopyString(rep.OpcodeText, MAX_ARG_LEN, ("Invoke"));
				format(rep, ("%s"), symbol != NULL ? symbol : ("<Unknown id>"));
			}
			else
			{
				FormatBinding& fb = s_formatters[ins->Opcode];
				if (fb.Formatter != NULL)
				{
					CopyString(rep.OpcodeText, MAX_ARG_LEN, fb.OpcodeText);
					fb.Formatter(*ins, OUT rep);
				}
				else
				{
					CopyString(rep.OpcodeText, MAX_ARG_LEN, ("UnknownOpcode"));
				}
			}
		}
	};
}

namespace Rococo { namespace VM
{
	IDisassembler* CreateDisassembler(ICore& core)
	{
		return new Disassembler(core);
	}
}}