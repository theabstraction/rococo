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

#ifndef SEXY_H
# define SEXY_H

#include <rococo.types.h>
#include <stdarg.h>

#if !defined(_W64)
# if !defined(__midl) && (defined(_X86_) || defined(_M_IX86))
#  define _W64 __w64
# else
#  define _W64
# endif
#endif

#ifndef _PTRDIFF_T_DEFINED
# ifdef  _WIN64
   typedef __int64 ptrdiff_t;
# else

# endif
# define _PTRDIFF_T_DEFINED
#endif

// #define SEXCHAR_IS_WIDE // Comment this out to make the codebase use ASCII, otherwise it will use 16-bit UNICODE

#define IN
#define OUT
#define REF

#ifndef _WIN64
# ifdef _WIN32
 #error "Sexy does not supports anything other than 64-bit platforms." 
# endif
#endif

#define POINTERS_ARE_64_BIT

namespace Sexy
{
	using namespace Rococo;

   namespace OS
   {
      void TripDebugger();
      bool IsDebuggerPresent();
   }

#ifndef _WIN32
# define _TRUNCATE ((size_t)-1)
   typedef int32 errno_t;
   void memcpy_s(void *dest, size_t destSize, const void *src, size_t count);
   int sscanf_s(const char* buffer, const char* format, ...);
   int sprintf_s(char* buffer, size_t capacity, const char* format, ...);
   int _vsnprintf_s(char* buffer, size_t capacity, size_t maxCount, const char* format, va_list args);
   
   template<size_t _Size>
   inline int _snprintf_s(char(&buffer)[_Size], size_t maxCount, const char* format, ...)
   {
      va_list args;
      va_start(args, format);
      return _vsnprintf_s(buffer, _Size, maxCount, format, args);
   }

   template<size_t _Size>
   inline int _vsnprintf_s(char(& buffer)[_Size], size_t maxCount, const char* format, va_list args)
   {
      return _vsnprintf_s(buffer, _Size, maxCount, format, args);
   }

   template<size_t _Size>
   inline int sprintf_s(char(&buffer)[_Size], const char* format, ...)
   {
      va_list args;
      va_start(args, format);
      return _vsnprintf_s(buffer, _Size, _TRUNCATE, format, args);
   }
       
   void strcpy_s(char* dest, size_t capacity, const char* source);
   void strncpy_s(char* dest, size_t capacity, const char* source, size_t maxCount);
   void strcat_s(char* dest, size_t capacity, const char* source);
   void strncat_s(char* dest, size_t capacity, const char* source, size_t maxCount);

# define _MAX_PATH 260 // There is no good answer for this
#endif

	typedef size_t ID_BYTECODE;

	enum CALLBACK_CONTROL
	{
		CALLBACK_CONTROL_CONTINUE,
		CALLBACK_CONTROL_BREAK
	};

	struct NULL_CONTEXT {};

	template<class T, class CONTEXT = NULL_CONTEXT> struct ICallback
	{
		virtual CALLBACK_CONTROL operator()(T& value, CONTEXT context) = 0;
	};

	template<class T> struct ICallback<T, NULL_CONTEXT>
	{
		virtual CALLBACK_CONTROL operator()(T& value) = 0;
	};

	template<> struct ICallback<cstr, NULL_CONTEXT>
	{
		virtual CALLBACK_CONTROL operator()(cstr value) = 0;
	};

	namespace Sex
	{
		class ParseException;
      void Throw(ParseException& ex);
	}

	enum {ROOT_TEMPDEPTH = 3};

	typedef unsigned long refcount_t;

	typedef uint32 ID_API_CALLBACK;

	struct VTABLEDEF
	{
		const ID_BYTECODE* Root;
	};

	enum { NAMESPACE_MAX_LENGTH = 256 };

#ifndef SEXCHAR_IS_WIDE
	typedef char SEXCHAR;
# define SEXTEXT(quote) quote 
# define __SEXFUNCTION__ __FUNCTION__
#else
	typedef rchar SEXCHAR;
# define SEXSTRINGIFY(x) L ## x
# define SEXSTRINGIFY2(x) SEXSTRINGIFY(x)
# define SEXTEXT(quote) L##quote
# define __SEXFUNCTION__ SEXSTRINGIFY2(__FUNCTION__)
#endif

   const SEXCHAR OS_DIRECTORY_SLASH = (SEXCHAR) '\\';

	typedef const SEXCHAR* csexstr;

   struct strbuilder
   {
      virtual strbuilder& AppendFormat(const char* format, ...) = 0;
      virtual strbuilder& operator << (csexstr text) = 0;
      virtual strbuilder& operator << (int32 value) = 0;
      virtual strbuilder& operator << (uint32 value) = 0;
      virtual strbuilder& operator << (int64 value) = 0;
      virtual strbuilder& operator << (uint64 value) = 0;
      virtual fstring operator * () const = 0;
   };

   class StackStringBuilder : public strbuilder
   {
   private:
      char* buffer;
      size_t capacity;
      int32 length;
   public:
      StackStringBuilder(char* _buffer, size_t _capacity);
      fstring operator * () const override { return fstring{ buffer, length }; }
      strbuilder& AppendFormat(const char* format, ...) override;
      strbuilder& operator << (csexstr text) override;
      strbuilder& operator << (int32 value)  override;
      strbuilder& operator << (uint32 value) override;
      strbuilder& operator << (int64 value)  override;
      strbuilder& operator << (uint64 value) override;
   };

	struct sexstring_key
	{
		int64 Length;
		csexstr Text;

		sexstring_key(csexstr text, int64 length): Length(length), Text(text) 	{}
	};

	struct TokenBuffer
	{
		enum { MAX_TOKEN_CHARS = 256 };
		SEXCHAR Text[MAX_TOKEN_CHARS];
		operator const SEXCHAR* ()	{ return Text; }
	};

#pragma pack(push,1)
	struct sexstring_header
	{
		int32 Length;
		SEXCHAR Buffer[1];
	};

	namespace Compiler
	{
		struct VirtualTable;
	}

	struct IString
	{
		Compiler::VirtualTable* vTable;
		int32 length; 
		csexstr buffer;
	};

   ROCOCOAPI IStringPopulator
   {
      virtual void Populate(csexstr text);
   };

#pragma pack(pop)

	typedef sexstring_header* sexstring;

	bool CALLTYPE_C IsCapital(SEXCHAR c);
	bool CALLTYPE_C IsLowerCase(SEXCHAR c);
	bool CALLTYPE_C IsAlphabetical(SEXCHAR c);
	bool CALLTYPE_C IsNumeric(SEXCHAR c);
	bool CALLTYPE_C IsAlphaNumeric(SEXCHAR c);

	void GetRefName(OUT TokenBuffer& token, csexstr name);

	int CALLTYPE_C StringPrintV(char* buf, size_t sizeInChars, va_list args, const char* format);
	int CALLTYPE_C StringPrintV(rchar* buf, size_t sizeInChars, va_list args, cstr format);
	int CALLTYPE_C StringPrint(char* buf, size_t sizeInChars, const char* format, ...);
	int CALLTYPE_C StringPrint(rchar* buf, size_t sizeInChars, cstr format, ...);
	int CALLTYPE_C StringPrint(TokenBuffer& buf, const SEXCHAR* format, ...);

   int CALLTYPE_C WriteToStandardOutput(const char* text, ...);
	int CALLTYPE_C WriteToStandardOutput(cstr text, ...);

	int32 CALLTYPE_C StringLength(const char* s);
	int32 CALLTYPE_C StringLength(cstr s);
	void CALLTYPE_C CopyChars(SEXCHAR* dest, const sexstring source);
	void CALLTYPE_C CopyString(char* dest, size_t capacity, const char* source);
	void CALLTYPE_C CopyString(char* dest, size_t destCapacity, const char* source, int maxChars); // use maxChars -1 to truncate

	void CALLTYPE_C StringCat(rchar* buf, cstr source, int maxChars);
	void CALLTYPE_C StringCat(char* buf, const char* source, int maxChars);

	bool TryParseSexHex(SEXCHAR& finalChar, csexstr s);
	bool ParseEscapeCharacter(SEXCHAR& finalChar, SEXCHAR c);

	size_t Hash(csexstr text);
	int32 Hash(csexstr s, int64 length);
	int32 Hash(int32 x);
	int32 Hash(int64 x);

	sexstring CreateSexString(csexstr src, int32 length = -1);
	void FreeSexString(sexstring s);

   ROCOCOAPI ILog
	{
		virtual void Write(csexstr text) = 0;
		virtual void OnUnhandledException(int errorCode, csexstr exceptionType, csexstr message, void* exceptionInstance) = 0; // thrown by uncaught sexy exceptions
		virtual void OnJITCompileException(Sex::ParseException& ex) = 0; // thrown by compile errors during JIT execution
	};

	void LogError(ILog& log, csexstr format, ...);

	struct Vector4
	{
		float X;
		float Y;
		float Z;
		float W;
	};

	union VariantValue
	{
		char charValue;
		unsigned char ucharValue;

		uint8 uint8Value;
		uint16 uint16Value;
		uint32 uint32Value;
		uint64 uint64Value;

		int8 int8Value;
		int16 int16Value;
		int32 int32Value;
		int64 int64Value;
		size_t sizetValue;
		ptrdiff_t ptrDiffValue;

		uint8* uint8PtrValue;
		uint16* uint16PtrValue;
		uint32* uint32PtrValue;
		uint64* uint64PtrValue;
		float32* float32PtrValue;
		float64* float64PtrValue;

		int8* int8PtrValue;
		int16* int16PtrValue;
		int32* int32PtrValue;
		int64* int64PtrValue;

		size_t* size_tPtrValue;
		char* charPtrValue;
		rchar* rcharPtrValue;
		void* vPtrValue;
		float floatValue;
		double doubleValue;
		Vector4* vec4PtrValue;
		ID_API_CALLBACK apiValue;
		ID_BYTECODE byteCodeIdValue;
		const ID_BYTECODE* vTable;
	};

	enum BITCOUNT
	{
		BITCOUNT_BAD = 0,
		BITCOUNT_32 = 32,
		BITCOUNT_64 = 64,
		BITCOUNT_128 = 128,
		BITCOUNT_ID_API = 8 * sizeof(ID_API_CALLBACK),

#ifdef _WIN64
      BITCOUNT_POINTER = 64
#else
# ifdef _Win32
      BITCOUNT_POINTER = 32
# else
      BITCOUNT_POINTER = 64
# endif
#endif
	};

	enum CONDITION
	{
		CONDITION_IF_EQUAL,
		CONDITION_IF_NOT_EQUAL,
		CONDITION_IF_GREATER_THAN,
		CONDITION_IF_LESS_THAN,
		CONDITION_IF_GREATER_OR_EQUAL,
		CONDITION_IF_LESS_OR_EQUAL
	};

	enum EXECUTERESULT
	{
		EXECUTERESULT_RUNNING = 0,  // The bytecode is running or about to be run
		EXECUTERESULT_TERMINATED, // The bytecode exited and returned an exit code
		EXECUTERESULT_YIELDED, // The bytecode stopped executing, but can safely continue where it left off
		EXECUTERESULT_BREAKPOINT, // Same as yield, but the cause was a breakpoint which should invoke a debugger
		EXECUTERESULT_NO_PROGRAM, // Cannot execute, as no program is loaded into the VM 
		EXECUTERESULT_NO_ENTRY_POINT, // Cannot find the entry point in the program
		EXECUTERESULT_RETURNED, // A function was executed and returned its value on the stack
		EXECUTERESULT_THROWN, // A throw was invoked, generally happens when an exeption has no handler
		EXECUTERESULT_ILLEGAL, // An illegal instruction was executed, happens when a compiler/gremlins creates duff bytecode
		EXECUTERESULT_SEH // An SEH exception was caught and terminated execution of the VM
	};

	namespace OS
	{
      enum BreakFlag
      {
         BreakFlag_None = 0,
         BreakFlag_STC = 1,
         BreakFlag_VM = 2,
         BreakFlag_SS= 4,
         BreakFlag_All = 7
      };
      void SetBreakPoints(int32 flags);
		void BreakOnThrow(BreakFlag targetFlag);
		void LoadAsciiTextFile(SEXCHAR* data, size_t capacity, const SEXCHAR* filename);
		void GetEnvVariable(SEXCHAR* data, size_t capacity, const SEXCHAR* envVariable);
      void Throw(int erroCode, csexstr format, ...);
	}

	template<class T> bool IsFlagged(T flags, T flag) { return (flags & flag) != 0; }

	class NamespaceSplitter
	{
	private:
		csexstr src;
		int length;
		SEXCHAR dottedName[NAMESPACE_MAX_LENGTH];

	public:
		NamespaceSplitter(csexstr _src);
		bool SplitTail(csexstr& _body, csexstr& _tail);
		bool SplitHead(csexstr& _head, csexstr& _body);
	};

	namespace Sex
	{
		struct ISParser;
		struct ISParserTree;
		struct ISExpression;
		struct ISourceCode;

		enum EXPRESSION_TYPE
		{
			EXPRESSION_TYPE_NULL,
			EXPRESSION_TYPE_STRING_LITERAL,
			EXPRESSION_TYPE_ATOMIC,
			EXPRESSION_TYPE_COMPOUND,		
		};

		struct ISExpressionBuilder;

      ROCOCOAPI ISExpression
		{
			virtual const Vec2i& Start() const = 0; // (X.Y) of start relative to tree origin in source file
			virtual const Vec2i& End() const = 0;// (X.Y) of end relative to tree origin in source file
			virtual size_t StartOffset() const = 0; // Displacement from the start of source code to the start of the expression
			virtual size_t EndOffset() const = 0;// Displacement from the end of source code to the start of the expression
			virtual EXPRESSION_TYPE Type() const = 0;
			virtual const sexstring String() const = 0;
			virtual const ISParserTree& Tree() const = 0;
			virtual int NumberOfElements() const = 0;
			virtual const ISExpression& GetElement(int index) const = 0;
			virtual const ISExpression& operator[](int index) const { return GetElement(index); }
			virtual const ISExpression* Parent() const = 0;
			virtual ISExpressionBuilder* CreateTransform() = 0;
			virtual const ISExpression* GetTransform() const = 0;
			virtual const ISExpression* GetOriginal() const = 0;
			virtual const int TransformDepth() const = 0;
			virtual bool operator == (const SEXCHAR* token) const = 0;
		};

		inline bool operator != (const ISExpression& s, const SEXCHAR* token)
		{
			return !(s == token);
		}

		ROCOCOAPI ISExpressionBuilder : public ISExpression
		{
			virtual ISExpressionBuilder* AddChild() = 0;
			virtual void AddAtomic(csexstr text) = 0;
			virtual void AddStringLiteral(csexstr text) = 0;
		};

		typedef const ISExpression& cr_sex;

		ROCOCOAPI IRefCounted
		{
			virtual refcount_t AddRef() = 0; // Increments the reference count and returns the new value
			virtual refcount_t Release() = 0; // Decrements the reference count and returns the new value. If it decrements to zero the instance is released
		};

		ROCOCOAPI ISParserTree : public IRefCounted
		{
			virtual ISExpression& Root() = 0; // Refers to the root expression in the tree
			virtual const ISExpression& Root() const = 0; // // Constant reference to the root expression in the tree
			virtual ISParser& Parser() = 0; // Refers to the object that created this instance
			virtual const ISourceCode& Source() const = 0; // The source code associated with this parser tree
		};

		ROCOCOAPI ISParser : public IRefCounted
		{
			virtual ISParserTree* CreateTree(ISourceCode& sourceCode) = 0; // Creates a new s-parser tree with a reference count of 1, and attaches a reference to ISourceCode 
			virtual ISourceCode* DuplicateSourceBuffer(csexstr buffer, int segmentLength, const Vec2i& origin, csexstr name) = 0; // Duplicates a source segment and exposes as an instance
			virtual ISourceCode* ProxySourceBuffer(csexstr bufferRef, int segmentLength, const Vec2i& origin, csexstr nameRef) = 0; // Duplicates the pointers defining the source code and its name and exposes as an instance
			virtual ISourceCode* LoadSource(csexstr filename, const Vec2i& origin) = 0; // Loads source code, converts it to SEXCHARs and returns a reference to it
			virtual ISourceCode* LoadSource(csexstr moduleName, const Vec2i& origin, const char* buffer, long len) = 0;
		};	

		ROCOCOAPI ISourceCode : public IRefCounted
		{
			virtual const Vec2i& Origin() const = 0; // The XY in the source document where the code segment begins
			virtual csexstr SourceStart() const = 0; // The first SEXCHAR in the source code segment
			virtual const int SourceLength() const = 0;  // The number of SEXCHARS in the source code segment
			virtual csexstr Name() const = 0; // The name of the source segment
		};

		csexstr ReadUntil(const Vec2i& pos, const ISourceCode& src);
		void GetSpecimen(SEXCHAR specimen[64], const ISExpression& e);
	} // Sex

	enum VARTYPE
	{
		VARTYPE_Bool,
		VARTYPE_Float32,
		VARTYPE_Float64,
		VARTYPE_Int32,
		VARTYPE_Int64,
		VARTYPE_Pointer,
		VARTYPE_Derivative,
		VARTYPE_Bad,
		VARTYPE_Closure,
		VARTYPE_AnyNumeric // Not really a type, passed to a function to indicate any numeric type is valid
	};

	namespace Parse
	{
		enum PARSERESULT
		{
			PARSERESULT_GOOD,
			PARSERESULT_HEXADECIMAL_INCORRECT_NUMBER_OF_DIGITS,
			PARSERESULT_HEXADECIMAL_BAD_CHARACTER,
			PARSERESULT_OVERFLOW,
			PARSERESULT_UNDERFLOW,
			PARSERESULT_BAD_DECIMAL_DIGIT,
			PARSERESULT_HEX_FOR_FLOAT,
			PARSERESULT_UNHANDLED_TYPE
		};

		VARTYPE GetLiteralType(csexstr candidate);
		bool TryGetDigit(OUT int32& value, SEXCHAR c);
		bool TryGetHex(OUT int32& value, SEXCHAR c);
		csexstr VarTypeName(VARTYPE type);
		PARSERESULT TryParseFloat(OUT float32& value, IN csexstr decimalDigits);
		PARSERESULT TryParseFloat(OUT float64& value, IN csexstr decimalDigits);
		PARSERESULT TryParseHex(OUT int32& value, IN csexstr hexDigits);
		PARSERESULT TryParseHex(OUT int64& value, IN csexstr hexDigits);
		PARSERESULT TryParseBoolean(OUT int32& value, IN csexstr valueLiteral);
		PARSERESULT TryParseDecimal(OUT int32& value, IN csexstr valueLiteral);
		PARSERESULT TryParseDecimal(OUT int64& value, IN csexstr valueLiteral);
		PARSERESULT TryParse(OUT VariantValue& value, VARTYPE type, IN csexstr valueLiteral);
		PARSERESULT TryParseExponentForm(OUT double& y, csexstr s);
		PARSERESULT TryParseExponentForm(OUT float& y, csexstr s);
		PARSERESULT TryParse(VariantValue& value, VARTYPE type, csexstr valueLiteral);		
		bool ContainsPoint(csexstr s);
	}

   void Throw(int errorCode, csexstr format, ...);

   namespace Sex
   {
      void AssertCompound(cr_sex e);
      void AssertAtomic(cr_sex e);
      void AssertStringLiteral(cr_sex e);
      void AssertNotTooManyElements(cr_sex e, int32 maxElements);
      void AssertNotTooFewElements(cr_sex e, int32 minElements);
      cr_sex GetAtomicArg(cr_sex e, int argIndex);
      void Throw(cr_sex e, csexstr message, ...);
   }
} // Sexy

#endif