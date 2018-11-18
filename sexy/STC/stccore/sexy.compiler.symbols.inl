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

namespace
{
	int HashString(cstr s)
	{
		int h = 0;
		for (cstr p = s; *p != 0; p++)
		{
			h = 31*h + (int) *p;
		}

		return h;
	}

	enum ESymbol { CSYMBOL_PERMANENT };

	class CSymbol
	{
	private:
		cstr staticRefSymbol;
		int hashCode;
		sexstring symbol;

	public:
		CSymbol(cstr _symbol): staticRefSymbol(_symbol), symbol(NULL)
		{
			hashCode = HashString(_symbol);
		}

		CSymbol(cstr _symbol, int _hashCode, ESymbol perm)
		{
			hashCode = _hashCode;			
			symbol = CreateSexString(_symbol);
			staticRefSymbol = symbol->Buffer;
		}

		CSymbol(const CSymbol& other): symbol(other.symbol)
		{
			hashCode = other.hashCode;
			staticRefSymbol = symbol->Buffer;
		}

		void Erase() const
		{
			FreeSexString(symbol);
		}

		cstr c_str() const { return staticRefSymbol; }
		int HashCode() const { return hashCode; }		
	};

	struct hashSymbol
	{
		size_t operator()(const CSymbol &symbol) const
		{
			return symbol.HashCode();
		}
	};

	bool operator == (const CSymbol& a, const CSymbol& b)
	{
		return AreEqual(a.c_str(), b.c_str());
	}

	typedef std::unordered_set<CSymbol,hashSymbol> TSymbols;

	cstr AddSymbol(TSymbols& symbols, cstr symbol)
	{
		CSymbol temp(symbol);
		auto i = symbols.find(temp);
		if (i == symbols.end())
		{
			CSymbol perm(symbol, temp.HashCode(), CSYMBOL_PERMANENT);
			symbols.insert(perm);
			return perm.c_str();
		}
		else
		{
			return i->c_str();
		}
	}

	void ClearSymbols(TSymbols& symbols)
	{
		for(TSymbols::const_iterator i = symbols.begin(); i != symbols.end(); ++i)
		{
			const CSymbol& symbol = *i;
			symbol.Erase();
		}

		symbols.clear();
	}
}