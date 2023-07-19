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

#pragma once

#ifndef SEXY_COMPILER_HELPERS_H
#define  SEXY_COMPILER_HELPERS_H

#ifndef SEXYCOMPILER_API
# define SEXYCOMPILER_API __declspec(dllimport)
#endif

#include <rococo.stl.allocators.h>

namespace Rococo
{
	namespace Compiler 
	{
		struct INamespaceBuilder;
		struct IStructure;
		struct IInterface;
		struct IProgramObject;

		class CommonStructures
		{
		private:
			INamespaceBuilder* root;
			INamespaceBuilder* sys;
			INamespaceBuilder* sysType;
			INamespaceBuilder* sysNative;
			INamespaceBuilder* sysReflection;

			IStructure* typePointer;
			IStructure* typeBool;
			IStructure* typeInt32;
			IStructure* typeInt64;
			IStructure* typeFloat32;
			IStructure* typeFloat64;
			IStructure* typeNode;
			IStructure* typeArray;
			IStructure* typeMapNode;
			IStructure* typeStringLiteral;
			IStructure* typeExpression;

			IInterface* sysTypeIString;
			IInterface* sysTypeIException;
			IInterface* sysTypeIExpression;

		public:
			CommonStructures(IProgramObject& obj);

			DEFINE_SEXY_ALLOCATORS_FOR_CLASS;

			INamespaceBuilder& Root() { return *root; }
			INamespaceBuilder& Sys() { return *sys; } 
			INamespaceBuilder& SysType() { return *sysType; } 
			INamespaceBuilder& SysNative() { return *sysNative; } 
			INamespaceBuilder& SysReflection() { return *sysReflection; } 
			
			const IStructure& TypePointer() const { return *typePointer; }
			const IStructure& TypeBool() const { return *typeBool; }
			const IStructure& TypeInt32() const { return *typeInt32; }
			const IStructure& TypeInt64() const { return *typeInt64; }
			const IStructure& TypeFloat32() const { return *typeFloat32; }
			const IStructure& TypeFloat64() const { return *typeFloat64; }
			const IStructure& TypeNode() const { return *typeNode; }
			const IStructure& TypeArray() const { return *typeArray; }
			const IStructure& TypeMapNode() const { return *typeMapNode; }
			const IStructure& TypeStringLiteral() const { return *typeStringLiteral; }

			const IStructure& SysReflectionExpression() const { return *typeExpression; }

			const IInterface& SysTypeIString() const { return *sysTypeIString; }
			const IInterface& SysTypeIException() const { return *sysTypeIException; }			
			const IInterface& SysTypeIExpression() const { return *sysTypeIExpression; }	
		};

		SEXYCOMPILER_API const IFunction* GetCurrentFunction(IPublicProgramObject& po, size_t& programOffset, size_t& pcOffset);

		struct ICodeBuilder;
		struct MemberDef;
		void UseStackFrameFor(ICodeBuilder& builder, const MemberDef& def);
		void RestoreStackFrameFor(ICodeBuilder& builder, const MemberDef& def);
	}
} // Sexy.Compiler

#endif