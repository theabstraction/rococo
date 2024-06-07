#define ROCOCO_API __declspec(dllexport)
#include <rococo.types.h>
#include <rococo.json.h>
#include <vector>
#include <rococo.strings.h>

using namespace Rococo;
using namespace Rococo::Strings;
using namespace Rococo::JSon;

struct NameValuePairs
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
	std::vector<NameValuePairs> attributes;

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
			int result = _atoflt(&f, name);
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
			int result = _atodbl(&d, const_cast<char*>(name)); // Blame microsoft, as the argument is char*, rather than const char*
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

	void AddKeyValue(cstr key, cstr value) override
	{
		Throw(0, "Not implemented: %s %s", key, value);
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
	BUILDING_NAME,
	EXPECTING_VALUE,
	BUILDING_VALUE
};

struct JSonParser: IJSonParserSupervisor
{
	void Free() override
	{
		delete this;
	}

	void BuildTree(cstr text, INameValueBranchBuilder& builder)
	{
		JSonParseState state = JSonParseState::EXPECTING_BEGIN_OBJECT;

		UNUSED(builder);
		UNUSED(state);

		cstr p = text;
		while (*p)
		{
			Throw(0, "Not implemented");
			p++;
		}
	}

	INameValueTreeSupervisor* Parse(cstr text) override
	{
		auto* tree = new NameValueTree();
		BuildTree(text, tree->Builder());
		return tree;
	}
};

namespace Rococo::JSon
{
	ROCOCO_API IJSonParserSupervisor* CreateJSonParser()
	{
		return new JSonParser();
	}
}