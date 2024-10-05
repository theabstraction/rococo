#define ROCOCO_API __declspec(dllexport)
#include <rococo.types.h>
#include <rococo.json.h>
#include <vector>
#include <rococo.strings.h>

using namespace Rococo;
using namespace Rococo::Strings;
using namespace Rococo::JSon;

struct NameValuePair
{
	HString key;
	HString value;
};

ROCOCO_INTERFACE INameValueBranchBuilder
{
	virtual void AddKeyValue(cstr key, cstr value) = 0;
};

struct NameValueBranch : INameValueBranch, INameValueBranchBuilder
{
	INameValueBranch* parent;
	std::vector<NameValueBranch*> children;
	std::vector<NameValuePair> attributes;

	NameValueBranch(NameValueBranch* _parent): parent(_parent)
	{

	}
	
	int32 AttributeCount() const override
	{
		return (int32) attributes.size();
	}

	void GetAttribute(int32 index, cstr& a, cstr& b) const override
	{
		if (index < 0 || index >= (int32)attributes.size())
		{
			Throw(0, "%s: bad index %d vs vector length %llu", __FUNCTION__, index, children.size());
		}

		auto& attr = attributes[index];
		a = attr.key;
		b = attr.value;
	}

	INameValueBranch* Parent() override
	{
		return parent;
	}

	int32 ChildCount() const override
	{
		return (int32)children.size();
	}

	INameValueBranch& Child(int32 index) override
	{
		if (index < 0 || index >= (int32)children.size())
		{
			Throw(0, "%s: bad index %d vs vector length %llu", __FUNCTION__, index, children.size());
		}

		return *children[index];
	}

	cstr At(cstr name) const
	{
		for (auto& a : attributes)
		{
			if (a.key == name)
			{
				return a.value;
			}
		}

		return nullptr;
	}

	int32 TryGet(cstr name, OUT bool& wasFound, int32 default) const override
	{
		cstr value = At(name);
		if (value)
		{
			int32 iValue;
			if (1 == sscanf_s(value, "%d", &iValue))
			{
				wasFound = true;
				return iValue;
			}
		}

		wasFound = false;
		return default;
	}

	float32 TryGet(cstr name, OUT bool& wasFound, float32 default) const override
	{
		cstr value = At(name);
		if (value)
		{
			_CRT_FLOAT f;
			int result = _atoflt(&f, value);
			if (result == 0)
			{
				wasFound = true;
				return f.f;
			}
		}

		wasFound = false;
		return default;
	}

	int64 TryGet(cstr name, OUT bool& wasFound, int64 default) const override
	{
		cstr value = At(name);
		if (value)
		{
			int64 iValue;
			if (1 == sscanf_s(value, "%lld", &iValue))
			{
				wasFound = true;
				return iValue;
			}
		}

		wasFound = false;
		return default;
	}

	float64 TryGet(cstr name, OUT bool& wasFound, float64 default) const override
	{
		cstr value = At(name);
		if (value)
		{
			_CRT_DOUBLE d;
			int result = _atodbl(&d, const_cast<char*>(value)); // Blame microsoft, as the argument is char*, rather than const char*
			if (result == 0)
			{
				wasFound = true;
				return d.x;
			}
		}

		wasFound = false;
		return default;
	}

	bool TryGet(cstr name, OUT bool& wasFound, bool default) const override
	{
		cstr value = At(name);
		if (value && *value)
		{
			if (value[1] == 0)
			{
				switch (*value)
				{
				case 'T':
				case '1':
					wasFound = true;
					return true;
				case 'F':
				case '0':
					wasFound = true;
					return false;
				}
			}

			if (EqI(value, "true"))
			{
				wasFound = true;
				return true;
			}
			else if (EqI(value, "yes"))
			{
				wasFound = true;
				return true;
			}
			else if (EqI(value, "on"))
			{
				wasFound = true;
				return true;
			}
			else if (EqI(value, "false"))
			{
				wasFound = true;
				return false;
			}
			else if (EqI(value, "off"))
			{
				wasFound = true;
				return false;
			}
			else if (EqI(value, "no"))
			{
				wasFound = true;
				return false;
			}
		}

		wasFound = false;
		return default;
	}

	void AddKeyValue(cstr key, const char* value) override
	{
		NameValuePair newPair{ key, value };
		attributes.push_back(newPair);
	}
};

struct NameValueTree : INameValueTreeSupervisor
{
	NameValueBranch* root = nullptr;

	void Free() override
	{
		delete this;
	}

	NameValueTree()
	{
		root = new NameValueBranch(nullptr);
	}

	INameValueBranch& Root() override
	{
		return *root;
	}

	INameValueBranchBuilder& Builder()
	{
		return *root;
	}
};

enum class JSonParseState
{
	EXPECTING_BEGIN_OBJECT,
	EXPECTING_NAME,
	EXPECTING_COLON,
	BUILDING_NAME,
	EXPECTING_VALUE,
	BUILDING_VALUE_STRING,
	EXPECTING_COMMA,
	COMPLETED
};

enum class JSonError
{
	NONE,
	BAD_CHAR,
	BAD_CHAR_EXPECTING_COMMA,
	BAD_CHAR_STRING,
	BAD_HEX_CHAR,
	BAD_CHAR_EXPECTING_COLON,
	UNEXPECTED_LEAVE_OBJECT,
	OBJECT_DEPTH_TOO_DEEP,
	UNSUPPORTED_CODEPOINT,
	BAD_CHAR_EXPECTED_VALUE,
	NUMBER_TOO_LONG
};

struct JSonParser: IJSonParserSupervisor
{
	int lineNumber = 0;

	std::vector<char> nameBuilder;
	std::vector<char> valueBuilder;

	JSonError lastError = JSonError::NONE;

	int depth = 0;

	enum { MAX_DEPTH = 256 };

	bool EnterObject()
	{
		if (depth == MAX_DEPTH)
		{
			lastError = JSonError::OBJECT_DEPTH_TOO_DEEP;
			return false;
		}

		depth++;
		return true;
	}

	bool LeaveObject()
	{
		if (depth == 0)
		{
			lastError = JSonError::UNEXPECTED_LEAVE_OBJECT;
			return false;
		}
		depth--;	
		return true;
	}

	void Free() override
	{
		delete this;
	}

	bool AddValueCharacterViaHex(const char* hex)
	{
		uint32 value = 0;
		for (int i = 0; i < 4; i++)
		{
			uint32 nibble;

			char c = hex[i];
			if (c >= '0' && c <= '9')
			{
				nibble = c - '0';
			}
			else if (c >= 'a' && c <= 'f')
			{
				nibble = c - 'A' + 10;
			}
			else if (c >= 'A' && c <= 'F')
			{
				nibble = c - 'A' + 10;
			}
			else
			{
				lastError = JSonError::BAD_HEX_CHAR;
				return false;
			}

			value += (nibble << (i * 4));
		}

		if (value > 127)
		{
			// Currently Rococo only supports ascii encoding in JSon packets
			lastError = JSonError::UNSUPPORTED_CODEPOINT;
			return false;
		}

		valueBuilder.push_back((char)value);
		return true;
	}

	// Assumes numberString points to a numeric sequence with option - and exponent characters. Returns the first character beyond the last digit or null if parse fails. Blankspace terminates the string gracefully. 0
	// Assumes the first digit has been validated
	const char* AddNumberValue(const char* numberString, INameValueBranchBuilder& builder)
	{
		char numberBuffer[64];

		char* dest = numberBuffer;

		cstr p = numberString;

		// First character has already been validated, so copy it
		*dest++ = *p++;

		for (size_t i = 0; i < sizeof numberBuffer - 2; i++)
		{
			char c = *p++;
			if (c == 0)
			{
				return nullptr;
			}

			if (dest )

			switch (c)
			{
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
			case '.':
			case 'e':
			case 'E':
			case '-':
				break;
			case '\r':
			case '\n':
			case '\t':
			case ' ':
				*dest++ = 0;
				builder.AddKeyValue(nameBuilder.data(), numberBuffer);
				nameBuilder.clear();
				return p;
			case ',':
				*dest++ = 0;
				builder.AddKeyValue(nameBuilder.data(), numberBuffer);
				nameBuilder.clear();
				return --p;
			case '}':
				*dest++ = 0;
				builder.AddKeyValue(nameBuilder.data(), numberBuffer);
				nameBuilder.clear();
				if (!LeaveObject())
				{
					return nullptr;
				}
				return p;				
			default:
				return nullptr;
			}

			*dest++ = c;
		}

		return nullptr;
	}

	void RunStateMachine(cstr text, INameValueBranchBuilder& builder)
	{
		JSonParseState state = JSonParseState::EXPECTING_BEGIN_OBJECT;
		lastError = JSonError::NONE;

		UNUSED(builder);
		UNUSED(state);

		nameBuilder.clear();
		valueBuilder.clear();
		lineNumber = 1;

		cstr p = text;

		for (;;)
		{
		nextChar:

			switch (state)
			{
			case JSonParseState::EXPECTING_BEGIN_OBJECT:
				for (;;)
				{
					char c = *p;
					if (c == '{')
					{
						if (!EnterObject())
						{
							return;
						}
						state = JSonParseState::EXPECTING_NAME;
						p++;
						goto nextChar;
					}
					else if (c == 0)
					{
						return;
					}
					else if (c == '\n')
					{
						lineNumber++;
						p++;
					}
					else if (c <= 32)
					{
						// Blank space
						p++;
					}
				}
				break;
			case JSonParseState::EXPECTING_NAME:
				for (;;)
				{
					char c = *p;
					if (c == 0)
					{
						return;
					}
					else if (c == '\n')
					{
						lineNumber++;
						p++;
					}
					else if (c <= 32)
					{
						// Blank space
						p++;
					}
					else if (c == '}')
					{
						p++;

						if (!LeaveObject())
						{
							return;
						}
					}
					else
					{
						state = JSonParseState::BUILDING_NAME;
						nameBuilder.push_back(c);
						p++;
						goto nextChar;
					}
				}
				break;
			case JSonParseState::BUILDING_NAME:
				for (;;)
				{
					char c = *p;
					if (c == 0)
					{
						return;
					}
					else if (c == '\n')
					{
						state = JSonParseState::EXPECTING_COLON;
						lineNumber++;
						p++;
						nameBuilder.push_back(0);
						goto nextChar;
					}
					else if (c <= 32)
					{
						// Blank space
						state = JSonParseState::EXPECTING_COLON;
						p++;
						nameBuilder.push_back(0);
						goto nextChar;
					}
					else
					{
						nameBuilder.push_back(c);
						p++;
					}
				}
				break;
			case JSonParseState::EXPECTING_COLON:
				for (;;)
				{
					char c = *p;
					if (c == 0)
					{
						return;
					}
					else if (c == '\n')
					{
						lineNumber++;
						p++;
					}
					else if (c <= 32)
					{
						// Blank space
						p++;
					}
					else if (c == ':')
					{
						state = JSonParseState::EXPECTING_VALUE;
						p++;
						goto nextChar;
					}
					else
					{
						lastError = JSonError::BAD_CHAR_EXPECTING_COLON;
						return;
					}
				}
				break;
			case JSonParseState::EXPECTING_VALUE:
				for (;;)
				{
					// One of object, array, string, number, 'true', 'false' or 'null'

					char c = *p;
					if (c == 0)
					{
						return;
					}
					else if (c == '\n')
					{
						lineNumber++;
						p++;
					}
					else if (c <= 32)
					{
						// Blank space
						p++;
					}
					else if (c == '"')
					{
						// string
						state = JSonParseState::BUILDING_VALUE_STRING;
						p++;
						goto nextChar;
					}
					else if (c == '-' || (c >= '0' && c <= '9'))
					{
						p = AddNumberValue(p, builder);
						if (!p)
						{
							return;
						}
						state = JSonParseState::EXPECTING_COMMA;
						goto nextChar;
					}
					else
					{
						if (Strings::Compare(p, "true", 5) == 0)
						{
							builder.AddKeyValue(nameBuilder.data(), "true");
							nameBuilder.clear();
							state = JSonParseState::EXPECTING_COMMA;
							p += 5;
							goto nextChar;
						}

						if (Strings::Compare(p, "false", 5) == 0)
						{
							builder.AddKeyValue(nameBuilder.data(), "false");
							nameBuilder.clear();
							state = JSonParseState::EXPECTING_COMMA;
							p += 5;
							goto nextChar;
						}

						if (Strings::Compare(p, "null", 5) == 0)
						{
							builder.AddKeyValue(nameBuilder.data(), "null");
							nameBuilder.clear();
							state = JSonParseState::EXPECTING_COMMA;
							p += 4;
							goto nextChar;
						}

						if (c == '{')
						{
							if (!EnterObject())
							{
								return;
							}
							state = JSonParseState::EXPECTING_NAME;
							goto nextChar;
						}

						lastError = JSonError::BAD_CHAR_EXPECTED_VALUE;
						return;
					}
				}
				break;
			case JSonParseState::BUILDING_VALUE_STRING:
				for (;;)
				{
					char c = *p;
					if (c == 0)
					{
						return;
					}

					if (c == '"')
					{
						valueBuilder.push_back(0);
						builder.AddKeyValue(nameBuilder.data(), valueBuilder.data());
						nameBuilder.clear();
						valueBuilder.clear();
						state = JSonParseState::EXPECTING_COMMA;
						p++;
						goto nextChar;
					}

					if (c == '\\')
					{
						// Escape character
						c = *p++;

						switch (c)
						{
						case 0:
							return;
						case '"':
							valueBuilder.push_back('"');
							break;
						case '\\':
							valueBuilder.push_back('\\');
							break;
						case '/':
							valueBuilder.push_back('/');
							break;
						case 'b':
							valueBuilder.push_back('\b');
							break;
						case 'f':
							valueBuilder.push_back('\f');
							break;
						case 'n':
							valueBuilder.push_back('\n');
							break;
						case 'r':
							valueBuilder.push_back('\r');
							break;
						case 't':
							valueBuilder.push_back('\t');
							break;
						case 'u':
							// Four hex characters, such as in \uA7C4
							{
								char hex[8];
								for (int i = 0; i < 4; i++)
								{
									c = *p++;
									hex[i] = c;
									if (c == 0)
									{
										lastError = JSonError::BAD_CHAR_STRING;
										return;
									}
								}

								if (!AddValueCharacterViaHex(hex))
								{
									return;
								}
							}
							break;
						default:
							lastError = JSonError::BAD_CHAR_STRING;
							return;
						}
					}
					else
					{
						valueBuilder.push_back(c);
						p++;
					}
				}
				break;
			case JSonParseState::EXPECTING_COMMA:
				for (;;)
				{
					char c = *p;
					if (c == 0)
					{
						return;
					}
					else if (c == '\n')
					{
						lineNumber++;
						p++;
					}
					else if (c <= 32)
					{
						// Blank space
						p++;
					}
					else if (c == '}')
					{
						if (!LeaveObject())
						{
							return;
						}

						p++;

						if (depth == 0)
						{
							state = JSonParseState::COMPLETED;
							return;
						}
					}
					else if (c == ',')
					{
						state = JSonParseState::EXPECTING_NAME;
						p++;
						goto nextChar;
					}
					else
					{
						lastError = JSonError::BAD_CHAR_EXPECTING_COMMA;
						return;
					}
				}
				break;
			}
		}
	}

	void BuildTree(cstr text, INameValueBranchBuilder& builder)
	{
		RunStateMachine(text, builder);
		switch (lastError)
		{
		case JSonError::NONE:
			return;
		case JSonError::BAD_CHAR:
			Throw(0, "Bad character in JSon at line %d", lineNumber);
		case JSonError::BAD_CHAR_STRING:
			Throw(0, "Bad character in JSon at line %d", lineNumber);
		case JSonError::BAD_CHAR_EXPECTED_VALUE:
			Throw(0, "Bad character in JSon at line %d. Expected a value (quoted string, number, true/false/null, object, array)", lineNumber);
		case JSonError::BAD_CHAR_EXPECTING_COMMA:
			Throw(0, "Bad character in JSon at line %d. Expected a comma", lineNumber);
		case JSonError::UNEXPECTED_LEAVE_OBJECT:
			Throw(0, "Unexpected leave object character '}' in JSon at line %d", lineNumber);
		case JSonError::OBJECT_DEPTH_TOO_DEEP:
			Throw(0, "Maximum object depth %d reached", MAX_DEPTH);
		case JSonError::UNSUPPORTED_CODEPOINT:
			Throw(0, "Unsupported codepoint. Only ascii characters are supported for this parser");
		case JSonError::NUMBER_TOO_LONG:
			Throw(0, "The number was defined with too many characters");
		case JSonError::BAD_HEX_CHAR:
			Throw(0, "Character was not a hex digit");
		case JSonError::BAD_CHAR_EXPECTING_COLON:
			Throw(0, "Expected a colon");
		default:
			Throw(0, "Unexpected error state %d", static_cast<uint32>(lastError));
		}
	}

	INameValueTreeSupervisor* Parse(cstr text) override
	{
		AutoFree<NameValueTree> tree = new NameValueTree();
		BuildTree(text, tree->Builder());
		return tree.Detach();
	}
};

namespace Rococo::JSon
{
	ROCOCO_API IJSonParserSupervisor* CreateJSonParser()
	{
		return new JSonParser();
	}
}