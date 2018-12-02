#include <rococo.types.h>
#include <rococo.os.win32.h>
#include <rococo.window.h>
#include <rococo.cute.h>

#include <rococo.strings.h>

using namespace Rococo;
using namespace Rococo::Cute;

struct Tree : public ITree
{
	IParentWindow& parent;
	HString populatorId;

	Tree(IParentWindow& _parent) : parent(_parent)
	{
	}

	void AddChild(IWindowSupervisor* child) override
	{
		Throw(0, "Tree does not support AddChild(...)");
	}

	void Close() override
	{

	}

	void Free() override
	{
		delete this;
	}

	void OnResize(Vec2i span, ResizeType to) override
	{

	}

	WindowRef Handle()
	{
		return WindowRef{ 0 };
	}

	void SetPopulator(const fstring& _populatorId) override
	{
		populatorId = _populatorId;
	}
};

namespace Rococo
{
	namespace Cute
	{
		ITree* CreateTree(IParentWindow& parent)
		{
			return new Tree(parent);
		}
	}
}