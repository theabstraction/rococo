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

#include "Sexy.VM.StdAfx.h"
#include "Sexy.VM.OS.h"

#include <algorithm>

using namespace Sexy;
using namespace Sexy::VM;

namespace
{
	class ProgramMemory: public IProgramMemory
	{
	private:
		uint8* memory;
		size_t allocSize;
		size_t nextStartIndex;
		ID_BYTECODE nextId;
		uint32 refCount;

		struct FunctionDef
		{
			size_t StartIndex;
			size_t Length;
			size_t UsedLength;
		};

		typedef std::tr1::unordered_map<ID_BYTECODE,FunctionDef> TFunctions;
		TFunctions functions;

		int version;
	public:
		ProgramMemory(size_t maxProgramSize)
		{
			allocSize = maxProgramSize + 32768 - (maxProgramSize % 32768);
			memory = (uint8*) VM::OS::AllocAlignedMemory(allocSize);
			nextStartIndex = 0;
			nextId = 1;
			refCount = 1;

		}

		~ProgramMemory()
		{
			VM::OS::FreeAlignedMemory(memory, allocSize);
		}

		virtual void AddRef()
		{
			refCount++;
		}
		
		virtual void Release()
		{
			refCount--;
			if (refCount == 0) 
			{
				delete this;
			}
		}

		virtual void Clear()
		{
			functions.clear();
			nextStartIndex = 0;
			nextId = 1;
		}

		virtual uint8* StartOfMemory() { return memory; }
		virtual uint8* EndOfMemory()  { return memory + allocSize;	}
		virtual const uint8* StartOfMemory() const { return memory; }
		virtual const uint8* EndOfMemory() const { return memory + allocSize;	}

		virtual ID_BYTECODE AddBytecode() 
		{
			FunctionDef fd;		
			fd.Length = 0;
			fd.StartIndex = 0;
			fd.UsedLength = 0;

			ID_BYTECODE id = nextId++;
			functions.insert(std::make_pair(id, fd));

			return id;
		}
			
		virtual void UnloadBytecode(ID_BYTECODE id) 
		{
			auto i = functions.find(id);
			if (i != functions.end())
			{
				FunctionDef& fd = i->second;
				memset(fd.StartIndex + memory, 0, fd.Length);
				functions.erase(i);
			}			
		}

		size_t GetFunctionAddress(ID_BYTECODE id) const
		{
			auto i = functions.find(id);			 
			if (i != functions.end())
			{
				const FunctionDef& fd = i->second;
				return fd.StartIndex;
			}
			else
			{
				return (size_t) -1;
			}
		}

		ID_BYTECODE GetFunctionContaingAddress(size_t pcOffset) const
		{
			for(auto i = functions.begin(); i != functions.end(); ++i)
			{
				const FunctionDef& fd = i->second;
				if (pcOffset >= fd.StartIndex && pcOffset < fd.StartIndex + fd.Length)
				{
					return i->first;
				}
			}

			return 0;
		}

		size_t GetFunctionLength(ID_BYTECODE id) const
		{
			TFunctions::const_iterator i = functions.find(id);			 
			if (i != functions.end())
			{
				const FunctionDef& fd = i->second;
				return fd.UsedLength;
			}
			else
			{
				return (size_t) -1;
			}
		}

		virtual bool UpdateBytecode(ID_BYTECODE id, const IAssembler& assembler) 
		{
			size_t newLength;
			const uint8* byteCode = assembler.Program(OUT newLength);

			auto i = functions.find(id);
			if (i != functions.end())
			{
				FunctionDef& fd = i->second;

				fd.UsedLength = newLength;

				if (newLength <= fd.Length)
				{
					memcpy(memory + fd.StartIndex, byteCode, newLength);
					memset(memory + fd.StartIndex + newLength, 0, fd.Length - newLength);
					return true;
				}
				else
				{
					memset(memory + fd.StartIndex, 0, fd.Length);

					if (allocSize - nextStartIndex < newLength)
					{
						// Insufficient space to hold function
						return false;
					}

					fd.Length = newLength;
					memcpy(memory + nextStartIndex, byteCode, newLength);
					fd.StartIndex = nextStartIndex;
					nextStartIndex += newLength;			
					return true;
				}
			}
			else
			{
				return false;
			}
		}
	};
}

namespace Sexy { namespace VM
{
	IProgramMemory* CreateProgramMemory(size_t maxProgramSize)
	{
		return new ProgramMemory(maxProgramSize);
	}
}}