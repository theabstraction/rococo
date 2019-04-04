# pragma once
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

#ifndef SVMCPU_H
#define SVMCPU_H

#include "sexy.vm.h"

#ifdef _WIN32
# define ALIGN_ON_CACHELINE __declspec( align( 128 ) )
#else
# define ALIGN_ON_CACHELINE
#endif

#pragma pack(push,1)

namespace Rococo { namespace VM
{
	enum EXCEPTIONCODE
	{
		EXCEPTIONCODE_NONE = 0,
		EXCEPTIONCODE_BAD_ADDRESS = 1,
		EXCEPTIONCODE_BAD_ALIGN = 2,
		EXCEPTION_CODE_UNKNOWN_CALLBACK = 3,
		EXCEPTIONCODE_DISABLED = 0xFFFFFFFE,
		EXCEPTIONCODE_UNKNOWN  = 0xFFFFFFFF
	};

	// ALWAYS profile performance before changing the CPU structure in any way.
	struct CPU
	{
		enum { DATA_REGISTER_COUNT = 256}; // If you have more than 256 registers then you need to change the instruction format, which uses 8-bit indices
		enum { FIRST_USER_REGISTER = 4};
		VariantValue D[DATA_REGISTER_COUNT]; 
		inline const uint8* PC() const	{ return D[REGISTER_PC].uint8PtrValue; }
		inline uint32& SR()				{ return D[REGISTER_SR].uint32Value;   }
		inline const uint32& SR() const	{ return D[REGISTER_SR].uint32Value;   }
		inline const uint8* SP() const	{ return D[REGISTER_SP].uint8PtrValue; }
		inline const uint8* SF() const	{ return D[REGISTER_SF].uint8PtrValue; }
		inline uint8* SF()				{ return D[REGISTER_SF].uint8PtrValue; }

		inline void SetPC(const uint8* pc) { D[REGISTER_PC].uint8PtrValue = const_cast<uint8*>(pc); }
		inline void SetSF(const uint8* sf) { D[REGISTER_SF].uint8PtrValue = const_cast<uint8*>(sf);}

		inline void AdvancePC(size_t offset) { D[REGISTER_PC].uint8PtrValue += offset; }
		inline void AdvanceSP(size_t offset) { D[REGISTER_SP].uint8PtrValue += offset; }

		template<class T> inline void Push(const T& t)
		{
			T* pStack = (T*) D[REGISTER_SP].vPtrValue;
			*pStack = t;
			D[REGISTER_SP].uint8PtrValue += sizeof(T);
		}

		template<class T> inline T Pop()
		{
			D[REGISTER_SP].uint8PtrValue -= sizeof(T);
			T* pStack = (T*) D[REGISTER_SP].vPtrValue;
			return *pStack;
		}

		uint8* ProgramStart;
		uint8* ProgramEnd;
		uint8* StackStart;
		uint8* StackEnd;
		uint8* Globals;
		EXCEPTIONCODE ExceptionCode;
	} TIGHTLY_PACKED;;

	enum STATUSBIT
	{
		STATUSBIT_EQUIVALENCE = 0x00000001,
		STATUSBIT_NEGATIVE		= 0x00000002,
		STATUSBIT_OVERFLOW		= 0x00000004
	};

	inline bool IsEquiSet(const CPU& cpu)
	{
		return (STATUSBIT_EQUIVALENCE & cpu.SR()) != 0;
	}

	inline bool IsNegSet(const CPU& cpu)
	{
		return (STATUSBIT_NEGATIVE & cpu.SR()) != 0;
	}

	inline bool IsOverflowSet(const CPU& cpu)
	{
		return (STATUSBIT_OVERFLOW & cpu.SR()) != 0;
	}

	namespace Opcodes
	{
		enum OPCODE: uint8
		{
			// Order instructions by frequency
			SetRegisterImmediate64,
			SetRegisterImmediate32,
			SetStackFrameImmediate32,
			SetStackFrameImmediate64,
			SetStackFrameImmediateFar,
			GetStackFrameValue32,
			GetStackFrameValue64,
			GetStackFrameValueFar,
			SetStackFrameValue32,
			SetStackFrameValue64,
			SetSFValueFromSFValue32,
			SetStackFrameValueFar,
			SetSFMemberByRefFromSFByValue32,
			SetSFValueFromSFMemberByRef32,
			SetSFMemberByRefFromRegister32,
			SetSFMemberByRefFromRegisterLong,
			GetStackFrameMemberPtr,
			GetStackFrameMemberPtrFar,
			GetStackFrameMember32,
			GetStackFrameMember64,
			GetStackFrameAddress,
			Test32,
			Test64,
			BranchIfGTE,
			BranchIfGT,
			BranchIfLT,
			BranchIfNE,
			AddQuick32,
			Increment32,
			Decrement32,
			Move32,
			Move64,
			Poke64,
			Poke32,
			Pop,
			PushIndirect,
			PushRegister32,
			PushRegister64,
			PushImmediate32,
			PushAddress,
			PushStackVariable32,
			PushStackAddress,
			BooleanNot,
			Call,
			CallBy,
			CallById,
			CallByIdIndirect,
			CallVitualFunctionViaRefOnStack,
			CallVirtualFunctionByAddress,
			CopySFMemory,
			CopySFMemoryNear,
			CopyMemory,
			CopyMemoryBig,		
			CopySFVariableFromRef,
			Copy32Bits,
			Copy64Bits,
			IncrementPtr,
			IncrementPtrBig,
			Invoke,
			InvokeBy,
			Return,
			Yield,
			IntAdd64,
			IntSubtract64,
			IntMultiply64,
			IntNegate64,
			IntDivide64,			
			IntAdd32,
			IntSubtract32,
			IntMultiply32,
			IntNegate32,
			IntDivide32,	
			LogicalAND32,
			LogicalAND64,
			LogicalOR32,
			LogicalOR64,
			LogicalXOR32,
			LogicalXOR64,
			LogicalNOT32,
			LogicalNOT64,
			Branch,
			BranchIf,
			SetIf32,
			SetIf64,
			Swap,
			AddImmediate,
			ShiftLeft32,
			ShiftRight32,
			ShiftLeft64,
			ShiftRight64,
			StackAlloc,
			StackAllocBig,
			Exit,
			FloatAdd,
			FloatSubtract,
			FloatMultiply,
			FloatDivide,
			DoubleAdd,
			DoubleSubtract,
			DoubleMultiply,
			DoubleDivide,
			NoOperation,
			RestoreRegister32,
			SaveRegister32,
			RestoreRegister64,
			SaveRegister64,
			TripDebugger,
			GetGlobal,
			SetGlobal,
			GetStackFrameValueAndExtendToPointer,
			IllegalOverflowOp
		};

		enum { MAX_OPCODES = 256 }  ;
	}

	struct Ins
	{
		uint8 Opcode;
		uint8 Opmod1;
		uint8 Opmod2;
		uint8 Opmod3;

		const uint8* ToPC() const { return (const uint8*) this; }
	} TIGHTLY_PACKED;

	struct ArgsCallVitualFunctionViaRefOnStack
	{
		Opcodes::OPCODE opcode;
		int32 SFoffsetToInterfaceRef;
		int32 vTableOffset;
		int32 instanceToInterfaceOffset;
	} TIGHTLY_PACKED;
}}

#pragma pack(pop)

#endif