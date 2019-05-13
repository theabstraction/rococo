#include "sexy.types.h"
#include "sexy.debug.types.h"
#include "sexy.compiler.public.h"
#include "sexy.vm.h"
#include "sexy.vm.cpu.h"
#include "sexy.script.h"
#include "sexy.native.sys.type.h"
#include "rococo.os.win32.h"
#include "..\STC\stccore\sexy.compiler.h"

#include <unordered_map>
#include <vector>

#include <rococo.api.h>

#include <sexy.vm.cpu.h>

using namespace Rococo;
using namespace Rococo::Script;
using namespace Rococo::Compiler;
using namespace Rococo::SysType;
using namespace Rococo::Sex;

namespace Rococo
{
	namespace Sex
	{
		typedef InterfacePointer CoroutineRef;
	}
}

#include "coroutine.sxh.h"

Sys::ICoroutineControl* FactoryConstructSysCoroutines(Sys::ICoroutineControl* _context)
{
	return _context;
}

#include "coroutine.sxh.inl"

#ifdef _WIN32

BOOLEAN WINAPI DllMain(HINSTANCE hDllHandle, DWORD nReason, LPVOID Reserved)
{
	BOOLEAN bSuccess = TRUE;

	switch (nReason)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hDllHandle);
		break;
	case DLL_PROCESS_DETACH:
		break;
	}

	return bSuccess;
}

# define DLLEXPORT __declspec(dllexport)
#else
# define DLLEXPORT
#endif

struct CoSpec: public ILock
{
	int64 id;
	ObjectStub* stub;
	CoroutineRef coroutine;
	ID_BYTECODE runId;
	VariantValue registers[VM::CPU::DATA_REGISTER_COUNT];
	uint8* startOfStackMemory = nullptr;
	uint8* endOfStackMemory = nullptr;

	bool isLocked = false;
	bool isStarted = true;

	CoSpec(int64 _id, ObjectStub* _stub, CoroutineRef _coroutine, ID_BYTECODE _runId):
		id(_id), stub(_stub), coroutine(_coroutine), runId(_runId)
	{

	}

	void Lock() override
	{
		isLocked = true;
	}

	void Unlock() override
	{
		isLocked = false;
	}

	cstr ClassName() const
	{
		return stub->Desc->TypeInfo->Name();
	}
};

typedef std::unordered_map<int64, CoSpec> SpecMap;

// A pool of virtual memory for use as stack memory by coroutines
struct StackPool
{
	std::vector<uint8*> freePointers;
	std::unordered_map<uint8*, uint32> allocatedPointers;

	enum { StackSize = 8192 };

	~StackPool()
	{
		for (auto i : allocatedPointers)
		{
			Rococo::OS::FreeBoundedMemory(i.first);
		}
	}

	uint8* AllocateBuffer()
	{
		if (!freePointers.empty())
		{
			uint8* back = freePointers.back();
			freePointers.pop_back();
			return back;
		}
		else
		{
			uint8* p = (uint8*)Rococo::OS::AllocBoundedMemory(StackSize);
			allocatedPointers[p] = 0;
			return p;
		}
	}

	void FreeBuffer(uint8* pMemory)
	{
		freePointers.push_back(pMemory);
	}
};

struct Coroutines : public Sys::ICoroutineControl
{
	int64 nextId = 1;
	IScriptSystem& ss;
	IProgramObject& object;
	VM::IVirtualMachine& vm;
	SpecMap specs;
	SpecMap::iterator next;
	StackPool stacks;

	Coroutines(IScriptSystem& _ss) :
		ss(_ss), object(ss.ProgramObject()), vm(object.VirtualMachine())
	{
	}

	int64 Add(Rococo::Sex::CoroutineRef coroutine) override
	{
		uint8* pByteInterface = (uint8*)coroutine;
		uint8* pInstance = pByteInterface + (*coroutine)->OffsetToInstance;
		auto* stub = (ObjectStub*)pInstance;

		ss.ProgramObject().IncrementRefCount(coroutine);
		
		ID_BYTECODE runId = (*coroutine)->FirstMethodId;

		int64 id = nextId++;
		next = specs.insert(std::make_pair(id, CoSpec( id, stub, coroutine, runId))).first;

		return id;
	}

	int64 Continue() override
	{
		if (next == specs.end())
		{
			next = specs.begin();
		}

		int64 id = 0;

		if (next != specs.end())
		{
			id = next->first;
			auto& spec = next->second;
			next++;

			auto& cpu = vm.Cpu();
			CpuShadow shadow(cpu);
			return Continue(spec);
		}

		return id;
	}

	void Detach(CoSpec& spec)
	{
		object.DecrementRefCount(spec.coroutine);
		auto i = specs.find(spec.id);
		if (i != specs.end())
		{
			next = specs.erase(i);
		}
	}

	void UpdateWithResult(CoSpec& spec, EXECUTERESULT result) 
	{
		vm.SetStatus(EXECUTERESULT_RUNNING);

		switch (result)
		{
		case EXECUTERESULT_YIELDED:
		case EXECUTERESULT_BREAKPOINT:
			break;
		case EXECUTERESULT_TERMINATED:
			// Co-routine completed
			Detach(spec);
			break;
		case EXECUTERESULT_THROWN:
			Throw(0, "%s %lld threw an exception", spec.ClassName(), spec.id);
			Detach(spec);
			break;
		case EXECUTERESULT_SEH:
			Detach(spec);
			Throw(0, "%s %lld threw SEH", spec.ClassName(), spec.id);
			break;
		default:
			Detach(spec);
			Throw(0, "Unknown Virtual Machine state: %u executing coroutine for %s %lld", result, spec.ClassName(), spec.id);
		}
	}

	// Creates a temp buffer for the VM cpu state. When the object goes out of scope the cpu state is restored
	struct CpuShadow
	{
		VM::CPU& cpu;

		VariantValue mainCpuState[VM::CPU::DATA_REGISTER_COUNT];
		uint8* startOfStackMemory = nullptr;
		uint8* endOfStackMemory = nullptr;

		CpuShadow(VM::CPU& _cpu): cpu(_cpu)
		{
			memcpy(mainCpuState, &cpu.D, sizeof(VariantValue) * VM::CPU::DATA_REGISTER_COUNT);
			startOfStackMemory = cpu.StackStart;
			endOfStackMemory = cpu.StackEnd;
		}

		~CpuShadow()
		{
			memcpy(&cpu.D, mainCpuState, sizeof(VariantValue) * VM::CPU::DATA_REGISTER_COUNT);
			cpu.StackStart = startOfStackMemory;
			cpu.StackEnd = endOfStackMemory;
		}
	};

	int64 Continue(CoSpec& spec)
	{
		auto& cpu = vm.Cpu();

		EXECUTERESULT result;
		VM::ExecutionFlags flags;

		{ // Spec lock zone
			Sync sync(spec);
			vm.GetLastFlags(flags);

			if (spec.isStarted)
			{
				cpu.SetPC(cpu.ProgramStart);
				cpu.D[5].byteCodeIdValue = spec.runId;

				cpu.D[VM::REGISTER_SP].uint8PtrValue = spec.registers[VM::REGISTER_SP].uint8PtrValue = spec.startOfStackMemory = stacks.AllocateBuffer();
				spec.endOfStackMemory = spec.startOfStackMemory + StackPool::StackSize;
				cpu.StackStart = spec.startOfStackMemory;
				cpu.StackEnd = spec.endOfStackMemory;

				spec.isStarted = false;

				cpu.Push((int32)0);

				cpu.Push(spec.coroutine);

				result = vm.Execute(VM::ExecutionFlags(flags.ThrowToQuit, true, true), nullptr);
			}
			else
			{
				memcpy(&cpu.D, spec.registers, sizeof(VariantValue) * VM::CPU::DATA_REGISTER_COUNT);
				cpu.StackStart = spec.startOfStackMemory;
				cpu.StackEnd = spec.endOfStackMemory;

				result = vm.ContinueExecution(VM::ExecutionFlags(flags.ThrowToQuit, true, true), nullptr);
			}
		} // Spec unlocked

		memcpy(spec.registers, &cpu.D, sizeof(VariantValue) * VM::CPU::DATA_REGISTER_COUNT);

		UpdateWithResult(spec, result);

		return spec.id;
	}

	boolean32 ContinueSpecific(int64 id) override
	{
		auto i = specs.find(id);
		if (i == specs.end())
		{
			return false;
		}
		else
		{
			auto& cpu = vm.Cpu();
			CpuShadow shadow(cpu);
			Continue(i->second);
			return true;
		}
	}

	boolean32 Release(int64 id) override
	{
		auto i = specs.find(id);
		if (i != specs.end())
		{
			if (i->second.isLocked)
			{
				Throw(0, "Coroutine %s %lld tried to release itself during execution", i->second.ClassName(), id);
			}
			int64 id = i->second.id;
			object.DecrementRefCount(i->second.coroutine);
			// Possibly a destructor invalidated the iterator, so grab again
			auto i = specs.find(id);
			if (i != specs.end())
			{
				next = specs.erase(i);
			}
			return true;
		}
		else
		{
			return false;
		}
	}

	void ReleaseAll()
	{
		for (auto& i : specs)
		{
			object.DecrementRefCount(i.second.coroutine);
		}

		specs.clear();
	}
};

extern "C"
{
	DLLEXPORT INativeLib* CreateLib(Rococo::Script::IScriptSystem& ss)
	{
		class MathsNativeLib : public INativeLib
		{
		private:
			IScriptSystem& ss;
			Coroutines coroutines;

		public:
			MathsNativeLib(IScriptSystem& _ss) : ss(_ss), coroutines(ss)
			{
			}

		private:
			virtual void AddNativeCalls()
			{
				Sys::AddNativeCalls_SysICoroutineControl(ss, static_cast<Sys::ICoroutineControl*>(&coroutines));

				auto& nsSys = ss.AddNativeNamespace("Sys");
			}

			virtual void ClearResources()
			{
				coroutines.ReleaseAll();
			}

			virtual void Release()
			{
				ClearResources();
				delete this;
			}
		};
		return new MathsNativeLib(ss);
	}
}