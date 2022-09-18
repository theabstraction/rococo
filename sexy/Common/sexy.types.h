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

#ifndef SEXY_H
# define SEXY_H

#include <rococo.types.h>
#include <stdarg.h>

namespace Rococo
{
	typedef void* OS_HWND;

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
		[[noreturn]] void Throw(ParseException& ex);
	}

	enum { ROOT_TEMPDEPTH = 3 };

	typedef unsigned long refcount_t;

	typedef uint32 ID_API_CALLBACK;

	enum { NAMESPACE_MAX_LENGTH = 128 };

#define __SEXFUNCTION__ __FUNCTION__


#ifdef _WIN32
	const char OS_DIRECTORY_SLASH = (char) '\\';
#else
	const char OS_DIRECTORY_SLASH = (char) '/';
#endif

	struct sexstring_key
	{
		int64 Length;
		cstr Text;

		sexstring_key(cstr text, int64 length) : Length(length), Text(text) {}
	};

	struct TokenBuffer
	{
		enum { MAX_TOKEN_CHARS = 256 };
		char Text[MAX_TOKEN_CHARS];
		operator const char* () { return Text; }
	};

#pragma pack(push,1)
	struct sexstring_header
	{
		int32 Length;
		char Buffer[1];
	};

	namespace Compiler
	{
		struct VirtualTable;
	}

	struct IString
	{
		Compiler::VirtualTable* vTable;
		int32 length;
		cstr buffer;
	};

#pragma pack(pop)

	typedef sexstring_header* sexstring;

	bool TryParseSexHex(char& finalChar, cstr s);
	bool ParseEscapeCharacter(char& finalChar, char c);

	sexstring CreateSexString(cstr src, int32 length = -1);
	void FreeSexString(sexstring s);

	ROCOCO_INTERFACE ILog
	{
		virtual void Write(cstr text) = 0;
		virtual void OnUnhandledException(int errorCode, cstr exceptionType, cstr message, void* exceptionInstance) = 0; // thrown by uncaught sexy exceptions
		virtual void OnJITCompileException(Sex::ParseException& ex) = 0; // thrown by compile errors during JIT execution
	};

	void LogError(ILog& log, cstr format, ...);

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
		char* rcharPtrValue;
		void* vPtrValue;
		float floatValue;
		double doubleValue;
		Vec4* vec4PtrValue;
		ID_API_CALLBACK apiValue;
		ID_BYTECODE byteCodeIdValue;
		const ID_BYTECODE* vTable;
	};

	int StringPrint(TokenBuffer& token, const char* format, ...);

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
# ifdef _WIN32
# error "32-bit Windows is no longer supported"
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

	enum EXECUTERESULT : int32
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

	class NamespaceSplitter
	{
	private:
		cstr src;
		int length;
		char dottedName[NAMESPACE_MAX_LENGTH];

	public:
		NamespaceSplitter(cstr _src);
		bool SplitTail(cstr& _body, cstr& _tail);
		bool SplitHead(cstr& _head, cstr& _body);
		inline char* Raw() { return dottedName; }
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

		ROCOCO_INTERFACE ISExpression
		{
			virtual const Vec2i Start() const = 0; // (X.Y) of start relative to tree origin in source file
			virtual const Vec2i End() const = 0;// (X.Y) of end relative to tree origin in source file
			virtual EXPRESSION_TYPE Type() const = 0;
			virtual const sexstring String() const = 0;
			virtual const ISParserTree& Tree() const = 0;
			virtual int NumberOfElements() const = 0;
			virtual const ISExpression& GetElement(int index) const = 0;
			virtual int GetIndexOf(const ISExpression& s) const = 0;
			const ISExpression& operator[](int index) const { return GetElement(index); }
			virtual const ISExpression* Parent() const = 0;
			virtual const ISExpression* GetOriginal() const = 0;
			virtual bool operator == (const char* token) const = 0;
			virtual void Free() = 0;
		};

		typedef const ISExpression& cr_sex;

		inline bool operator != (const ISExpression& s, const char* token)
		{
			return !(s == token);
		}

		ROCOCO_INTERFACE ISExpressionBuilder : public ISExpression
		{			
			virtual void AddAtomic(cstr text) = 0;
			virtual ISExpressionBuilder* AddChild() = 0;
			virtual void AddRef(cr_sex s) = 0;
			virtual void AddStringLiteral(cstr text) = 0;

			// Shifts all elements, i = 1 and onwards from s[index + i] to s[index + i + 1]. Then inserts a new compound element at s[index + 1]
			virtual ISExpressionBuilder* InsertChildAfter(int index) = 0;
		};

		ROCOCO_INTERFACE IExpressionTransform
		{
			virtual ISExpressionBuilder& Root() = 0;
			virtual void Free() = 0;
		};

		ROCOCO_INTERFACE IRefCounted
		{
			virtual refcount_t AddRef() = 0; // Increments the reference count and returns the new value
			virtual refcount_t Release() = 0; // Decrements the reference count and returns the new value. If it decrements to zero the instance is released
		};

		ROCOCO_INTERFACE ISParserTree : public IRefCounted
		{
			virtual ISExpression& Root() = 0; // Refers to the root expression in the tree
			virtual const ISExpression& Root() const = 0; // // Constant reference to the root expression in the tree
			virtual ISParser& Parser() = 0; // Refers to the object that created this instance
			virtual const ISourceCode& Source() const = 0; // The source code associated with this parser tree
		};

		ROCOCO_INTERFACE ISParser : public IRefCounted
		{
			virtual ISParserTree* CreateTree(ISourceCode& sourceCode) = 0; // Creates a new s-parser tree with a reference count of 1, and attaches a reference to ISourceCode 
			virtual ISourceCode* DuplicateSourceBuffer(cstr buffer, int segmentLength, const Vec2i& origin, const char* name) = 0; // Duplicates a source segment and exposes as an instance
			virtual ISourceCode* ProxySourceBuffer(cstr bufferRef, int segmentLength, const Vec2i& origin, cstr nameRef) = 0; // Duplicates the pointers defining the source code and its name and exposes as an instance
			virtual ISourceCode* LoadSource(const wchar_t* filename, const Vec2i& origin) = 0; // Loads source code, converts it to chars and returns a reference to it
			virtual ISourceCode* LoadSource(const wchar_t* filename, const Vec2i& origin, const char* buffer, long len) = 0;
		};

		ROCOCO_INTERFACE ISourceCode : public IRefCounted
		{
			virtual const Vec2i& Origin() const = 0; // The XY in the source document where the code segment begins
			virtual cstr SourceStart() const = 0; // The first char in the source code segment
			virtual const int SourceLength() const = 0;  // The number of charS in the source code segment
			virtual cstr Name() const = 0; // The name of the source segment
		};

		cstr ReadUntil(const Vec2i& pos, const ISourceCode& src);
		void GetSpecimen(char specimen[64], const ISExpression& e);
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
		VARTYPE_AnyNumeric, // Not really a type, passed to a function to indicate any numeric type is valid
		VARTYPE_Array,
		VARTYPE_List,
		VARTYPE_Map,
		VARTYPE_ListNode,
		VARTYPE_MapNode,
		VARTYPE_Lock
	};

	namespace Sex
	{
		cstr ToString(EXPRESSION_TYPE type);
		void AssertCompound(cr_sex e);
		void AssertAtomic(cr_sex e);
		void AssertStringLiteral(cr_sex e);
		void AssertNotTooManyElements(cr_sex e, int32 maxElements);
		void AssertNotTooFewElements(cr_sex e, int32 minElements);
		cr_sex GetAtomicArg(cr_sex e, int argIndex);
		[[noreturn]] void Throw(cr_sex e, _Printf_format_string_ cstr format, ...);
	}
}// Sexy

#endif