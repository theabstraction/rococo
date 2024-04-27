#include <rococo.os.win32.h>
#include <rococo.window.h>
#include <rococo.cfgs.h>
#include <CommCtrl.h>
#include "cfgs.sexy.ide.h"
#include <rococo.sexystudio.api.h>
#include "..\sexystudio\sexystudio.api.h"
#include <rococo.functional.h>
#include <rococo.strings.h>
#include <stdio.h>
#include <rococo.abstract.editor.h>
#include <vector>

using namespace Rococo;
using namespace Rococo::CFGS;
using namespace Rococo::Windows;
using namespace Rococo::SexyStudio;
using namespace Rococo::Strings;
using namespace Rococo::Abedit;
using namespace Rococo::Visitors;

namespace ANON
{
	struct Popup : ICFGSDesignerSpacePopupSupervisor, StandardWindowHandler, Visitors::ITreeControlHandler
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
			OPTION_METHOD method{ nullptr };
			TREE_NODE_ID nodeId{ 0 };
		};

		IAbstractEditor& editor;
		ICFGSDatabase& cfgs;
		SexyStudio::ISexyDatabase& db;
		ICFGSCosmetics& cosmetics;
		INamespaceValidator& nv;
		Vec2i referencePosition{ 0,0 };
		Editors::DesignerVec2 designPosition{ 0,0 };

		AutoFree<IParentWindowSupervisor> window;
		AutoFree<ITreeControlSupervisor> treeControl;

		HWND hListTitle{ 0 };

		std::vector<NodeOption> functions;

		enum { Width = 640, Height = 480 };

		Popup(IAbstractEditor& _editor, ICFGSDatabase& _cfgs, SexyStudio::ISexyDatabase& _db, INamespaceValidator& _nv, ICFGSCosmetics& _cosmetics) : editor(_editor), cfgs(_cfgs), db(_db), nv(_nv), cosmetics(_cosmetics)
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

		ISXYInterface* FindFirstInterfaceIf(ISxyNamespace& ns, Rococo::Function<bool(ISxyNamespace& ns, ISXYInterface& x)> predicate)
		{
			for (int i = 0; i < ns.InterfaceCount(); i++)
			{
				auto& x = ns.GetInterface(i);
				if (predicate(ns, x))
				{
					return &x;
				}
			}

			for (int i = 0; i < ns.SubspaceCount(); i++)
			{
				ISxyNamespace& subspace = ns[i];
				auto* x = FindFirstInterfaceIf(subspace, predicate);
				if (x)
				{
					return x;
				}
			}

			return nullptr;
		}

		void AddNewNodeForFunction(const NodeOptionHeader& header)
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
				[&fName, ptr](ISxyNamespace& ns, ISXYPublicFunction& f) -> bool
				{
					char fqName[256];
					StackStringBuilder sb(fqName, sizeof fqName);
					ns.AppendFullNameToStringBuilder(sb);
					sb << ".";
					sb << f.PublicName();
					
					return Eq(fqName, fName);
				}
			);

			if (sexyFunction)
			{
				auto* graph = cfgs.CurrentFunction();
				if (!graph)
				{
					return;
				}

				auto& node = graph->Nodes().Builder().AddNode(header.visibleName, designPosition, NodeId{ Rococo::Ids::MakeNewUniqueId() });
				auto& flowIn = node.AddSocket("Flow", SocketClass::Trigger, "Start", SocketId());
				flowIn.SetColours(RGBAb(0, 224, 0, 255), RGBAb(0, 255, 0, 255));
				auto& flowOut = node.AddSocket("Flow", SocketClass::Exit, "End", SocketId());
				flowOut.SetColours(RGBAb(0, 224, 0, 255), RGBAb(0, 255, 0, 255));

				for (int i = 0; i < sexyFunction->LocalFunction()->InputCount(); i++)
				{
					cstr name = sexyFunction->LocalFunction()->InputName(i);
					cstr type = sexyFunction->LocalFunction()->InputType(i);

					Colours colours = cosmetics.GetColoursForType(type);
					auto& socket = node.AddSocket(type, SocketClass::InputVar, name, SocketId());
					socket.SetColours(colours.normal, colours.hilight);
				}

				for (int i = 0; i < sexyFunction->LocalFunction()->OutputCount(); i++)
				{
					cstr name = sexyFunction->LocalFunction()->OutputName(i);
					cstr type = sexyFunction->LocalFunction()->OutputType(i);

					Colours colours = cosmetics.GetColoursForType(type);
					auto& socket = node.AddSocket(type, SocketClass::OutputValue, name, SocketId());
					socket.SetColours(colours.normal, colours.hilight);
				}


				ShowWindow(*window, SW_HIDE);
				editor.RefreshSlate();
			}
		}

		void AddNewNodeForMethod(const NodeOptionHeader& header)
		{
			Substring url = Substring::ToSubstring(header.url);
			cstr at = Strings::ReverseFind('@', url);
			cstr underscore = Strings::ReverseFind('_', Substring{ url.start, at });
			Substring fqName = Substring{ url.start, underscore };

			size_t ptr;
			if (1 != sscanf_s(at + 1, "%llX", &ptr))
			{
				Throw(0, "%s: Bad option: %s", __FUNCTION__, header.url.c_str());
			}

			ISXYInterface* sexyInterface = FindFirstInterfaceIf(db.GetRootNamespace(),
				[&fqName, ptr](ISxyNamespace& ns, ISXYInterface& f) -> bool
				{
					char iName[256];
					StackStringBuilder sb(iName, sizeof iName);
					ns.AppendFullNameToStringBuilder(sb);
					sb << ".";
					sb << f.PublicName();

					return Eq(fqName, iName);
				}
			);

			if (sexyInterface)
			{
				auto* graph = cfgs.CurrentFunction();
				if (!graph)
				{
					return;
				}

				Substring methodName{ underscore + 1, at };

				auto& node = graph->Nodes().Builder().AddNode(header.visibleName, designPosition, NodeId{ Rococo::Ids::MakeNewUniqueId() });
				auto& flowIn = node.AddSocket("Flow", SocketClass::Trigger, "Start", SocketId());
				flowIn.SetColours(RGBAb(0, 224, 0, 255), RGBAb(0, 255, 0, 255));
				auto& flowOut = node.AddSocket("Flow", SocketClass::Exit, "End", SocketId());
				flowOut.SetColours(RGBAb(0, 224, 0, 255), RGBAb(0, 255, 0, 255));

				ISXYFunction* pMethod = nullptr;
				for (int j = 0; j < sexyInterface->MethodCount(); j++)
				{
					auto& sMethod = sexyInterface->GetMethod(j);
					if (Eq(sMethod.PublicName(), methodName))
					{
						pMethod = &sMethod;
					}
				}

				if (pMethod)
				{
					char fqNameBuffer[256];
					fqName.CopyWithTruncate(fqNameBuffer, sizeof fqNameBuffer);

					Colours thisColours = cosmetics.GetColoursForType(fqNameBuffer);
					auto& thisSocket = node.AddSocket(fqNameBuffer, SocketClass::InputVar, "*this", SocketId());
					thisSocket.SetColours(thisColours.normal, thisColours.hilight);

					for (int i = 0; i < pMethod->InputCount(); i++)
					{
						cstr name = pMethod->InputName(i);
						cstr type = pMethod->InputType(i);

						Colours colours = cosmetics.GetColoursForType(type);
						auto& socket = node.AddSocket(type, SocketClass::InputVar, name, SocketId());
						socket.SetColours(colours.normal, colours.hilight);
					}

					for (int i = 0; i < pMethod->OutputCount(); i++)
					{
						cstr name = pMethod->OutputName(i);
						cstr type = pMethod->OutputType(i);

						Colours colours = cosmetics.GetColoursForType(type);
						auto& socket = node.AddSocket(type, SocketClass::OutputValue, name, SocketId());
						socket.SetColours(colours.normal, colours.hilight);
					}
				}


				ShowWindow(*window, SW_HIDE);
				editor.RefreshSlate();
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

				if (!nv.IsLegalNamespace(visibleName))
				{
					continue;
				}

				visibleNameBuilder << "." << f.PublicName();

				opt.header.visibleName = visibleName;

				char url[256];
				SafeFormat(url, "%s@%llX", visibleName, (size_t) f.PublicName());

				opt.header.url = url;
				opt.method = &Popup::AddNewNodeForFunction;

				functions.push_back(opt);
			}

			for (int i = 0; i < ns.SubspaceCount(); i++)
			{
				AppendAllFunctions(ns[i]);
			}
		}

		void AppendAllMethods(ISxyNamespace& ns)
		{
			if (Eq(ns.Name(), "Native"))
			{
				return;
			}

			if (Eq(ns.Name(), "EntryPoint"))
			{
				return;
			}

			for (int i = 0; i < ns.InterfaceCount(); i++)
			{
				auto& f = ns.GetInterface(i);

				if (f.PublicName()[0] == '_')
				{
					// Skip C++ only functions
					continue;
				}

				char fqInterfaceName[256];
				StackStringBuilder fqNameBuilder(fqInterfaceName, sizeof fqInterfaceName);

				ns.AppendFullNameToStringBuilder(fqNameBuilder);

				if (!nv.IsLegalNamespace(fqInterfaceName))
				{
					continue;
				}

				fqNameBuilder << "." << f.PublicName();

				for (int j = 0; j < f.MethodCount(); j++)
				{
					auto& m = f.GetMethod(j);

					char methodName[256];
					SecureFormat(methodName, "%s_%s", fqInterfaceName, m.PublicName());

					NodeOption opt;
					opt.header.visibleName = methodName;

					char url[256];
					SafeFormat(url, "%s@%llX", methodName, (size_t)f.PublicName());

					opt.header.url = url;
					opt.method = &Popup::AddNewNodeForMethod;

					functions.push_back(opt);
				}
			}

			for (int i = 0; i < ns.SubspaceCount(); i++)
			{
				AppendAllMethods(ns[i]);
			}
		}

		void PopulateOptionsBackingList()
		{
			if (!functions.empty())
			{
				return;
			}

			AppendAllFunctions(db.GetRootNamespace());
			AppendAllMethods(db.GetRootNamespace());
		}

		TREE_NODE_ID AddFunctionNameToTree(cstr functionName, TREE_NODE_ID branch)
		{
			cstr dot = FindChar(functionName, '.');
			if (!dot)
			{
				cstr underscore = FindChar(functionName, '_');
				if (underscore)
				{
					// Indicates a method
					char shortInterfaceName[256];
					strncpy_s(shortInterfaceName, functionName, underscore - functionName);

					TREE_NODE_ID interfaceNameId = treeControl->Tree().FindFirstChild(branch, shortInterfaceName);
					if (!interfaceNameId)
					{
						interfaceNameId = treeControl->Tree().AddChild(branch, shortInterfaceName, CheckState_NoCheckBox);
					}

					auto methodId = treeControl->Tree().AddChild(interfaceNameId, underscore + 1, CheckState_NoCheckBox);
					return methodId;
				}
				else
				{
					auto functionId = treeControl->Tree().AddChild(branch, functionName, CheckState_NoCheckBox);
					return functionId;
				}
			}
			else
			{
				char subspace[256];
				strncpy_s(subspace, functionName, dot - functionName);

				auto subspaceId = treeControl->Tree().FindFirstChild(branch, subspace);
				if (!subspaceId)
				{
					subspaceId = treeControl->Tree().AddChild(branch, subspace, CheckState_NoCheckBox);
				}

				return AddFunctionNameToTree(dot + 1, subspaceId);
			}
		}
			

		void ShowAt(Vec2i desktopPosition, Rococo::Editors::DesignerVec2 designPosition) override
		{
			this->designPosition = designPosition;

			referencePosition = Vec2i {desktopPosition.x + 16, desktopPosition.y };
			Layout();

			SetCursor(LoadCursorA(NULL, IDC_WAIT));

			treeControl->Tree().ResetContent();

			PopulateOptionsBackingList();

			auto functionId = treeControl->Tree().AddChild(TREE_NODE_ID::Root(), "Functions", CheckState_NoCheckBox);
			auto methodId = treeControl->Tree().AddChild(TREE_NODE_ID::Root(), "Methods", CheckState_NoCheckBox);

			for (auto& f : functions)
			{
				if (f.method == &Popup::AddNewNodeForFunction)
				{
					TREE_NODE_ID optionId = AddFunctionNameToTree(f.header.visibleName, functionId);
					f.nodeId = optionId;
				}
				else if (f.method == &Popup::AddNewNodeForMethod)
				{
					TREE_NODE_ID optionId = AddFunctionNameToTree(f.header.visibleName, methodId);
					f.nodeId = optionId;
				}
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
			SetPopupWindowConfig(config, GuiRect{ 0, 0, 0, 0 }, editor.ContainerWindow(), "ContextPopup", style, 0);
			window = Windows::CreateChildWindow(config, this);

			hListTitle = CreateWindowExA(0, WC_STATICA, "Add node...", SS_CENTER | WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, *window, NULL, NULL, NULL);

			GuiRect nullRect{ 0,0,0,0 };
			treeControl = AddTree(*window, nullRect, "", 0, *this, WS_CHILD | WS_VISIBLE | TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT | WS_BORDER | TVS_SHOWSELALWAYS);

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

			enum { TITLE_HEIGHT = 24 };

			MoveWindow(hListTitle, 0, 0, Width, TITLE_HEIGHT, FALSE);
			MoveWindow(*treeControl, 0, TITLE_HEIGHT, Width, Height - TITLE_HEIGHT, FALSE);
		}

		LRESULT OnMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) override
		{
			return StandardWindowHandler::OnMessage(hWnd, msg, wParam, lParam);
		}

		void OnItemSelected(TREE_NODE_ID id, IUITree& origin) override
		{
			UNUSED(origin);

			if (!id)
			{
				return;
			}

			for (auto& f : functions)
			{
				if (f.nodeId == id)
				{
					(this->*f.method)(f.header);
					break;
				}
			}
		}

		void OnItemRightClicked(TREE_NODE_ID id, IUITree& origin) override
		{
			UNUSED(id);
			UNUSED(origin);
		}

		void OnSize(HWND, const Vec2i&, RESIZE_TYPE) override
		{
			Layout();
		}
	};
}

namespace Rococo::CFGS
{
	ICFGSDesignerSpacePopupSupervisor* CreateWin32ContextPopup(IAbstractEditor& editor, ICFGSDatabase& cfgs, SexyStudio::ISexyDatabase& db, INamespaceValidator& namespaceValidator, ICFGSCosmetics& cosmetics)
	{
		auto* popup = new ANON::Popup(editor, cfgs, db, namespaceValidator, cosmetics);
		popup->Create();
		return popup;
	}
}
