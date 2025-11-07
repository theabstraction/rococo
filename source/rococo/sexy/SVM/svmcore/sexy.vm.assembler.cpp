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

	3. THERE IS NO WARRANTY FOR THE PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE LAW. EXCEPT WHEN OTHERWISE STATED IN WRITING THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES PROVIDE THE PROGRAM 'AS IS' WITHOUT
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

#include <sexy.vector.h>

using namespace Rococo;
using namespace Rococo::VM;

namespace
{
	typedef TSexyVector<unsigned char> TMemory;

	bool IsToInt8Lossless(int32 value)
	{
		return value >= -128 && value <= 127;
	}

	class Assembler: public IAssembler
	{
	private:
		TMemory program;
		ICore& core;
		size_t nextWritePosition;

		template<class T>
		void AddArgument(T value)
		{
			const uint8* p = reinterpret_cast<const uint8*> ( &value );

			if (nextWritePosition == (size_t) -1)
			{
				for(size_t i = 0; i < sizeof(T); ++i)
				{
					program.push_back(*p++);
				}
			}
			else
			{
				for(size_t i = 0; i < sizeof(T); ++i)
				{
					program[nextWritePosition++] = *p++;
				}
			}
		}

		void AddInstruction(const Ins* pInst, size_t len)
		{
			const uint8* p = reinterpret_cast<const uint8*> ( pInst );

			if (nextWritePosition == (size_t) -1)
			{
				for(size_t i = 0; i < len; ++i)
				{
					program.push_back(*p++);
				}
			}
			else
			{
				for(size_t i = 0; i < len; ++i)
				{
					program[nextWritePosition++] = *p++;
				}
			}
		}

		void AddSingleByteInstruction(Opcodes::OPCODE opcode)
		{
			Ins i;
			i.Opcode = (uint8) opcode;
			AddInstruction(&i, 1);
		}

		void AddTwoByteInstruction(Opcodes::OPCODE opcode, uint8 opmod)
		{
			Ins i;
			i.Opcode = (uint8) opcode;
			i.Opmod1 = opmod;			
			AddInstruction(&i, 2);
		}

		void AddThreeByteInstruction(Opcodes::OPCODE opcode, uint8 opmod1, uint8 opmod2)
		{
			Ins i;
			i.Opcode = (uint8) opcode;
			i.Opmod1 = opmod1;			
			i.Opmod2 = opmod2;				
			AddInstruction(&i, 3);
		}

		void AddFourByteInstruction(Opcodes::OPCODE opcode, uint8 opmod1, uint8 opmod2, uint8 opmod3)
		{
			Ins i;
			i.Opcode = (uint8) opcode;
			i.Opmod1 = opmod1;			
			i.Opmod2 = opmod2;			
			i.Opmod3 = opmod3;			
			AddInstruction(&i, 4);
		}
	public:
		Assembler(ICore& _core): core(_core), nextWritePosition((size_t) - 1) {}
		~Assembler() {}

		DEFINE_SEXY_ALLOCATORS_FOR_CLASS;

		void Append_AddImmediate(DINDEX Dsource,  BITCOUNT bits, DINDEX Dtarget, const VariantValue& v) override
		{
			if (Dsource == Dtarget && bits == BITCOUNT_32 && IsToInt8Lossless(v.int32Value))
			{
				Rococo::int8 value = (Rococo::int8) v.int32Value;

				if (value == -1)
				{
					ArgsOperateOnRegister args;
					args.opcode = Opcodes::Decrement32;
					args.reg = Dtarget;
					AddArgument(args);
					return;
				}
				else if (value == 1)
				{
					ArgsOperateOnRegister args;
					args.opcode = Opcodes::Increment32;
					args.reg = Dtarget;
					AddArgument(args);
					return;
				}
				else
				{
					AddThreeByteInstruction(Opcodes::AddQuick32, Dtarget, (uint8) value);
				}
			}
			else
			{
				AddFourByteInstruction(Opcodes::AddImmediate, Dsource, (uint8) bits, Dtarget);
				switch(bits)
				{
				case BITCOUNT_32:
					AddArgument(v.int32Value);
					break;
				case BITCOUNT_64:
					AddArgument(v.int64Value);
					break;
            default:
               break;
				}
			}
		}

		void Append_BooleanNot(DINDEX r) override
		{
			AddTwoByteInstruction(Opcodes::BooleanNot, r);
		}

		void Append_IntAdd(DINDEX Da, BITCOUNT bits, DINDEX Db) override
		{
			if (bits == BITCOUNT_32)
			{
				AddThreeByteInstruction(Opcodes::IntAdd32, Da, Db);
			}
			else
			{
				AddThreeByteInstruction(Opcodes::IntAdd64, Da, Db);
			}
		}

		void Append_IntSubtract(DINDEX Da, BITCOUNT bits, DINDEX Db) override
		{
			if (bits == BITCOUNT_32)
			{
				AddThreeByteInstruction(Opcodes::IntSubtract32, Da, Db);
			}
			else
			{
				AddThreeByteInstruction(Opcodes::IntSubtract64, Da, Db);
			}
		}

		void Append_IntMultiply(DINDEX Da, BITCOUNT bits, DINDEX Db) override
		{
			if (bits == BITCOUNT_32)
			{
				AddThreeByteInstruction(Opcodes::IntMultiply32, Da, Db);
			}
			else
			{
				AddThreeByteInstruction(Opcodes::IntMultiply64, Da, Db);
			}
		}

		void Append_IntNegate(DINDEX src, BITCOUNT bits) override
		{
			if (bits == BITCOUNT_32)
			{
				AddTwoByteInstruction(Opcodes::IntNegate32, src);
			}
			else
			{
				AddTwoByteInstruction(Opcodes::IntNegate64, src);
			}
		}

		void Append_FloatNegate32(DINDEX src) override
		{
			AddTwoByteInstruction(Opcodes::FloatNegate32, src);
		}

		void Append_FloatNegate64(DINDEX src) override
		{
			AddTwoByteInstruction(Opcodes::FloatNegate64, src);
		}

		void Append_IntDivide(DINDEX Dnumerator, BITCOUNT bits, DINDEX Ddenominator) override
		{
			if (bits == BITCOUNT_32)
			{
				AddThreeByteInstruction(Opcodes::IntDivide32, Dnumerator, Ddenominator);
			}
			else
			{
				AddThreeByteInstruction(Opcodes::IntDivide64, Dnumerator, Ddenominator);
			}
		}

		void Append_Invoke(ID_API_CALLBACK id) override
		{
			AddSingleByteInstruction(Opcodes::Invoke);
			AddArgument(id);
		}

		void Append_InvokeBy(DINDEX index) override
		{
			AddTwoByteInstruction(Opcodes::InvokeBy, index);
		}

		void Append_RestoreRegister(DINDEX Di, BITCOUNT bits) override
		{
			if (bits == BITCOUNT_32)
			{
				AddTwoByteInstruction(Opcodes::RestoreRegister32, Di);
			}
			else
			{
				AddTwoByteInstruction(Opcodes::RestoreRegister64, Di);
			}			
		}

		void Append_SaveRegister(DINDEX Di, BITCOUNT bits) override
		{
			if (bits == BITCOUNT_32)
			{
				AddTwoByteInstruction(Opcodes::SaveRegister32, Di);
			}
			else
			{
				AddTwoByteInstruction(Opcodes::SaveRegister64, Di);
			}
		}

		void Append_SubtractImmediate(DINDEX Dsource,  BITCOUNT bits, DINDEX Dtarget, const VariantValue& v) override
		{
			if (Dsource == Dtarget && bits == BITCOUNT_32 && IsToInt8Lossless(v.int32Value))
			{
				Rococo::int8 value = (Rococo::int8) -v.int32Value;
				if (value == -1)
				{
					ArgsOperateOnRegister args;
					args.opcode = Opcodes::Decrement32;
					args.reg = Dtarget;
					AddArgument(args);
				}
				else if (value == 1)
				{
					ArgsOperateOnRegister args;
					args.opcode = Opcodes::Increment32;
					args.reg = Dtarget;
					AddArgument(args);
				}
				else
				{
					AddThreeByteInstruction(Opcodes::AddQuick32, Dtarget, (uint8) value);
				}
			}
			else
			{
				AddFourByteInstruction(Opcodes::AddImmediate, Dsource, (uint8) bits, Dtarget);
				switch(bits)
				{
				case BITCOUNT_32:
					{
						int32 value = -v.int32Value;
						AddArgument(value);
					}
					break;
				case BITCOUNT_64:
					{
						int64 value = -v.int64Value;
						AddArgument(value);
					}
					break;
            default:
               break;
				}
			}
		}

		void Append_LogicalAND(DINDEX Dsource,  BITCOUNT bitCount, DINDEX Dtarget) override
		{
			AddThreeByteInstruction(bitCount == 64 ? Opcodes::LogicalAND64 : Opcodes::LogicalAND32, Dsource, Dtarget);	
		}

		void Append_LogicalOR(DINDEX Dsource,  BITCOUNT bitCount, DINDEX Dtarget) override
		{
			AddThreeByteInstruction(bitCount == 64 ? Opcodes::LogicalOR64 : Opcodes::LogicalOR32, Dsource, Dtarget);		
		}

		void Append_LogicalXOR(DINDEX Dsource,  BITCOUNT bitCount, DINDEX Dtarget) override
		{
			AddThreeByteInstruction(bitCount == 64 ? Opcodes::LogicalXOR64 : Opcodes::LogicalXOR32, Dsource, Dtarget);		
		}

		void Append_LogicalNOT(DINDEX Dsource,  BITCOUNT bitCount, DINDEX Dtarget) override
		{
			AddThreeByteInstruction(bitCount == 64 ? Opcodes::LogicalNOT64 : Opcodes::LogicalNOT32, Dsource, Dtarget);		
		}

		void Append_MoveRegister(DINDEX Dsource, DINDEX DTarget, BITCOUNT bitCount) override
		{
			if (bitCount == 64)
			{
				AddThreeByteInstruction(Opcodes::Move64, Dsource, DTarget);
			}
			else
			{
				AddThreeByteInstruction(Opcodes::Move32, Dsource, DTarget);
			}
		}

		void Append_NoOperation() override
		{
			AddSingleByteInstruction(Opcodes::NoOperation);
		}

		void Append_Poke(DINDEX Dsource, BITCOUNT bits, DINDEX Dtarget, int32 offset) override
		{
			if (bits == BITCOUNT_32)
			{
				AddThreeByteInstruction(Opcodes::Poke32, Dsource, Dtarget);
				AddArgument(offset);
			}
			else
			{
				AddThreeByteInstruction(Opcodes::Poke64, Dsource, Dtarget);
				AddArgument(offset);
			}
		}

		void Append_Pop(uint8 nBytes) override
		{
			if (nBytes > 0)
			{
				AddTwoByteInstruction(Opcodes::Pop, nBytes);
			}
		}

		void Append_Dereference_D4() override
		{
			AddSingleByteInstruction(Opcodes::DereferenceD4);
		}

		void Append_PushIndirect(DINDEX Dsource, DINDEX Dtarget, size_t nBytes)  override
		{
			AddThreeByteInstruction(Opcodes::PushIndirect, Dsource, Dtarget);
			AddArgument(nBytes);
		}

		void Append_PushAddress(DINDEX source, DINDEX offsetRegister) override
		{
			AddThreeByteInstruction(Opcodes::PushAddress, source, offsetRegister);
		}

		void Append_PushRegister(DINDEX Dsource, BITCOUNT bits) override
		{
			if (bits == BITCOUNT_32)
			{
				AddTwoByteInstruction(Opcodes::PushRegister32, Dsource);
			}
			else
			{
				AddTwoByteInstruction(Opcodes::PushRegister64, Dsource);
			}
		}

		void Append_PushStackVariable(int sfOffset, BITCOUNT bitcount) override
		{
			ArgsPushStackVariable args;
			args.opcode = (bitcount == BITCOUNT_32) ? Opcodes::PushStackVariable32 : Opcodes::PushStackVariable64;
			args.sfOffset = sfOffset;
			AddArgument(args);
		}

		void Append_PushStackFrameMemberPtr(int sfOffsetToStruct, int memberOffset) override
		{
			ArgsPushStackFrameMemberPtr args;
			args.opcode = Opcodes::PushStackFrameMemberPtr;
			args.sfOffset = sfOffsetToStruct;
			args.memberOffset = memberOffset;
			AddArgument(args);
		}

		void Append_PushStackFrameAddress(int offset) override
		{
			ArgsPushStackVariable args;
			args.opcode = Opcodes::PushStackAddress;
			args.sfOffset = offset;
			AddArgument(args);
		}

		void Append_PushLiteral(BITCOUNT bits, const VariantValue& value) override
		{
			if (bits == BITCOUNT_32)
			{
				AddSingleByteInstruction(Opcodes::PushImmediate32);
				AddArgument(value.int32Value);
			}
			else
			{
				AddSingleByteInstruction(Opcodes::PushImmediate64);
				AddArgument(value.int64Value);
			}
		}

		void Append_SetRegisterImmediate(DINDEX Di, const VariantValue& v, BITCOUNT bitCount) override
		{
			switch(bitCount)
			{
				case BITCOUNT_32:
				{
					ArgsSetRegister32 args;
					args.opcode = Opcodes::SetRegisterImmediate32;
					args.reg = Di;
					args.value = v.int32Value;
					AddArgument(args);
				}
				break;
				case BITCOUNT_64:
				{
					ArgsSetRegister64 args;
					args.opcode = Opcodes::SetRegisterImmediate64;
					args.reg = Di;
					args.value = v.int64Value;
					AddArgument(args);
				}
				break;
			default:
				throw InvalidInstructionException();
			}
		}

		void Append_SetStackFrameImmediate(int32 offset, const VariantValue& v, BITCOUNT bitCount) override
		{
			if (IsToInt8Lossless(offset) && bitCount == BITCOUNT_32)
			{
				AddTwoByteInstruction(Opcodes::SetStackFrameImmediate32, (Rococo::int8) offset);
				AddArgument(v.uint32Value);			
			}
			else if (IsToInt8Lossless(offset) && bitCount == BITCOUNT_64)
			{
				AddTwoByteInstruction(Opcodes::SetStackFrameImmediate64, (Rococo::int8) offset);
				AddArgument(v.uint64Value);			
			}
			else
			{
				AddTwoByteInstruction(Opcodes::SetStackFrameImmediateFar, (uint8) bitCount);
				AddArgument(offset);

				switch(bitCount)
				{
				case BITCOUNT_32:
					AddArgument(v.int32Value);
					break;

				case BITCOUNT_64:
					AddArgument(v.int64Value);
					break;

				default:
					throw InvalidInstructionException();
				}
			}
		}

		void Append_GetStackFrameValue(int32 offset, DINDEX Dtarget, BITCOUNT bitCount) override
		{
			if (IsToInt8Lossless(offset) && bitCount == BITCOUNT_32)
			{
				AddThreeByteInstruction(Opcodes::GetStackFrameValue32, (Rococo::int8) offset, Dtarget);			
			}
			else if (IsToInt8Lossless(offset) && bitCount == BITCOUNT_64)
			{
				AddThreeByteInstruction(Opcodes::GetStackFrameValue64, (Rococo::int8) offset, Dtarget);		
			}
			else
			{
				AddThreeByteInstruction(Opcodes::GetStackFrameValueFar, (uint8) bitCount, Dtarget);
				AddArgument(offset);
			}
		}

		void Append_GetStackFrameValueAndExtendToPointer(int32 offset, DINDEX target) override
		{
			AddTwoByteInstruction(Opcodes::GetStackFrameValueAndExtendToPointer, target);
			AddArgument(offset);
		}

		void Append_GetStackFrameMemberPtr(DINDEX Dtarget, int SFoffset, int memberOffset) override
		{
			if (IsToInt8Lossless(SFoffset) && IsToInt8Lossless(memberOffset))
			{
				AddFourByteInstruction(Opcodes::GetStackFrameMemberPtr, Dtarget, (Rococo::int8) SFoffset, (Rococo::int8) memberOffset);
			}
			else
			{
				AddTwoByteInstruction(Opcodes::GetStackFrameMemberPtrFar, Dtarget);
				AddArgument(SFoffset);
				AddArgument(memberOffset);
			}
		}

		void Append_SetStackFramePtrFromD5(int SFoffset, int memberOffset) override
		{
			AddSingleByteInstruction(Opcodes::SetSFMemberPtrFromD5);
			AddArgument(SFoffset);
			AddArgument(memberOffset);
		}

		void Append_GetStackFrameMemberPtrAndDeref(DINDEX Dtarget, int SFoffset, int memberOffset) override
		{
			ArgsGetStackFrameMemberPtrAndDeref args;
			args.opcode = Opcodes::GetStackFrameMemberPtrAndDeref;
			args.dtarget = Dtarget;
			args.sfToStructRef = SFoffset;
			args.structToMemberOffset = memberOffset;

			AddArgument(args);
		}

		void Append_GetStackFrameMember(DINDEX Dtarget, int SFoffset, int memberOffset, BITCOUNT bits) override
		{
			if (bits == BITCOUNT_32)
			{
				AddTwoByteInstruction(Opcodes::GetStackFrameMember32, Dtarget);
			}
			else
			{
				AddTwoByteInstruction(Opcodes::GetStackFrameMember64, Dtarget);
			}

			AddArgument(SFoffset);
			AddArgument(memberOffset);
		}

		void Append_GetStackFrameAddress(DINDEX Dtarget, int offset) override
		{
			AddTwoByteInstruction(Opcodes::GetStackFrameAddress, Dtarget);
			AddArgument(offset);
		}

		void Append_SetSFValueFromSFValue(int32 trgOffset, int32 srcOffset, BITCOUNT bitCount) override
		{
			if (IsToInt8Lossless(trgOffset) && IsToInt8Lossless(srcOffset) && bitCount == BITCOUNT_32)
			{
				AddThreeByteInstruction(Opcodes::SetSFValueFromSFValue32, (Rococo::int8) trgOffset, (Rococo::int8) srcOffset);
			}
			else
			{
				ArgsSetSFValueFromSFValue args;
				args.opcode = Opcodes::SetSFValueFromSFValueLong;
				args.byteCount = (Rococo::int8) (bitCount >> 3);
				args.sfTargetOffset = trgOffset;
				args.sfSourceOffset = srcOffset;

				AddArgument(args);
			}
		}

		void Append_SetStackFrameValue(int32 offset, DINDEX Dsource, BITCOUNT bitCount) override
		{
			if (IsToInt8Lossless(offset) && bitCount == BITCOUNT_32)
			{
				AddThreeByteInstruction(Opcodes::SetStackFrameValue32, (Rococo::int8) offset, Dsource);			
			}
			else if (IsToInt8Lossless(offset) && bitCount == BITCOUNT_64)
			{
				AddThreeByteInstruction(Opcodes::SetStackFrameValue64, (Rococo::int8) offset, Dsource);		
			}
			else
			{
				AddThreeByteInstruction(Opcodes::SetStackFrameValueFar, (Rococo::int8) bitCount, Dsource);
				AddArgument(offset);
			}
		}

		void Append_ShiftLeft(DINDEX Di, BITCOUNT bitCount, Rococo::int8 shiftCount) override
		{
			AddThreeByteInstruction(bitCount == BITCOUNT_64 ? Opcodes::ShiftLeft64 : Opcodes::ShiftLeft32, Di, shiftCount);
		}

		void Append_ShiftRight(DINDEX Di, BITCOUNT bitCount, Rococo::int8 shiftCount) override
		{
			AddThreeByteInstruction(bitCount == BITCOUNT_64 ? Opcodes::ShiftRight64 : Opcodes::ShiftRight32, Di, shiftCount);
		}

		void Append_Exit(DINDEX Di) override
		{
			AddTwoByteInstruction(Opcodes::Exit, Di);
		}

		void Append_StackAlloc(int32 nBytes) override
		{
			if (nBytes == 0) return;

			if (nBytes < 0 && nBytes > -255)
			{
				int nBytesToPop = -nBytes;
				Append_Pop((uint8) nBytesToPop);
			}
			else if (nBytes >= 0 && nBytes < 255)
			{
				AddTwoByteInstruction(Opcodes::StackAlloc, (uint8) nBytes);
			}
			else
			{
				AddSingleByteInstruction(Opcodes::StackAllocBig);
				AddArgument(nBytes);
			}
		}

		void Append_Test(DINDEX Di, BITCOUNT bitcount) override
		{
			ArgsOperateOnRegister args;

			if (bitcount == BITCOUNT_32)
			{
				args.opcode = Opcodes::Test32;
			}
			else
			{
				args.opcode = Opcodes::Test64;
			}

			args.reg = Di;

			AddArgument(args);
		}

		void Append_Branch(int PCoffset) override
		{
			AddSingleByteInstruction(Opcodes::Branch);
			AddArgument(PCoffset);
		}

		void Append_BranchIf(CONDITION cse, int PCoffset) override
		{	
			ArgsBranchIf args;
			args.PCoffset = PCoffset;

			if (cse == CONDITION_IF_GREATER_OR_EQUAL)
			{
				args.opcode = Opcodes::BranchIfGTE;
			}
			else if (cse == CONDITION_IF_GREATER_THAN)
			{
				args.opcode = Opcodes::BranchIfGT;
			}
			else if (cse == CONDITION_IF_NOT_EQUAL)
			{
				args.opcode = Opcodes::BranchIfNE;
			}
			else if (cse == CONDITION_IF_EQUAL)
			{
				args.opcode = Opcodes::BranchIfEqual;
			}
			else if (cse == CONDITION_IF_LESS_THAN)
			{
				args.opcode = Opcodes::BranchIfLT;
			}
			else
			{
				Throw(0, "Unhandled condition");
			}

			AddArgument(args);
		}

		void Append_SetIf(CONDITION cse, DINDEX Di, BITCOUNT bits) override
		{
			AddThreeByteInstruction(((int)bits) == 32 ? Opcodes::SetIf32 : Opcodes::SetIf64, (uint8) cse, Di);
		}

		void Append_SwapRegister(DINDEX a, DINDEX b) override
		{
			AddThreeByteInstruction(Opcodes::Swap, a, b);
		}

		void Append_FloatAdd(DINDEX Da, DINDEX Db, FLOATSPEC spec) override
		{
			AddFourByteInstruction(spec == FLOATSPEC_SINGLE ? Opcodes::FloatAdd : Opcodes::DoubleAdd, Da, Db, (uint8) spec);
		}

		void Append_FloatSubtract(DINDEX Da, DINDEX Db, FLOATSPEC spec) override
		{
			AddFourByteInstruction(spec == FLOATSPEC_SINGLE ? Opcodes::FloatSubtract : Opcodes::DoubleSubtract, Da, Db, (uint8) spec);
		}

		void Append_FloatMultiply(DINDEX Da, DINDEX Db, FLOATSPEC spec) override
		{
			AddFourByteInstruction(spec == FLOATSPEC_SINGLE ? Opcodes::FloatMultiply : Opcodes::DoubleMultiply, Da, Db, (uint8) spec);
		}

		void Append_FloatDivide(DINDEX Da, DINDEX Db, FLOATSPEC spec) override
		{
			AddFourByteInstruction(spec == FLOATSPEC_SINGLE ? Opcodes::FloatDivide : Opcodes::DoubleDivide, Da, Db, (uint8) spec);
		}

		void Append_CallByRegister(DINDEX offsetRegister)  override
		{
			AddTwoByteInstruction(Opcodes::CallBy, offsetRegister);
		}

		void Append_CallById(ID_BYTECODE id) override
		{
			AddSingleByteInstruction(Opcodes::CallById);
			AddArgument(id);
		}

		void Append_CallByIdIndirect(DINDEX Di) override
		{
			AddTwoByteInstruction(Opcodes::CallByIdIndirect, Di);
		}

		void Append_Call(int offsetFromPCstart) override
		{
			AddSingleByteInstruction(Opcodes::Call);
			AddArgument(offsetFromPCstart);
		}

		void Append_CallVirtualFunctionViaRefOnStack(int32 SFoffsetToInterfaceRef, int32 vTableOffset /* nBytes into vtable to find the method id */, int32 instanceToInterfaceOffset) override
		{
			ArgsCallVirtualFunctionViaRefOnStack args;
			args.opcode = Opcodes::CallVirtualFunctionViaRefOnStack;
			args.SFoffsetToInterfaceRef = SFoffsetToInterfaceRef;
			args.vTableOffset = vTableOffset >> 3;
			args.instanceToInterfaceOffset = instanceToInterfaceOffset;
			AddArgument(args);
		}

		void Append_CallVirtualFunctionViaMemberOffsetOnStack(int32 SFoffsetToStruct, int32 memberOffset, int32 vTableOffset) override
		{
			ArgsCallVirtualFunctionViaMemberOffsetOnStack args;
			args.opcode = Opcodes::CallVirtualFunctionViaMemberOffsetOnStack;
			args.SFoffsetToStruct = SFoffsetToStruct;
			args.memberOffsetToInterfaceRef = memberOffset;
			args.vTableOffset = vTableOffset >> 3;
			AddArgument(args);
		}

		void Append_CallVirtualFunctionByAddress(int32 SFoffsetToInterfaceValue, int32 vTableOffset) override
		{
			AddSingleByteInstruction(Opcodes::CallVirtualFunctionByAddress);
			AddArgument(SFoffsetToInterfaceValue);
			AddArgument(vTableOffset);
		}

		void Append_CopySFVariable(int targetOffset, int sourceOffset, size_t nBytes) override
		{
			if (nBytes == 0) { Append_NoOperation(); return; }
			if (IsToInt8Lossless(targetOffset) && IsToInt8Lossless(sourceOffset) && nBytes <= 255)
			{
				AddFourByteInstruction(Opcodes::CopySFMemoryNear, (uint8) (Rococo::int8) targetOffset, (uint8) (Rococo::int8) sourceOffset, (uint8) nBytes);
			}
			else
			{
				AddSingleByteInstruction(Opcodes::CopySFMemory);
				AddArgument(targetOffset);
				AddArgument(sourceOffset);
				AddArgument(nBytes);
			}
		}

		void Append_SetSFMemberRefFromSFValue(int32 targetSFOffset, int32 targetMemberOffset, int32 SFSourceValueOffset, size_t nBytesSource) override
		{
			if (IsToInt8Lossless(targetSFOffset) && IsToInt8Lossless(targetMemberOffset) && IsToInt8Lossless(SFSourceValueOffset) && nBytesSource == 4)
			{
				AddFourByteInstruction(Opcodes::SetSFMemberByRefFromSFByValue32, (Rococo::int8)targetSFOffset, (Rococo::int8) targetMemberOffset, (Rococo::int8) SFSourceValueOffset);
			}
			else if (IsToInt8Lossless(targetSFOffset) && IsToInt8Lossless(targetMemberOffset) && IsToInt8Lossless(SFSourceValueOffset) && nBytesSource == 8)
			{
				AddFourByteInstruction(Opcodes::SetSFMemberByRefFromSFByValue64, (Rococo::int8)targetSFOffset, (Rococo::int8)targetMemberOffset, (Rococo::int8)SFSourceValueOffset);
			}
			else
			{
				ArgsSetSFMemberRefFromSFValue args;
				args.opcode = Opcodes::SetSFMemberRefFromSFValue;
				args.targetSFOffset = targetSFOffset;
				args.targetMemberOffset = targetMemberOffset;
				args.SFSourceValueOffset = SFSourceValueOffset;
				args.nBytesSource = nBytesSource;
				AddArgument(args);
			}
		}

		void Append_SetSFMemberRefFromSFMemberByRef(int32 sourceSFOffset, int32 sourceMemberOffset, int32 targetSFOffset, int32 targetMemberOffset, size_t nBytesSource) override
		{
			Args_SetMemberRefFromSFMemberByRef args;
			args.opcode = Opcodes::SetSFMemberRefFromSFMemberByRef64;
			args.sourceSFOffset = sourceSFOffset;
			args.sourceMemberOffset = sourceMemberOffset;
			args.targetSFOffset = targetSFOffset;
			args.targetMemberOffset = targetMemberOffset;

			if (nBytesSource != 8)
			{
				Throw(0, "Not implemented for anything other than 64-bit copying");
			}

			AddArgument(args);
		}

		void Append_SetSFValueFromSFMemberRef(int32 sourceSFOffset, int32 sourceMemberOffset, int32 SFTargetValueOffset, size_t nBytesSource) override
		{
			if (IsToInt8Lossless(sourceSFOffset) && IsToInt8Lossless(sourceMemberOffset) && IsToInt8Lossless(SFTargetValueOffset) && nBytesSource == 4)
			{
				AddFourByteInstruction(Opcodes::SetSFValueFromSFMemberByRef32, (Rococo::int8)sourceSFOffset, (Rococo::int8) sourceMemberOffset, (Rococo::int8) SFTargetValueOffset);
			}
			else
			{
				ArgsSetSFValueFromSFMemberRef args;
				args.opcode = Opcodes::SetSFValueFromSFMemberByRef;
				args.srcSFOffset = sourceSFOffset;
				args.srcMemberOffset = sourceMemberOffset;
				args.targetSFOffset = SFTargetValueOffset;
				args.nBytesSource = nBytesSource;
				AddArgument(args);
			}
		}

		void Append_SetSFMemberByRefFromRegister(DINDEX Dsource, int32 sfOffset, int32 memberOffset, BITCOUNT bitcount) override
		{
			if (IsToInt8Lossless(sfOffset) && IsToInt8Lossless(memberOffset) && bitcount == 32)
			{
				AddFourByteInstruction(Opcodes::SetSFMemberByRefFromRegister32, Dsource, (Rococo::int8) sfOffset, (Rococo::int8) memberOffset);
			}
			else
			{
				AddThreeByteInstruction(Opcodes::SetSFMemberByRefFromRegisterLong, Dsource, (uint8) bitcount);
				AddArgument(sfOffset);
				AddArgument(memberOffset);
			}
		}

		void Append_CopyMemory(DINDEX target, DINDEX source, size_t nBytes) override
		{
			if (nBytes == 0) { Append_NoOperation(); return; }

			if (nBytes == 4)
			{
				AddThreeByteInstruction(Opcodes::Copy32Bits, target, source);
			}
			else if (nBytes == 8)
			{
				AddThreeByteInstruction(Opcodes::Copy64Bits, target, source);
			}
			else if (nBytes <= 255)
			{
				AddFourByteInstruction(Opcodes::CopyMemory, target, source, (uint8) nBytes);
			}
			else
			{			
				AddThreeByteInstruction(Opcodes::CopyMemoryBig, target, source);
				AddArgument(nBytes);
			}
		}

		void Append_CopySFVariableFromRef(int32 targetSFOffset, int32 sourcePtrSFOffset, int32 sourceMemberOffset, size_t nBytesSource) override
		{
			AddSingleByteInstruction(Opcodes::CopySFVariableFromRef);
			AddArgument(targetSFOffset);
			AddArgument(sourcePtrSFOffset);
			AddArgument(sourceMemberOffset);
			AddArgument(nBytesSource);
		}

		void Append_IncrementPtr(DINDEX sourceAndTarget, int32 value) override
		{
			if (value == 0) return;
			if (IsToInt8Lossless(value))	
			{
				AddThreeByteInstruction(Opcodes::IncrementPtr, sourceAndTarget, (uint8)(Rococo::int8) value);
			}
			else
			{
				AddTwoByteInstruction(Opcodes::IncrementPtrBig, sourceAndTarget);
				AddArgument(value);
			}
		}

		void Append_Return() override
		{
			AddSingleByteInstruction(Opcodes::Return);
		}

		void Append_Yield() override
		{
			AddSingleByteInstruction(Opcodes::Yield);
		}

		void Append_TripDebugger() override
		{
			AddSingleByteInstruction(Opcodes::Debug);
		}

		void Append_GetGlobal(BITCOUNT bits, int32 offset) override
		{
			AddTwoByteInstruction(Opcodes::GetGlobal, (uint8) bits);
			AddArgument(offset);
		}

		void Append_SetGlobal(BITCOUNT bits, int32 offset) override
		{
			AddTwoByteInstruction(Opcodes::SetGlobal, (uint8) bits);
			AddArgument(offset);
		}

		void Clear()  override { program.clear(); }
		ICore& Core()  override { return core; }
		void Free()  override { delete this; }

		size_t WritePosition() const override
		{
			return (nextWritePosition != -1) ? nextWritePosition : program.size();
		}

		void SetWriteModeToOverwrite(size_t position) override
		{
			this->nextWritePosition = position;
		}

		void SetWriteModeToAppend() override
		{
			this->nextWritePosition = (size_t) -1;
		}

		const unsigned char* Program(OUT size_t& length) const override
		{
			if (program.empty())
			{
				OUT length = 0;
				return NULL;
			}
			else
			{
				OUT length = program.size();
				return &program[0];
			}
		}

		void Revert(size_t position) override
		{
			if (position < program.size())
			{
				program.resize(position);
				nextWritePosition = (size_t) -1;
			}
		}
	};
}

namespace Rococo { namespace VM
{
	IAssembler* CreateAssembler(ICore& core)
	{
		return new Assembler(core);
	}
}}