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
	struct Popup : ICFGSSexyPopup, StandardWindowHandler, Visitors::ITreeControlHandler
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

		std::vector<NodeOption> backingList;

		CableDropped dropInfo;

		ICFGSDesignerSpacePopupPopulator& populator;

		enum { Width = 640, Height = 480 };

		Popup(IAbstractEditor& _editor, ICFGSDatabase& _cfgs, SexyStudio::ISexyDatabase& _db, INamespaceValidator& _nv, ICFGSCosmetics& _cosmetics, ICFGSDesignerSpacePopupPopulator& _populator) : 
			editor(_editor), cfgs(_cfgs), db(_db), nv(_nv), cosmetics(_cosmetics), populator(_populator)
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

		ISXYFactory* FindFirstFactoryIf(ISxyNamespace& ns, Rococo::Function<bool(ISxyNamespace& ns, ISXYFactory& x)> predicate)
		{
			for (int i = 0; i < ns.FactoryCount(); i++)
			{
				auto& x = ns.GetFactory(i);
				if (predicate(ns, x))
				{
					return &x;
				}
			}

			for (int i = 0; i < ns.SubspaceCount(); i++)
			{
				ISxyNamespace& subspace = ns[i];
				auto* x = FindFirstFactoryIf(subspace, predicate);
				if (x)
				{
					return x;
				}
			}

			return nullptr;
		}

		void AddNewNodeForFactory(const NodeOptionHeader& header)
		{
			Substring url = Substring::ToSubstring(header.url);
			cstr at = Strings::ReverseFind('@', url);
			Substring fName = Substring{ url.start, at };

			size_t ptr;
			if (1 != sscanf_s(at + 1, "%llX", &ptr))
			{
				Throw(0, "%s: Bad option: %s", __FUNCTION__, header.url.c_str());
			}

			ISXYFactory* sexyFactory = FindFirstFactoryIf(db.GetRootNamespace(),
				[&fName, ptr](ISxyNamespace& ns, ISXYFactory& f) -> bool
				{
					char fqName[256];
					StackStringBuilder sb(fqName, sizeof fqName);
					ns.AppendFullNameToStringBuilder(sb);
					sb << ".";
					sb << f.PublicName();

					return Eq(fqName, fName);
				}
			);

			if (sexyFactory)
			{
				auto* graph = cfgs.CurrentFunction();
				if (!graph)
				{
					return;
				}

				auto& node = graph->Nodes().Builder().AddNode(header.visibleName, designPosition, NodeId());
				auto& flowIn = node.AddSocket("Flow", SocketClass::Trigger, "Start", SocketId());
				flowIn.SetColours(cosmetics.GetColoursForType("__Flow"));
				auto& flowOut = node.AddSocket("Flow", SocketClass::Exit, "End", SocketId());
				flowOut.SetColours(cosmetics.GetColoursForType("__Flow"));

				for (int i = 0; i < sexyFactory->InputCount(); i++)
				{
					cstr name = sexyFactory->InputName(i);
					cstr type = sexyFactory->InputType(i);

					Colours colours = cosmetics.GetColoursForType(type);
					auto& socket = node.AddSocket(type, SocketClass::InputVar, name, SocketId());
					socket.SetColours(colours);
				}

				char interfaceName[256];
				sexyFactory->GetDefinedInterface(interfaceName, sizeof interfaceName);

				Colours colours = cosmetics.GetColoursForType(interfaceName);
				auto& socket = node.AddSocket(interfaceName, SocketClass::OutputValue, "*this", SocketId());
				socket.SetColours(colours);

				ShowWindow(*window, SW_HIDE);
				editor.RefreshSlate();
			}
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

				auto& node = graph->Nodes().Builder().AddNode(header.visibleName, designPosition, NodeId());
				auto& flowIn = node.AddSocket("Flow", SocketClass::Trigger, "Start", SocketId());
				Colours flowColours = cosmetics.GetColoursForType("__Flow");
				flowIn.SetColours(flowColours);
				auto& flowOut = node.AddSocket("Flow", SocketClass::Exit, "End", SocketId());
				flowOut.SetColours(flowColours);

				for (int i = 0; i < sexyFunction->LocalFunction()->InputCount(); i++)
				{
					cstr name = sexyFunction->LocalFunction()->InputName(i);
					cstr type = sexyFunction->LocalFunction()->InputType(i);

					Colours colours = cosmetics.GetColoursForType(type);
					auto& socket = node.AddSocket(type, SocketClass::InputVar, name, SocketId());
					socket.SetColours(colours);
				}

				for (int i = 0; i < sexyFunction->LocalFunction()->OutputCount(); i++)
				{
					cstr name = sexyFunction->LocalFunction()->OutputName(i);
					cstr type = sexyFunction->LocalFunction()->OutputType(i);

					Colours colours = cosmetics.GetColoursForType(type);
					auto& socket = node.AddSocket(type, SocketClass::OutputValue, name, SocketId());
					socket.SetColours(colours);
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

			if (!sexyInterface)
			{
				return;
			}

			auto* graph = cfgs.CurrentFunction();
			if (!graph)
			{
				return;
			}

			Substring methodName{ underscore + 1, at };

			auto& node = graph->Nodes().Builder().AddNode(header.visibleName, designPosition, NodeId());
			auto& flowIn = node.AddSocket("Flow", SocketClass::Trigger, "Start", SocketId());
			Colours colours = cosmetics.GetColoursForType("__Flow");
			flowIn.SetColours(colours);
			auto& flowOut = node.AddSocket("Flow", SocketClass::Exit, "End", SocketId());
			flowOut.SetColours(colours);

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
				thisSocket.SetColours(thisColours);

				for (int i = 0; i < pMethod->InputCount(); i++)
				{
					cstr name = pMethod->InputName(i);
					cstr type = pMethod->InputType(i);

					Colours inputColours = cosmetics.GetColoursForType(type);
					auto& socket = node.AddSocket(type, SocketClass::InputVar, name, SocketId());
					socket.SetColours(inputColours);
				}

				for (int i = 0; i < pMethod->OutputCount(); i++)
				{
					cstr name = pMethod->OutputName(i);
					cstr type = pMethod->OutputType(i);

					Colours outputColours = cosmetics.GetColoursForType(type);
					auto& socket = node.AddSocket(type, SocketClass::OutputValue, name, SocketId());
					socket.SetColours(outputColours);
				}

				if (dropInfo.anchor.node)
				{
					graph->Cables().Add(dropInfo.anchor.node, dropInfo.anchor.socket, node.Id(), thisSocket.Id());
					graph->ConnectCablesToSockets();
				}
			}

			ShowWindow(*window, SW_HIDE);

			editor.RefreshSlate();
		}

		void AddReturnNode(const NodeOptionHeader& header)
		{
			UNUSED(header);

			auto* graph = cfgs.CurrentFunction();
			if (!graph)
			{
				return;
			}

			auto& node = graph->Nodes().Builder().AddNode("<Return>", designPosition, NodeId() );
			auto& flowIn = node.AddSocket("Flow", SocketClass::Trigger, "Terminate", SocketId());
			Colours flowColours = cosmetics.GetColoursForType("__Flow");
			flowIn.SetColours(flowColours);
			
			auto& inputSpec = graph->ReturnNode();

			for (int i = 0; i < inputSpec.SocketCount(); i++)
			{
				auto& s = inputSpec[i];

				SocketClass inputClass = SocketClass::None;

				switch (s.SocketClassification())
				{
				case SocketClass::OutputValue:
					inputClass = SocketClass::InputVar;
					break;
				case SocketClass::OutputRef:
					inputClass = SocketClass::InputRef;
					break;
				case SocketClass::ConstOutputRef:
					inputClass = SocketClass::OutputRef;
					break;
				default:
					continue;
				}

				auto& inputSocket = node.AddSocket(s.Type().Value, inputClass, s.Name(), SocketId());
				Colours inputColours = cosmetics.GetColoursForType(s.Type().Value);
				inputSocket.SetColours(inputColours);
			}

			ShowWindow(*window, SW_HIDE);

			editor.RefreshSlate();
		}

		void AddNodeFromTemplate(const NodeOptionHeader& header)
		{
			UNUSED(header);

			auto* graph = cfgs.CurrentFunction();
			if (!graph)
			{
				return;
			}

			auto* templateFunction = cfgs.FindFunction(templateId);
			if (!templateFunction)
			{
				return;
			}

			auto& node = graph->Nodes().Builder().AddNode(templateFunction->Name(), designPosition, NodeId());
			auto& flowIn = node.AddSocket("Flow", SocketClass::Trigger, "Start", SocketId());
			Colours flowColours = cosmetics.GetColoursForType("__Flow");
			flowIn.SetColours(flowColours);
			auto& flowOut = node.AddSocket("Flow", SocketClass::Exit, "End", SocketId());
			flowOut.SetColours(flowColours);

			auto& beginTemplate = templateFunction->BeginNode();

			for (int i = 0; i < beginTemplate.SocketCount(); i++)
			{
				auto& s = beginTemplate[i];
				
				auto socketClass = s.SocketClassification();
				if (IsInputClass(socketClass))
				{
					Colours colours = cosmetics.GetColoursForType(s.Type().Value);
					auto& socket = node.AddSocket(s.Type().Value, socketClass, s.Name(), SocketId());
					socket.SetColours(colours);
				}
			}

			auto& returnTemplate = templateFunction->ReturnNode();

			for (int i = 0; i < returnTemplate.SocketCount(); i++)
			{
				auto& s = returnTemplate[i];

				auto socketClass = FlipInputOutputClass(s.SocketClassification());
				if (IsOutputClass(socketClass))
				{
					Colours colours = cosmetics.GetColoursForType(s.Type().Value);
					auto& socket = node.AddSocket(s.Type().Value, socketClass, s.Name(), SocketId());
					socket.SetColours(colours);
				}
			}

			ShowWindow(*window, SW_HIDE);

			editor.RefreshSlate();
		}

		void AddGetNode(const NodeOptionHeader& header)
		{
			cstr space = FindChar(header.url, ' ');
			if (!space)
			{
				return;
			}

			Substring name{ header.url, space };
			cstr type = space + 1;

			char nameBuffer[256];
			name.CopyWithTruncate(nameBuffer, sizeof nameBuffer);

			auto* graph = cfgs.CurrentFunction();
			if (!graph)
			{
				return;
			}

			char desc[256];
			SafeFormat(desc, "<Get> %s - %s", nameBuffer, type);

			auto& node = graph->Nodes().Builder().AddNode(desc, designPosition, NodeId());
			auto colours = cosmetics.GetColoursForType("__Variables");
			node.SetColours(colours, cosmetics.GetColoursForType("__VariablesTab"));
			auto& valueSocket = node.AddSocket(type, SocketClass::ConstOutputRef, nameBuffer, SocketId());
			colours = cosmetics.GetColoursForType(type);
			valueSocket.SetColours(colours);

			ShowWindow(*window, SW_HIDE);

			editor.RefreshSlate();
		}

		void AddSetNode(const NodeOptionHeader& header)
		{
			cstr space = FindChar(header.url, ' ');
			if (!space)
			{
				return;
			}


			Substring name{ header.url, space };
			cstr type = space + 1;

			char nameBuffer[256];
			name.CopyWithTruncate(nameBuffer, sizeof nameBuffer);

			auto* graph = cfgs.CurrentFunction();
			if (!graph)
			{
				return;
			}

			char desc[256];
			SafeFormat(desc, "<Set> %s - %s", nameBuffer, type);

			auto& node = graph->Nodes().Builder().AddNode(desc, designPosition, NodeId());
			auto colours = cosmetics.GetColoursForType("__Variables");
			node.SetColours(colours, cosmetics.GetColoursForType("__VariablesTab"));

			auto& flowIn = node.AddSocket("Flow", SocketClass::Trigger, "Start", SocketId());
			colours = cosmetics.GetColoursForType("__Flow");
			flowIn.SetColours(colours);
			auto& flowOut = node.AddSocket("Flow", SocketClass::Exit, "End", SocketId());
			flowOut.SetColours(colours);

			auto& assignSocket = node.AddSocket(type, SocketClass::ConstInputRef, nameBuffer, SocketId());
			colours = cosmetics.GetColoursForType(type);
			assignSocket.SetColours(colours);

			ShowWindow(*window, SW_HIDE);

			editor.RefreshSlate();
		}

		void Free() override
		{
			delete this;
		}

		bool IsVisible() const override
		{
			return IsWindowVisible(*window);
		}

		void AppendAllFactoriesToBackingList(ISxyNamespace& ns)
		{
			if (Eq(ns.Name(), "Native"))
			{
				return;
			}

			if (Eq(ns.Name(), "EntryPoint"))
			{
				return;
			}

			for (int i = 0; i < ns.FactoryCount(); i++)
			{
				auto& f = ns.GetFactory(i);

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
				SafeFormat(url, "%s@%llX", visibleName, (size_t)f.PublicName());

				opt.header.url = url;
				opt.method = &Popup::AddNewNodeForFactory;

				backingList.push_back(opt);
			}

			for (int i = 0; i < ns.SubspaceCount(); i++)
			{
				AppendAllFactoriesToBackingList(ns[i]);
			}
		}

		void AppendAllFunctionsToBackingList(ISxyNamespace& ns)
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

				backingList.push_back(opt);
			}

			for (int i = 0; i < ns.SubspaceCount(); i++)
			{
				AppendAllFunctionsToBackingList(ns[i]);
			}
		}

		void AppendAllMethodsToBackingList(ISxyNamespace& ns)
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

					backingList.push_back(opt);
				}
			}

			for (int i = 0; i < ns.SubspaceCount(); i++)
			{
				AppendAllMethodsToBackingList(ns[i]);
			}
		}

		void PopulateOptionsBackingList()
		{
			if (!backingList.empty())
			{
				return;
			}

			AppendAllFunctionsToBackingList(db.GetRootNamespace());
			AppendAllMethodsToBackingList(db.GetRootNamespace());
			AppendAllFactoriesToBackingList(db.GetRootNamespace());
		}

		void AddMethodsForInterface(const ISXYInterface& refInterface, const ISxyNamespace& ns, int depth = 0)
		{
			char fqInterfaceName[256];
			StackStringBuilder fqNameBuilder(fqInterfaceName, sizeof fqInterfaceName);

			ns.AppendFullNameToStringBuilder(fqNameBuilder);

			fqNameBuilder << "." << refInterface.PublicName();

			for (int j = 0; j < refInterface.MethodCount(); j++)
			{
				auto& m = refInterface.GetMethod(j);

				char methodName[256];
				SecureFormat(methodName, "%s_%s", fqInterfaceName, m.PublicName());

				NodeOption opt;
				opt.header.visibleName = methodName;

				char url[256];
				SafeFormat(url, "%s@%llX", methodName, (size_t)refInterface.PublicName());

				opt.header.url = url;
				opt.method = &Popup::AddNewNodeForMethod;

				backingList.push_back(opt);
			}

			const ISxyNamespace* baseNS = nullptr;
			cstr base = refInterface.Base();

			enum { MAX_RECURSIVE_DEPTH = 10};

			// we limit recursion because source code could erroneously specify an interface to be its own ancestor
			if (base && depth < MAX_RECURSIVE_DEPTH)
			{
				auto* baseInterface = db.FindInterface(base, &baseNS);
				if (baseInterface)
				{
					AddMethodsForInterface(*baseInterface, *baseNS, depth + 1);
				}
			}
		}

		void PopulateOptionsBackingList(const ISXYInterface& refInterface, const ISxyNamespace& ns)
		{
			if (!backingList.empty())
			{
				return;
			}

			AddMethodsForInterface(refInterface, ns);
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

					auto publicMethodsId = treeControl->Tree().AddChild(interfaceNameId, underscore + 1, CheckState_NoCheckBox);
					return publicMethodsId;
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
			dropInfo.anchor.node = NodeId();
			dropInfo.anchor.socket = SocketId();

			referencePosition = Vec2i {desktopPosition.x + 16, desktopPosition.y };
			Layout();

			SetCursor(LoadCursorA(NULL, IDC_WAIT));

			treeControl->Tree().ResetContent();

			PopulateOptionsBackingList();

			auto functionId = treeControl->Tree().AddChild(TREE_NODE_ID::Root(), "Functions", CheckState_NoCheckBox);
			auto publicMethodsId = treeControl->Tree().AddChild(TREE_NODE_ID::Root(), "Methods", CheckState_NoCheckBox);
			auto factoryId = treeControl->Tree().AddChild(TREE_NODE_ID::Root(), "Factories", CheckState_NoCheckBox);

			auto specialId = treeControl->Tree().AddChild(TREE_NODE_ID::Root(), "Special", CheckState_NoCheckBox);
			auto returnId = treeControl->Tree().AddChild(specialId, "<Return>", CheckState_NoCheckBox);

			TREE_NODE_ID templateControlId{ 0 };
			auto* templateFunction = cfgs.FindFunction(templateId);
			if (templateFunction)
			{
				char templateDesc[256];
				SafeFormat(templateDesc, "<Template>: %s", templateFunction->Name());

				templateControlId = treeControl->Tree().AddChild(specialId, templateDesc, CheckState_NoCheckBox);

				NodeOption templateOpt;
				templateOpt.header.url = templateFunction->Name();
				templateOpt.header.visibleName = templateDesc;
				templateOpt.method = &Popup::AddNodeFromTemplate;
				templateOpt.nodeId = templateControlId;

				backingList.push_back(templateOpt);
			}

			auto variablesId = treeControl->Tree().AddChild(TREE_NODE_ID::Root(), "Variables", CheckState_NoCheckBox);

			for (auto& f : backingList)
			{
				if (f.method == &Popup::AddNewNodeForFunction)
				{
					TREE_NODE_ID optionId = AddFunctionNameToTree(f.header.visibleName, functionId);
					f.nodeId = optionId;
				}
				else if (f.method == &Popup::AddNewNodeForMethod)
				{
					TREE_NODE_ID optionId = AddFunctionNameToTree(f.header.visibleName, publicMethodsId);
					f.nodeId = optionId;
				}
				else if (f.method == &Popup::AddNewNodeForFactory)
				{
					TREE_NODE_ID optionId = AddFunctionNameToTree(f.header.visibleName, factoryId);
					f.nodeId = optionId;
				}
			}

			NodeOption returnOpt;
			returnOpt.header.url = "<return>";
			returnOpt.header.visibleName = "<return>";
			returnOpt.method = &Popup::AddReturnNode;
			returnOpt.nodeId = returnId;

			backingList.push_back(returnOpt);

			populator.Variables().ForEachVariable(
				[this, variablesId](cstr name, cstr type) 
				{
					char url[256];
					SafeFormat(url, "%s %s", name, type);

					char desc[256];
					SafeFormat(desc, "GET %s (%s)", name, type);

					TREE_NODE_ID getVariableId = AddFunctionNameToTree(desc, variablesId);
				
					NodeOption opt;
					opt.header.url = url;
					opt.header.visibleName = desc;
					opt.method = &Popup::AddGetNode;
					opt.nodeId = getVariableId;

					backingList.push_back(opt);

					SafeFormat(desc, "SET %s (%s)", name, type);

					TREE_NODE_ID setVariableId = AddFunctionNameToTree(desc, variablesId);

					opt.header.url = url;
					opt.header.visibleName = desc;
					opt.method = &Popup::AddSetNode;
					opt.nodeId = setVariableId;

					backingList.push_back(opt);
				}
			);


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

			for (auto& f : backingList)
			{
				if (f.nodeId == id)
				{
					(this->*f.method)(f.header);
					backingList.clear();
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

		FunctionId templateId;

		void SetTemplate(FunctionId id) override
		{
			templateId = id;
		}

		void ShowInterface(Vec2i desktopPosition, Rococo::Editors::DesignerVec2 designPosition, const SexyStudio::ISXYInterface& refInterface, const SexyStudio::ISxyNamespace& ns, const CableDropped& dropInfo) override
		{
			this->designPosition = designPosition;
			this->dropInfo = dropInfo;

			referencePosition = Vec2i{ desktopPosition.x + 16, desktopPosition.y };

			Layout();

			SetCursor(LoadCursorA(NULL, IDC_WAIT));

			treeControl->Tree().ResetContent();

			PopulateOptionsBackingList(refInterface, ns);

			auto publicMethodsId = treeControl->Tree().AddChild(TREE_NODE_ID::Root(), "Methods", CheckState_NoCheckBox);

			TREE_NODE_ID firstId{ 0 };

			for (auto& f : backingList)
			{
				if (f.method == &Popup::AddNewNodeForMethod)
				{
					TREE_NODE_ID optionId = AddFunctionNameToTree(f.header.visibleName, publicMethodsId);
					f.nodeId = optionId;

					if (!firstId) firstId = optionId;
				}
			}

			SetCursor(LoadCursorA(NULL, IDC_ARROW));

			ShowWindow(*window, SW_SHOW);

			if (firstId)
			{
				treeControl->Tree().ScrollTo(firstId);
			}
		}
	};
}

namespace Rococo::CFGS
{
	ICFGSSexyPopup* CreateWin32ContextPopup(IAbstractEditor& editor, ICFGSDatabase& cfgs, SexyStudio::ISexyDatabase& db, INamespaceValidator& namespaceValidator, ICFGSCosmetics& cosmetics, ICFGSDesignerSpacePopupPopulator& populator)
	{
		auto* popup = new ANON::Popup(editor, cfgs, db, namespaceValidator, cosmetics, populator);
		popup->Create();
		return popup;
	}
}
