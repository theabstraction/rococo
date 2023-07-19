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

#include "sexy.vm.stdafx.h"

#include <sexy.stdstrings.h>
#include <sexy.unordered_map.h>
#include <sexy.vector.h>

using namespace Rococo;
using namespace Rococo::VM;

namespace
{
	class SourceImage final: public ISourceFile
	{
	private:
		rstdstring sourceName;
		
		typedef TSexyVector<size_t> TOffsets;
		TOffsets rowOffsets;

	public:
		SourceImage(const char* _sourceName, const char* srcCode, size_t codeLenBytes):
			sourceName(_sourceName)
		{
			rowOffsets.push_back(0); // Row 1 always starts at offset 0

			if (srcCode != NULL)
			{	
				if (codeLenBytes == -1) codeLenBytes = strlen(srcCode);

				if (codeLenBytes > 1)
				{
					for(size_t i = 0; i < codeLenBytes-1; i++)
					{
						char c = srcCode[i];
						if (c == '\n')
						{
							rowOffsets.push_back(i+1);
						}
					}
				}
			}
		}

		virtual size_t GetCodeOffset( const Vec2i& pos ) const
		{
			if (pos.y == 0 || pos.y > (int) rowOffsets.size()) return (size_t) -1;

			return rowOffsets[pos.y-1] + pos.x;
		}

		virtual const char* FileName() const
		{
			return sourceName.c_str();
		}
	};

	class Symbols final: public ISymbols
	{
   private:
		typedef TSexyHashMap<size_t,VM::FileData> TOffsetToSymbol;
		TOffsetToSymbol symbolMap;

		typedef TSexyVector<SourceImage*> TImages;
		TImages images;
		
	public:
		Symbols()
		{

		}

		~Symbols()
		{
			Clear();
		}

		virtual bool TryGetSymbol( size_t index, OUT VM::FileData& fd ) const
		{
			TOffsetToSymbol::const_iterator i = symbolMap.find(index);
			if (i != symbolMap.end())
			{
				OUT fd = i->second;
				return true;
			}
			else
			{
				fd.Source = NULL;
            fd.Pos = Vec2i{ 0,0 };
				return false;
			}
		}

		virtual void Add(size_t index, const VM::FileData& fp ) 
		{
			symbolMap.insert(std::make_pair(index, fp));
		}

		virtual ISourceFile* AddSourceImage(const char* sourceName, const char* sourceCode, size_t srcLenBytes )
		{
			SourceImage* si = new SourceImage(sourceName, sourceCode, srcLenBytes);
			images.push_back(si);
			return si;
		}

		virtual void Clear() 
		{
			for(auto i = images.begin(); i != images.end(); ++i)
			{
				delete *i;
			}

			images.clear();
			symbolMap.clear();
		}

		virtual void Free()
		{
			delete this;
		}
	};
}

namespace Rococo { namespace VM
{
	ISymbols* CreateSymbolTable(ICore&)
	{
		return new Symbols();
	}
}} // Rococo::VM