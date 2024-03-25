#include <rococo.os.win32.h>
#include <rococo.window.h>
#include <rococo.cfgs.h>
#include <vector>
#include <CommCtrl.h>
#include <rococo.sexystudio.api.h>
#include <../sexystudio/sexystudio.api.h>
#include <rococo.strings.h>
#include <rococo.functional.h>
#include <stdio.h>

using namespace Rococo;
using namespace Rococo::CFGS;
using namespace Rococo::Windows;
using namespace Rococo::SexyStudio;
using namespace Rococo::Strings;

namespace ANON
{
	struct Popup : ICFGSDesignerSpacePopupSupervisor, StandardWindowHandler, IListViewEvents
	{
		struct NodeOptionHeader
		{
			HString visibleName;
			HString url;
		};

		using OPTION_METHOD = void (Popup::*)(const NodeOptionHeader& header);

		struct NodeOption
		{
			NodeOptionHeader header;
			OPTION_METHOD method;
		};

		HWND hHostWindow;
		ICFGSDatabase& cfgs;
		SexyStudio::ISexyDatabase& db;
		Vec2i referencePosition{ 0,0 };
		Editors::DesignerVec2 designPosition;

		AutoFree<IParentWindowSupervisor> window;
		AutoFree<IListViewSupervisor> listView;

		std::vector<NodeOption> options;

		enum { Width = 640, Height = 480 };

		Popup(HWND _hHostWindow, ICFGSDatabase& _cfgs, SexyStudio::ISexyDatabase& _db) : hHostWindow(_hHostWindow), cfgs(_cfgs), db(_db)
		{

		}

		ISXYPublicFunction* FindFirstFunctionIf(ISxyNamespace& ns, Rococo::Function<bool (ISxyNamespace& ns, ISXYPublicFunction& f)> predicate)
		{
			for (int i = 0; i < ns.FunctionCount(); i++)
			{
				auto& f = ns.GetFunction(i);
				if (predicate(ns, f))
				{
					return &f;
				}
			}

			for (int i = 0; i < ns.SubspaceCount(); i++)
			{
				ISxyNamespace& subspace = ns[i];
				auto* f = FindFirstFunctionIf(subspace, predicate);
				if (f)
				{
					return f;
				}
			}

			return nullptr;
		}

		void SelectFunction(const NodeOptionHeader& header)
		{
			Substring url = Substring::ToSubstring(header.url);
			cstr at = Strings::ReverseFind('@', url);
			Substring fName = Substring{ url.start, at };
			
			size_t ptr;
			if (1 != sscanf_s(at + 1, "%llX", &ptr))
			{
				Throw(0, "%s: Bad option: %s", __FUNCTION__, header.url.c_str());
			}
	
			ISXYPublicFunction* sexyFunction = FindFirstFunctionIf(db.GetRootNamespace(),
				[&fName, ptr](ISxyNamespace&, ISXYPublicFunction& f) -> bool
				{
					if (!Eq(f.PublicName(), fName))
					{
						return false;
					}

					return true;

					// return ((size_t)&f) == ptr;
				}
			);

			if (sexyFunction)
			{
				auto* graph = cfgs.CurrentFunction();
				if (!graph)
				{
					return;
				}

				auto& node = graph->Nodes().Builder().AddNode(header.visibleName, designPosition, NodeId{ Rococo::MakeNewUniqueId() });
				node.AddSocket("Flow", SocketClass::Trigger, "Start", SocketId());
				node.AddSocket("Flow", SocketClass::Exit, "End", SocketId());

				for (int i = 0; i < sexyFunction->LocalFunction()->InputCount(); i++)
				{
					auto qualifier = sexyFunction->LocalFunction()->InputQualifier(i);

					cstr name = sexyFunction->LocalFunction()->InputName(i);
					cstr type = sexyFunction->LocalFunction()->InputType(i);

					switch (qualifier)
					{
					case EQualifier::None: // constant by default
					case EQualifier::Constant:
					case EQualifier::Ref: // refs require the variable input to be defined
						node.AddSocket(type, SocketClass::InputVar, name, SocketId());
						break;
					case EQualifier::Output:
						node.AddSocket(type, SocketClass::OutputValue, name, SocketId());
					}
				}

				for (int i = 0; i < sexyFunction->LocalFunction()->OutputCount(); i++)
				{
					cstr name = sexyFunction->LocalFunction()->OutputName(i);
					cstr type = sexyFunction->LocalFunction()->OutputType(i);

					node.AddSocket(type, SocketClass::OutputValue, name, SocketId());
				}


				ShowWindow(*window, SW_HIDE);
				InvalidateRect(hHostWindow, NULL, TRUE);
			}
		}

		void Free() override
		{
			delete this;
		}

		bool IsVisible() const override
		{
			return IsWindowVisible(*window);
		}

		void AppendAllFunctions(ISxyNamespace& ns)
		{
			if (Eq(ns.Name(), "Native"))
			{
				return;
			}

			if (Eq(ns.Name(), "EntryPoint"))
			{
				return;
			}

			for (int i = 0; i < ns.FunctionCount(); i++)
			{
				auto& f = ns.GetFunction(i);

				if (f.PublicName()[0] == '_')
				{
					// Skip C++ only functions
					continue;
				}

				NodeOption opt;

				char visibleName[256];
				StackStringBuilder visibleNameBuilder(visibleName, sizeof visibleName);

				ns.AppendFullNameToStringBuilder(visibleNameBuilder);
				visibleNameBuilder << "." << f.PublicName();

				opt.header.visibleName = visibleName;

				char url[256];
				SafeFormat(url, "%s@%llX", f.PublicName(), (size_t) f.PublicName());

				opt.header.url = url;
				opt.method = &Popup::SelectFunction;

				options.push_back(opt);
			}

			for (int i = 0; i < ns.SubspaceCount(); i++)
			{
				AppendAllFunctions(ns[i]);
			}
		}

		void PopulateOptionsBackingList()
		{
			if (!options.empty())
			{
				return;
			}

			AppendAllFunctions(db.GetRootNamespace());
		}

		void ShowAt(Vec2i desktopPosition, Rococo::Editors::DesignerVec2 designPosition) override
		{
			this->designPosition = designPosition;

			referencePosition = Vec2i {desktopPosition.x + 16, desktopPosition.y };
			Layout();

			SetCursor(LoadCursorA(NULL, IDC_WAIT));

			listView->UIList().ClearRows();

			PopulateOptionsBackingList();

			cstr row[2] = { "", nullptr };

			for (auto& opt : options)
			{
				row[0] = opt.header.visibleName;
				listView->UIList().AddRow(row);
			}

			SetCursor(LoadCursorA(NULL, IDC_ARROW));

			ShowWindow(*window, SW_SHOW);
		}

		void Hide() override
		{
			ShowWindow(*window, SW_HIDE);
		}

		void Create()
		{
			Windows::WindowConfig config;
			DWORD style = 0;
			SetPopupWindowConfig(config, GuiRect{ 0, 0, 0, 0 }, hHostWindow, "ContextPopup", style, 0);
			window = Windows::CreateChildWindow(config, this);

			GuiRect nullRect{ 0,0,0,0 };
			listView = AddListView(*window, nullRect, "", *this, LVS_REPORT | WS_VISIBLE | WS_CHILD, WS_VISIBLE | WS_CHILD, 0);

			cstr columns[] = { "Add Node...", nullptr };
			int32 widths[] = { Width, 0 };
			listView->UIList().SetColumns(columns, widths);

			Layout();
		}

		void Layout()
		{
			int x = 0;
			int y = 0;

			HWND hDesktopWnd = GetDesktopWindow();

			RECT desktopWindowRect;
			GetWindowRect(hDesktopWnd, &desktopWindowRect);

			if (referencePosition.x + Width < desktopWindowRect.right)
			{
				x = referencePosition.x;
			}
			else
			{
				x = referencePosition.x - Width;
			}

			if (referencePosition.y + Height < desktopWindowRect.bottom)
			{
				y = referencePosition.y;
			}
			else
			{
				y = referencePosition.y - Height;
			}

			MoveWindow(*window, x, y, Width, Height, FALSE);
			MoveWindow(*listView, 0, 0, Width, Height, FALSE);
		}

		LRESULT OnMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) override
		{
			return StandardWindowHandler::OnMessage(hWnd, msg, wParam, lParam);
		}

		void OnItemChanged(int index) override
		{
			if (index < 0 || index >= (int32)options.size())
			{
				return;
			}

			auto& option = options[index];
			(this->*option.method)(option.header);
		}

		void OnDrawItem(DRAWITEMSTRUCT&) override
		{

		}

		void OnMeasureItem(HWND hListView, MEASUREITEMSTRUCT&) override
		{
			UNUSED(hListView);
		}

		void OnSize(HWND, const Vec2i&, RESIZE_TYPE) override
		{
			Layout();
		}
	};
}

namespace Rococo::CFGS
{
	ICFGSDesignerSpacePopupSupervisor* CreateWin32ContextPopup(HWND hHostWindow, ICFGSDatabase& cfgs, SexyStudio::ISexyDatabase& db)
	{
		auto* popup = new ANON::Popup(hHostWindow, cfgs, db);
		popup->Create();
		return popup;
	}
}
