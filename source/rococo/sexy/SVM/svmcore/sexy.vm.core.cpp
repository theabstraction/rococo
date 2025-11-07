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
#include "sexy.strings.h"
#include "sexy.stdstrings.h"
#include "sexy.unordered_map.h"

using namespace Rococo;
using namespace Rococo::VM;

namespace Rococo { namespace VM
{
	IVirtualMachine* CreateVirtualMachine(ICore& core);
	IAssembler* CreateAssembler(ICore& core);
	IDisassembler* CreateDisassembler(ICore& core);
	ISymbols* CreateSymbolTable(ICore& core);
	IProgramMemory* CreateProgramMemory(size_t capacity);

	struct ApiCallbackBinding
	{
		FN_API_CALLBACK Callback;
		void* Context;
		rstdstring Symbol;
	};
}} // Rococo::VM

namespace
{
	class Core final: public ICore
	{
	private:
		ILog* logger;
		ID_API_CALLBACK nextId;
		typedef TSexyHashMap<ID_API_CALLBACK, ApiCallbackBinding> TMapIdToCallback;
		TMapIdToCallback callbacks;
	public:
		Core(const CoreSpec&):
			logger(NULL),nextId(1)
		{

		}

		DEFINE_SEXY_ALLOCATORS_FOR_CLASS;

		IVirtualMachine* CreateVirtualMachine() override
		{
			IVirtualMachine* vm = Rococo::VM::CreateVirtualMachine(*this);
			return vm;
		}

		IAssembler* CreateAssembler() override
		{
			return Rococo::VM::CreateAssembler(*this);
		}

		IProgramMemory* CreateProgramMemory(size_t capacity) override
		{
			return Rococo::VM::CreateProgramMemory(capacity);
		}

		IDisassembler* CreateDisassembler() override
		{
			return Rococo::VM::CreateDisassembler(*this);
		}

		ISymbols* CreateSymbolTable() override
		{
			return Rococo::VM::CreateSymbolTable(*this);
		}

		bool TryInvoke(ID_API_CALLBACK id, VariantValue* registers) override
		{
			TMapIdToCallback::const_iterator i = callbacks.find(id);
			if (i != callbacks.end())
			{
				const ApiCallbackBinding& binding = i->second;
				binding.Callback(registers, binding.Context);
				return true;
			}
			else
			{
				return false;
			}
		}

		cstr GetCallbackSymbolName(ID_API_CALLBACK id) override
		{
			TMapIdToCallback::const_iterator i = callbacks.find(id);
			return (i != callbacks.end()) ? i->second.Symbol.c_str() : NULL;
		}

		ID_API_CALLBACK RegisterCallback(FN_API_CALLBACK callback, void* context, cstr symbol) override
		{
			ID_API_CALLBACK id = nextId++;
			ApiCallbackBinding binding;
			binding.Callback = callback;
			binding.Context = context;
			binding.Symbol = symbol;
			callbacks.insert(std::make_pair(id, binding));
			return id;
		}

		void UnregisterCallback(ID_API_CALLBACK id) override
		{
			callbacks.erase(id);
		}

		void Log(cstr text) override
		{
			if (logger != NULL) logger->Write(text);
		}

		void SetLogger(ILog* log) override
		{
			logger = log;
		}

		void Free() override
		{
			delete this;
		}
	};
}

namespace Rococo { namespace VM
{
	ICore* CreateSVMCore(const CoreSpec& spec)
	{
		return new Core(spec);
	}
}}