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

#pragma once

#ifndef SEXY_DEBUG_H
# define  SEXY_DEBUG_H

#ifndef SEXYUTIL_API
# define SEXYUTIL_API ROCOCO_API_IMPORT
#endif

// This header does not reference other sexy headers, or other sexy types to minimize the exposure of third party debuggers to sexy types

namespace Rococo
{
	namespace Compiler
	{
		struct IStructure;
		struct MemberDef;
	}

	namespace Debugger
	{
		struct ICallStackEnumerationCallback
		{
			virtual void OnStackLevel(const unsigned char* sf, const char* functionDef) = 0;
		};

		struct IRegisterEnumerationCallback
		{
			virtual void OnRegister(const char* name, const char* value) = 0;
		};

		struct VariableDesc
		{
			enum { VALUE_CAPACITY = 128, TYPE_CAPACITY = 128, LOCATION_CAPACITY = 16, NAME_CAPACITY = 128 };

			char Name[NAME_CAPACITY];
			char Type[TYPE_CAPACITY];
			char Value[VALUE_CAPACITY];
			char Location[LOCATION_CAPACITY];

			ptrdiff_t Address;
			const Compiler::IStructure* s;
			cstr parentName;
			const uint8* instance;
		};

		DECLARE_ROCOCO_INTERFACE IVariableEnumeratorCallback
		{
			virtual void OnVariable(size_t index, const VariableDesc& variable, const Rococo::Compiler::MemberDef& def) = 0;
		};
	}
}

namespace Rococo::VM
{
	struct CPU;
}

namespace Rococo::Script
{
	SEXYUTIL_API void EnumerateRegisters(Rococo::VM::CPU& cpu, Rococo::Debugger::IRegisterEnumerationCallback& cb);
}

#endif