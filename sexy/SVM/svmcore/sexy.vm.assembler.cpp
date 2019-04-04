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

using namespace Rococo;
using namespace Rococo::VM;

namespace
{
	typedef std::vector<unsigned char> TMemory;

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
		Assembler(ICore& _core): core(_core), nextWritePosition(-1) {}
		~Assembler() {}

		void Append_AddImmediate(DINDEX Dsource,  BITCOUNT bits, DINDEX Dtarget, const VariantValue& v)
		{
			if (Dsource == Dtarget && bits == BITCOUNT_32 && IsToInt8Lossless(v.int32Value))
			{
				int8 value = (int8) v.int32Value;

				if (value == -1)
				{
					AddTwoByteInstruction(Opcodes::Decrement32, Dtarget);
				}
				else if (value == 1)
				{
					AddTwoByteInstruction(Opcodes::Increment32, Dtarget);
				}
				else
				{
					AddThreeByteInstruction(Opcodes::AddQuick32, Dtarget, (uint8) value);
				}
			}
			else
			{
				AddFourByteInstruction(Opcodes::AddImmediate, Dsource, bits, Dtarget);
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

		virtual void Append_BooleanNot(DINDEX r)
		{
			AddTwoByteInstruction(Opcodes::BooleanNot, r);
		}

		virtual void Append_IntAdd(DINDEX Da, BITCOUNT bits, DINDEX Db)
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

		virtual void Append_IntSubtract(DINDEX Da, BITCOUNT bits, DINDEX Db)
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

		virtual void Append_IntMultiply(DINDEX Da, BITCOUNT bits, DINDEX Db)
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

		virtual void Append_IntNegate(DINDEX src, BITCOUNT bits, DINDEX trg)
		{
			if (bits == BITCOUNT_32)
			{
				AddThreeByteInstruction(Opcodes::IntNegate32, src, trg);
			}
			else
			{
				AddThreeByteInstruction(Opcodes::IntNegate64, src, trg);
			}
		}

		virtual void Append_IntDivide(DINDEX Dnumerator, BITCOUNT bits, DINDEX Ddenominator)
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

		virtual void Append_Invoke(ID_API_CALLBACK id)
		{
			AddSingleByteInstruction(Opcodes::Invoke);
			AddArgument(id);
		}

		virtual void Append_InvokeBy(DINDEX index)
		{
			AddTwoByteInstruction(Opcodes::InvokeBy, index);
		}

		virtual void Append_RestoreRegister(DINDEX Di, BITCOUNT bits)
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

		virtual void Append_SaveRegister(DINDEX Di, BITCOUNT bits)
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

		virtual void Append_SubtractImmediate(DINDEX Dsource,  BITCOUNT bits, DINDEX Dtarget, const VariantValue& v)
		{
			if (Dsource == Dtarget && bits == BITCOUNT_32 && IsToInt8Lossless(v.int32Value))
			{
				int8 value = (int8) -v.int32Value;
				if (value == -1)
				{
					AddTwoByteInstruction(Opcodes::Decrement32, Dtarget);
				}
				else if (value == 1)
				{
					AddTwoByteInstruction(Opcodes::Increment32, Dtarget);
				}
				else
				{
					AddThreeByteInstruction(Opcodes::AddQuick32, Dtarget, (uint8) value);
				}
			}
			else
			{
				AddFourByteInstruction(Opcodes::AddImmediate, Dsource, bits, Dtarget);
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

		virtual void Append_LogicalAND(DINDEX Dsource,  BITCOUNT bitCount, DINDEX Dtarget)
		{
			AddThreeByteInstruction(bitCount == 64 ? Opcodes::LogicalAND64 : Opcodes::LogicalAND32, Dsource, Dtarget);	
		}

		virtual void Append_LogicalOR(DINDEX Dsource,  BITCOUNT bitCount, DINDEX Dtarget)
		{
			AddThreeByteInstruction(bitCount == 64 ? Opcodes::LogicalOR64 : Opcodes::LogicalOR32, Dsource, Dtarget);		
		}

		virtual void Append_LogicalXOR(DINDEX Dsource,  BITCOUNT bitCount, DINDEX Dtarget)
		{
			AddThreeByteInstruction(bitCount == 64 ? Opcodes::LogicalXOR64 : Opcodes::LogicalXOR32, Dsource, Dtarget);		
		}

		virtual void Append_LogicalNOT(DINDEX Dsource,  BITCOUNT bitCount, DINDEX Dtarget)
		{
			AddThreeByteInstruction(bitCount == 64 ? Opcodes::LogicalNOT64 : Opcodes::LogicalNOT32, Dsource, Dtarget);		
		}

		virtual void Append_MoveRegister(DINDEX Dsource, DINDEX DTarget, BITCOUNT bitCount)
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

		virtual void Append_NoOperation()
		{
			AddSingleByteInstruction(Opcodes::NoOperation);
		}

		virtual void Append_Poke(DINDEX Dsource, BITCOUNT bits, DINDEX Dtarget, int32 offset)
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

		virtual void Append_Pop(uint8 nBytes)
		{
			if (nBytes > 0)
			{
				AddTwoByteInstruction(Opcodes::Pop, nBytes);
			}
		}

		virtual void Append_PushIndirect(DINDEX Dsource, DINDEX Dtarget, size_t nBytes) 
		{
			AddThreeByteInstruction(Opcodes::PushIndirect, Dsource, Dtarget);
			AddArgument(nBytes);
		}

		virtual void Append_PushAddress(DINDEX source, DINDEX offsetRegister)
		{
			AddThreeByteInstruction(Opcodes::PushAddress, source, offsetRegister);
		}

		virtual void Append_PushRegister(DINDEX Dsource, BITCOUNT bits)
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

		virtual void Append_PushStackVariable(int sfOffset, BITCOUNT bitcount)
		{
			if (bitcount == BITCOUNT_32 && IsToInt8Lossless(sfOffset))
			{
				AddTwoByteInstruction(Opcodes::PushStackVariable32, (int8) sfOffset);
			}
			else
			{
				Append_GetStackFrameValue(sfOffset, VM::REGISTER_D4, bitcount);
				Append_PushRegister(VM::REGISTER_D4, bitcount);
			}
		}

		virtual void Append_PushStackFrameAddress(int offset)
		{
			if (IsToInt8Lossless(offset))
			{
				AddTwoByteInstruction(Opcodes::PushStackAddress, (int8) offset);
			}
			else
			{
				Append_GetStackFrameAddress(VM::REGISTER_D4, offset);
				Append_PushRegister(VM::REGISTER_D4, BITCOUNT_POINTER);			
			}
		}

		virtual void Append_PushLiteral(BITCOUNT bits, const VariantValue& value)
		{
			if (bits == BITCOUNT_32)
			{
				AddSingleByteInstruction(Opcodes::PushImmediate32);
				AddArgument(value.int32Value);
			}
			else
			{
				Append_SetRegisterImmediate(VM::REGISTER_D4, value, bits);
				Append_PushRegister(VM::REGISTER_D4, bits);
			}
		}

		virtual void Append_SetRegisterImmediate(DINDEX Di, const VariantValue& v, BITCOUNT bitCount)
		{
			switch(bitCount)
			{
			case BITCOUNT_32:
					AddTwoByteInstruction(Opcodes::SetRegisterImmediate32, Di);
					AddArgument(v.uint32Value);
					break;
			case BITCOUNT_64:
					AddTwoByteInstruction(Opcodes::SetRegisterImmediate64, Di);
					AddArgument(v.uint64Value);
				break;
			default:
				throw InvalidInstructionException();
			}
		}

		virtual void Append_SetStackFrameImmediate(int32 offset, const VariantValue& v, BITCOUNT bitCount)
		{
			if (IsToInt8Lossless(offset) && bitCount == BITCOUNT_32)
			{
				AddTwoByteInstruction(Opcodes::SetStackFrameImmediate32, (int8) offset);
				AddArgument(v.uint32Value);			
			}
			else if (IsToInt8Lossless(offset) && bitCount == BITCOUNT_64)
			{
				AddTwoByteInstruction(Opcodes::SetStackFrameImmediate64, (int8) offset);
				AddArgument(v.uint64Value);			
			}
			else
			{
				AddTwoByteInstruction(Opcodes::SetStackFrameImmediateFar, bitCount);
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

		virtual void Append_GetStackFrameValue(int32 offset, DINDEX Dtarget, BITCOUNT bitCount)
		{
			if (IsToInt8Lossless(offset) && bitCount == BITCOUNT_32)
			{
				AddThreeByteInstruction(Opcodes::GetStackFrameValue32, (int8) offset, Dtarget);			
			}
			else if (IsToInt8Lossless(offset) && bitCount == BITCOUNT_64)
			{
				AddThreeByteInstruction(Opcodes::GetStackFrameValue64, (int8) offset, Dtarget);		
			}
			else
			{
				AddThreeByteInstruction(Opcodes::GetStackFrameValueFar, bitCount, Dtarget);
				AddArgument(offset);
			}
		}

		virtual void Append_GetStackFrameValueAndExtendToPointer(int32 offset, DINDEX target)
		{
			AddTwoByteInstruction(Opcodes::GetStackFrameValueAndExtendToPointer, target);
			AddArgument(offset);
		}

		virtual void Append_GetStackFrameMemberPtr(DINDEX Dtarget, int SFoffset, int memberOffset)
		{
			if (IsToInt8Lossless(SFoffset) && IsToInt8Lossless(memberOffset))
			{
				AddFourByteInstruction(Opcodes::GetStackFrameMemberPtr, Dtarget, (int8) SFoffset, (int8) memberOffset);
			}
			else
			{
				AddTwoByteInstruction(Opcodes::GetStackFrameMemberPtrFar, Dtarget);
				AddArgument(SFoffset);
				AddArgument(memberOffset);
			}
		}

		virtual void Append_GetStackFrameMember(DINDEX Dtarget, int SFoffset, int memberOffset, BITCOUNT bits)
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

		virtual void Append_GetStackFrameAddress(DINDEX Dtarget, int offset)
		{
			AddTwoByteInstruction(Opcodes::GetStackFrameAddress, Dtarget);
			AddArgument(offset);
		}

		virtual void Append_SetSFValueFromSFValue(int32 trgOffset, int32 srcOffset, BITCOUNT bitCount)
		{
			if (IsToInt8Lossless(trgOffset) && IsToInt8Lossless(srcOffset) && bitCount == BITCOUNT_32)
			{
				AddThreeByteInstruction(Opcodes::SetSFValueFromSFValue32, (int8) trgOffset, (int8) srcOffset);
			}
			else if (bitCount == BITCOUNT_32 || bitCount == BITCOUNT_64)
			{
				Append_GetStackFrameValue(srcOffset, VM::REGISTER_D4, bitCount);
				Append_SetStackFrameValue(trgOffset, VM::REGISTER_D4, bitCount);
			}
			else if (bitCount == BITCOUNT_128)
			{
				Append_GetStackFrameValue(srcOffset, VM::REGISTER_D4, BITCOUNT_64);
				Append_SetStackFrameValue(trgOffset, VM::REGISTER_D4, BITCOUNT_64);
				Append_GetStackFrameValue(srcOffset+8, VM::REGISTER_D4, BITCOUNT_64);
				Append_SetStackFrameValue(trgOffset+8, VM::REGISTER_D4, BITCOUNT_64);
			}
		}

		virtual void Append_SetStackFrameValue(int32 offset, DINDEX Dsource, BITCOUNT bitCount)
		{
			if (IsToInt8Lossless(offset) && bitCount == BITCOUNT_32)
			{
				AddThreeByteInstruction(Opcodes::SetStackFrameValue32, (int8) offset, Dsource);			
			}
			else if (IsToInt8Lossless(offset) && bitCount == BITCOUNT_64)
			{
				AddThreeByteInstruction(Opcodes::SetStackFrameValue64, (int8) offset, Dsource);		
			}
			else
			{
				AddThreeByteInstruction(Opcodes::SetStackFrameValueFar, bitCount, Dsource);
				AddArgument(offset);
			}
		}

		virtual void Append_ShiftLeft(DINDEX Di, BITCOUNT bitCount, int8 shiftCount)
		{
			AddThreeByteInstruction(bitCount == BITCOUNT_64 ? Opcodes::ShiftLeft64 : Opcodes::ShiftLeft32, Di, shiftCount);
		}

		virtual void Append_ShiftRight(DINDEX Di, BITCOUNT bitCount, int8 shiftCount)
		{
			AddThreeByteInstruction(bitCount == BITCOUNT_64 ? Opcodes::ShiftRight64 : Opcodes::ShiftRight32, Di, shiftCount);
		}

		virtual void Append_Exit(DINDEX Di)
		{
			AddTwoByteInstruction(Opcodes::Exit, Di);
		}

		virtual void Append_StackAlloc(int32 nBytes)
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

		virtual void Append_Test(DINDEX Di, BITCOUNT bitcount)
		{
			if (bitcount == BITCOUNT_32)
			{
				AddTwoByteInstruction(Opcodes::Test32, Di);
			}
			else
			{
				AddTwoByteInstruction(Opcodes::Test64, Di);
			}
		}

		virtual void Append_Branch(int PCoffset)
		{
			AddSingleByteInstruction(Opcodes::Branch);
			AddArgument(PCoffset);
		}

		virtual void Append_BranchIf(CONDITION cse, int PCoffset)
		{			
			if (cse == CONDITION_IF_GREATER_OR_EQUAL)
			{
				AddSingleByteInstruction(Opcodes::BranchIfGTE);
				AddArgument(PCoffset);
			}
			else if (cse == CONDITION_IF_GREATER_THAN)
			{
				AddSingleByteInstruction(Opcodes::BranchIfGT);
				AddArgument(PCoffset);
			}
			else if (cse == CONDITION_IF_NOT_EQUAL)
			{
				AddSingleByteInstruction(Opcodes::BranchIfNE);
				AddArgument(PCoffset);
			}
			else if (cse == CONDITION_IF_LESS_THAN)
			{
				int8 smallOffset = (int8) PCoffset;
				AddSingleByteInstruction(Opcodes::BranchIfLT);
				AddArgument(PCoffset);
			}
			else
			{
				AddTwoByteInstruction(Opcodes::BranchIf, cse);
				AddArgument(PCoffset);
			}
		}

		virtual void Append_SetIf(CONDITION cse, DINDEX Di, BITCOUNT bits)
		{
			AddThreeByteInstruction(((int)bits) == 32 ? Opcodes::SetIf32 : Opcodes::SetIf64, cse, Di);
		}

		virtual void Append_SwapRegister(DINDEX a, DINDEX b)
		{
			AddThreeByteInstruction(Opcodes::Swap, a, b);
		}

		virtual void Append_FloatAdd(DINDEX Da, DINDEX Db, FLOATSPEC spec)
		{
			AddFourByteInstruction(spec == FLOATSPEC_SINGLE ? Opcodes::FloatAdd : Opcodes::DoubleAdd, Da, Db, spec);
		}

		virtual void Append_FloatSubtract(DINDEX Da, DINDEX Db, FLOATSPEC spec)
		{
			AddFourByteInstruction(spec == FLOATSPEC_SINGLE ? Opcodes::FloatSubtract : Opcodes::DoubleSubtract, Da, Db, spec);
		}

		virtual void Append_FloatMultiply(DINDEX Da, DINDEX Db, FLOATSPEC spec)
		{
			AddFourByteInstruction(spec == FLOATSPEC_SINGLE ? Opcodes::FloatMultiply : Opcodes::DoubleMultiply, Da, Db, spec);
		}

		virtual void Append_FloatDivide(DINDEX Da, DINDEX Db, FLOATSPEC spec)
		{
			AddFourByteInstruction(spec == FLOATSPEC_SINGLE ? Opcodes::FloatDivide : Opcodes::DoubleDivide, Da, Db, spec);
		}

		virtual void Append_CallByRegister(DINDEX offsetRegister) 
		{
			AddTwoByteInstruction(Opcodes::CallBy, offsetRegister);
		}

		virtual void Append_CallById(ID_BYTECODE id)
		{
			AddSingleByteInstruction(Opcodes::CallById);
			AddArgument(id);
		}

		virtual void Append_CallByIdIndirect(DINDEX Di)
		{
			AddTwoByteInstruction(Opcodes::CallByIdIndirect, Di);
		}

		virtual void Append_Call(int offsetFromPCstart)
		{
			AddSingleByteInstruction(Opcodes::Call);
			AddArgument(offsetFromPCstart);
		}

		virtual void Append_CallVirtualFunctionByValue(int32 SFoffsetToInterfaceRef, int32 vTableOffset /* nBytes into vtable to find the method id */, int32 instanceToInterfaceOffset)
		{
			ArgsCallVirtualFunctionByValue args;
			args.opcode = Opcodes::CallVirtualFunctionByValue;
			args.SFoffsetToInterfaceRef = SFoffsetToInterfaceRef;
			args.vTableOffset = vTableOffset >> 3;
			args.instanceToInterfaceOffset = instanceToInterfaceOffset;
			AddArgument(args);
		}

		virtual void Append_CallVirtualFunctionByAddress(int32 SFoffsetToInterfaceValue, int32 vTableOffset)
		{
			AddSingleByteInstruction(Opcodes::CallVirtualFunctionByAddress);
			AddArgument(SFoffsetToInterfaceValue);
			AddArgument(vTableOffset);
		}

		virtual void Append_CopySFVariable(int targetOffset, int sourceOffset, size_t nBytes)
		{
			if (nBytes == 0) { Append_NoOperation(); return; }
			if (IsToInt8Lossless(targetOffset) && IsToInt8Lossless(sourceOffset) && nBytes <= 255)
			{
				AddFourByteInstruction(Opcodes::CopySFMemoryNear, (uint8) (int8) targetOffset, (uint8) (int8) sourceOffset, (uint8) nBytes);
			}
			else
			{
				AddSingleByteInstruction(Opcodes::CopySFMemory);
				AddArgument(targetOffset);
				AddArgument(sourceOffset);
				AddArgument(nBytes);
			}
		}

		virtual void Append_SetSFMemberRefFromSFValue(int32 targetSFOffset, int32 targetMemberOffset, int32 SFSourceValueOffset, size_t nBytesSource)
		{
			if (IsToInt8Lossless(targetSFOffset) && IsToInt8Lossless(targetMemberOffset) && IsToInt8Lossless(SFSourceValueOffset) && nBytesSource == 4)
			{
				AddFourByteInstruction(Opcodes::SetSFMemberByRefFromSFByValue32, (int8)targetSFOffset, (int8) targetMemberOffset, (int8) SFSourceValueOffset);
			}
			else
			{
				Append_GetStackFrameMemberPtr(VM::REGISTER_D5, targetSFOffset, targetMemberOffset);
				Append_GetStackFrameAddress(VM::REGISTER_D4, SFSourceValueOffset);
				Append_CopyMemory(VM::REGISTER_D5, VM::REGISTER_D4, nBytesSource);
			}
		}

		virtual void Append_SetSFValueFromSFMemberRef(int32 sourceSFOffset, int32 sourceMemberOffset, int32 SFTargetValueOffset, size_t nBytesSource)
		{
			if (IsToInt8Lossless(sourceSFOffset) && IsToInt8Lossless(sourceMemberOffset) && IsToInt8Lossless(SFTargetValueOffset) && nBytesSource == 4)
			{
				AddFourByteInstruction(Opcodes::SetSFValueFromSFMemberByRef32, (int8)sourceSFOffset, (int8) sourceMemberOffset, (int8) SFTargetValueOffset);
			}
			else
			{
				Append_GetStackFrameMemberPtr(VM::REGISTER_D5, sourceSFOffset, sourceMemberOffset);
				Append_GetStackFrameAddress(VM::REGISTER_D4, SFTargetValueOffset);
				Append_CopyMemory(VM::REGISTER_D4, VM::REGISTER_D5, nBytesSource);
			}
		}

		virtual void Append_SetSFMemberByRefFromRegister(DINDEX Dsource, int32 sfOffset, int32 memberOffset, BITCOUNT bitcount)
		{
			if (IsToInt8Lossless(sfOffset) && IsToInt8Lossless(memberOffset) && bitcount == 32)
			{
				AddFourByteInstruction(Opcodes::SetSFMemberByRefFromRegister32, Dsource, (int8) sfOffset, (int8) memberOffset);
			}
			else
			{
				AddThreeByteInstruction(Opcodes::SetSFMemberByRefFromRegisterLong, Dsource, bitcount);
				AddArgument(sfOffset);
				AddArgument(memberOffset);
			//	Append_GetStackFrameValue(sfOffset, VM::REGISTER_D4, BITCOUNT_POINTER); // D4 points to the containing instance				
			//	Append_Poke(Dsource, bitcount, VM::REGISTER_D4, memberOffset);
			}
		}

		virtual void Append_CopyMemory(DINDEX target, DINDEX source, size_t nBytes)
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

		virtual void Append_CopySFVariableFromRef(int32 targetSFOffset, int32 sourcePtrSFOffset, int32 sourceMemberOffset, size_t nBytesSource)
		{
			AddSingleByteInstruction(Opcodes::CopySFVariableFromRef);
			AddArgument(targetSFOffset);
			AddArgument(sourcePtrSFOffset);
			AddArgument(sourceMemberOffset);
			AddArgument(nBytesSource);
		}

		virtual void Append_IncrementPtr(DINDEX sourceAndTarget, int32 value)
		{
			if (value == 0) return;
			if (IsToInt8Lossless(value))	
			{
				AddThreeByteInstruction(Opcodes::IncrementPtr, sourceAndTarget, (uint8)(int8) value);
			}
			else
			{
				AddTwoByteInstruction(Opcodes::IncrementPtrBig, sourceAndTarget);
				AddArgument(value);
			}
		}

		virtual void Append_Return()
		{
			AddSingleByteInstruction(Opcodes::Return);
		}

		virtual void Append_Yield()
		{
			AddSingleByteInstruction(Opcodes::Yield);
		}

		virtual void Append_TripDebugger()
		{
			AddSingleByteInstruction(Opcodes::TripDebugger);
		}

		virtual void Append_GetGlobal(BITCOUNT bits, int32 offset)
		{
			AddTwoByteInstruction(Opcodes::GetGlobal, bits);
			AddArgument(offset);
		}

		virtual void Append_SetGlobal(BITCOUNT bits, int32 offset)
		{
			AddTwoByteInstruction(Opcodes::SetGlobal, bits);
			AddArgument(offset);
		}

		virtual void Clear() { program.clear(); }
		virtual ICore& Core() { return core; }
		virtual void Free() { delete this; }

		virtual size_t WritePosition() const
		{
			return (nextWritePosition != -1) ? nextWritePosition : program.size();
		}

		virtual void SetWriteModeToOverwrite(size_t position)
		{
			this->nextWritePosition = position;
		}

		virtual void SetWriteModeToAppend()
		{
			this->nextWritePosition = -1;
		}

		virtual const unsigned char* Program(OUT size_t& length) const
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

		void Revert(size_t position)
		{
			if (position < program.size())
			{
				program.resize(position);
				nextWritePosition = -1;
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