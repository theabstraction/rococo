#include <rococo.types.h>
#include <rococo.os.win32.h>
#include <rococo.window.h>
#include <rococo.cute.h>
#include "rococo.cute.post.h"

#include <rococo.strings.h>

#include <CommCtrl.h>
#include <vector>

using namespace Rococo;
using namespace Rococo::Cute;
using namespace Rococo::Post;

ROCOCOAPI ITreeNodeSupervisor: public ITreeNode
{
	virtual void Free() = 0;
};

struct TreeNode : public ITreeNodeSupervisor
{
	HWND hWnd;
	TreeNode* parent;
	HTREEITEM hNode;

	std::vector<ITreeNodeSupervisor*> nodes;

	ITreeNode* AddItem(cstr text)
	{
		auto* t = new TreeNode(*this, text);
		nodes.push_back(t);
		return t;
	}

	void Free() override
	{
		for (auto& i : nodes)
		{
			i->Free();
		}
		delete this;
	}

	TreeNode(TreeNode& _parent, cstr text) : hWnd(_parent.hWnd), parent(&_parent)
	{
		TV_INSERTSTRUCTA i = { 0 };
		i.hParent = _parent.hNode;
		i.hInsertAfter = TVI_LAST;
		i.itemex.mask = TVIF_TEXT;
		i.itemex.pszText = (char*) text;
		i.itemex.cchTextMax = (int) strlen(text);
		hNode = (HTREEITEM) SendMessage(hWnd, TVM_INSERTITEMA, 0, (LPARAM) &i);
	}

	TreeNode(HWND _hWnd): hWnd(_hWnd)
	{
		parent = nullptr;
		hNode = TVGN_ROOT;
	}
};

struct Tree : public ITree
{
	IParentWindow& parent;
	Post::IPostbox& postbox;

	HString populatorId;
	HWND hTreeContainerWnd;
	HWND hTreeWnd;

	AutoFree<IChildSupervisor> treeContainer;
	AutoFree<TreeNode> root;

	Tree(IParentWindow& _parent, Post::IPostbox& post, int32 createStyleFlags) : 
		parent(_parent), postbox(post)
	{
		auto hParent = ToHWND(_parent.Handle());
		RECT r;
		GetClientRect(hParent, &r);
		treeContainer = CreateChild(hParent, WS_CHILD | WS_VISIBLE, 0, 0, 0, r.right, r.bottom);
		auto hInstance = (HINSTANCE)GetWindowLongPtrA(hParent, GWLP_HINSTANCE);
		DWORD exStyle = 0;

		WindowRef ref = treeContainer->Handle();

		DWORD style = createStyleFlags | WS_CHILD | WS_VISIBLE;

		HWND hContainer = ToHWND(ref);
		hTreeWnd = CreateWindowExA(exStyle, WC_TREEVIEWA, "Tree", style,
			0, 0, r.right, r.bottom, hContainer, NULL, hInstance, NULL);

		root = new TreeNode(hTreeWnd);
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
		Populate();
	}

	void Populate()
	{
		PopulateTree p;
		p.Root = root;
		postbox.SendDirect(p);
	}
};

namespace Rococo
{
	namespace Cute
	{
		ITree* CreateTree(IParentWindow& parent, Post::IPostbox& post, boolean32 hasLines)
		{
			InitCommonControls();
			auto* tree = new Tree(parent, post, hasLines);
			return tree;
		}
	}
}