#include "sexy.types.h"
#include "sexy.debug.types.h"
#include "sexy.compiler.public.h"
#include "sexy.vm.h"
#include "sexy.vm.cpu.h"
#include "sexy.script.h"
#include "sexy.native.sys.type.h"
#include "rococo.os.win32.h"
#include "rococo.os.h"
#include "rococo.time.h"
#include "sexy.compiler.h"

#include <unordered_map>
#include <vector>
#include <algorithm>

#include <rococo.api.h>

#include <sexy.vm.cpu.h>
#include <rococo.stl.allocators.h>

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

struct CoSpec: public ILock
{
	// The id assigned by the coroutine control object when the coroutine is added to the coroutine list
	int64 id;
	// This is a pointer to the ICoroutine object within the sexy environment. It is offset from the coroutine interface
	ObjectStub* stub;
	// This is a pointer to the ICoroutine interface within the sexy environment. It is offset from the ObjectStub pointer
	CoroutineRef coroutine;

	// The id of the method ICoroutine.Run in the ICoroutine interface within the sexy environment
	ID_BYTECODE runId;

	// The coroutines private virtual machine register state
	VariantValue registers[VM::CPU::DATA_REGISTER_COUNT];

	// The coroutine's private stack
	uint8* startOfStackMemory = nullptr;
	uint8* endOfStackMemory = nullptr;

	// The tick count when this coroutine is due to wake up. 0 implies the routine is currently active
	int64 nextWaitTrigger = 0;

	// The coroutine will lock during execution, preventing it from removing itself from the coroutine list
	bool isLocked = false;

	// True until the coroutine initializes itself before the first yields
	bool isStarted = true;

	bool isDormant = false;

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

typedef std::unordered_map<int64, CoSpec*> SpecMap;

// A pool of virtual memory for use as stack memory by coroutines
struct StackPool
{
	std::vector<uint8*, Memory::SexyAllocator<uint8*>> freePointers;
	std::unordered_map<uint8*, uint32, std::hash<uint8*>, std::equal_to<uint8*>, Memory::SexyAllocator<std::pair<uint8* const, uint32>>> allocatedPointers;

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
	enum Constants: int64 { nextYear = 0x4001000100010001 };
	int64 nextId = 1;
	IScriptSystem& ss;
	IProgramObject& object;
	VM::IVirtualMachine& vm;
	SpecMap specs;
	SpecMap::iterator next;
	StackPool stacks;
	int64 nextWakeTime = Constants::nextYear;

	std::vector<CoSpec*,Memory::SexyAllocator<CoSpec*>> dormantCoSpecs;

	Coroutines(IScriptSystem& _ss) :
		ss(_ss), object(ss.ProgramObject()), vm(object.VirtualMachine())
	{
		next = specs.end();
	}

	int64 Add(Rococo::Sex::CoroutineRef coroutine) override
	{
		InterfacePointer pInterface = (InterfacePointer)coroutine;

		auto* stub = InterfaceToInstance(pInterface);

		ss.ProgramObject().IncrementRefCount(coroutine);
		
		ID_BYTECODE runId = (*coroutine)->FirstMethodId;

		int64 id = nextId++;
		next = specs.insert(std::make_pair(id, new CoSpec( id, stub, coroutine, runId ))).first;

		return id;
	}

	void Wakeup(int64 now)
	{
		if (now > nextWakeTime)
		{
			// Wake up dormant jobs, by sticking them on the active map

			// N.B the more work we do here the greater the cost of context switching,
			// and the fewer the coroutines that can progress in a given time frame

			int64 nearestWakeUpTime = nextYear;

			for (auto& d : dormantCoSpecs)
			{
				if (now > d->nextWaitTrigger)
				{
					specs.insert(std::make_pair(d->id, d));
					d->isDormant = false;
				}
				else
				{
					nearestWakeUpTime = min(d->nextWaitTrigger, nearestWakeUpTime);
				}
			}

			nextWakeTime = nearestWakeUpTime;

			// Anything that was awoken is now removed from the dormant queue

			struct
			{
				bool operator()(CoSpec* spec)
				{
					return !spec->isDormant;
				}
			} noLongerDormant;
			auto erasePoint = std::remove_if(dormantCoSpecs.begin(), dormantCoSpecs.end(), noLongerDormant);
			dormantCoSpecs.erase(erasePoint, dormantCoSpecs.end());
		}
	}

	int64 Continue() override
	{
		int64 now = Rococo::Time::TickCount();

		Wakeup(now);

		// N.B the more work we do here the greater the cost of context switching,
		// and the fewer the coroutines that can progress in a given time frame

		if (next == specs.end())
		{
			next = specs.begin();
		}

		if (next != specs.end())
		{
			int64 id = next->first;
			auto* spec = next->second;

			auto& cpu = vm.Cpu();
			CpuShadow shadow(cpu);

			struct Anon: IEventCallback<VM::WaitArgs>
			{
				VM::IVirtualMachine* vm;
				int64 nextWaitTrigger = 0;
				void OnEvent(VM::WaitArgs& args) override
				{
					nextWaitTrigger = args.nextWakeTime;
				}

				~Anon()
				{
					vm->SetWaitHandler(nullptr);
				}
			} waitMonitor;

			waitMonitor.vm = &vm;

			vm.SetWaitHandler(&waitMonitor);
			id = Continue(*spec);

			shadow.Restore();

			if (waitMonitor.nextWaitTrigger > now)
			{
				// Coroutine is yielding for a bit
				spec->nextWaitTrigger = waitMonitor.nextWaitTrigger;
				spec->isDormant = true;
				dormantCoSpecs.push_back(spec);

				if (spec->id == next->first)
				{
					next = specs.erase(next);
				}
				else
				{
					auto j = specs.find(spec->id);
					next = specs.erase(j);
				}

				nextWakeTime = min(waitMonitor.nextWaitTrigger, nextWakeTime);
			}
			else
			{
				next++;
			}

			return id;
		}
		else // Nothing in the spec map
		{
			return dormantCoSpecs.empty() ? 0 : -1;
		}
	}

	void Detach(CoSpec& spec)
	{
		object.DecrementRefCount(spec.coroutine);
		auto i = specs.find(spec.id);
		if (i != specs.end())
		{
			delete i->second;
			next = specs.erase(i);
		}
	}

	void UpdateWithResult(CoSpec& spec, EXECUTERESULT result) 
	{
		vm.SetStatus(EXECUTERESULT_RUNNING);

		switch (result)
		{
		case EXECUTERESULT_YIELDED:
			break;
		case EXECUTERESULT_BREAKPOINT:
			Throw(0, "%s %lld hit a breakpoint", spec.ClassName(), spec.id);
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

		void Restore() // We could put this in a destructor, but sometime we dont want to restore, such as on a crash
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

	void Release(int64 id) override
	{
		auto i = specs.find(id);
		if (i != specs.end())
		{
			if (i->second->isLocked)
			{
				Throw(0, "Coroutine %s %lld tried to release itself during execution", i->second->ClassName(), id);
			}
			int64 id = i->second->id;
			object.DecrementRefCount(i->second->coroutine);
			// Possibly a destructor invalidated the iterator, so grab again
			auto i = specs.find(id);
			if (i != specs.end())
			{
				delete i->second;
				next = specs.erase(i);
			}
		}
		else
		{
			struct
			{
				int64 id;
				bool operator()(CoSpec* spec)
				{
					return spec->id == id;
				}
			} matchesId;
			matchesId.id = id;

			auto erasePoint = std::remove_if(dormantCoSpecs.begin(), dormantCoSpecs.end(), matchesId);
			for (auto i = erasePoint; i != dormantCoSpecs.end(); ++i)
			{
				delete *i;
			}
			dormantCoSpecs.erase(erasePoint, dormantCoSpecs.end());
		}
	}

	void ReleaseAll() override
	{
		for (auto& i : specs)
		{
			object.DecrementRefCount(i.second->coroutine);
			delete i.second;
		}

		for (auto& d : dormantCoSpecs)
		{
			object.DecrementRefCount(d->coroutine);
			delete d;
		}

		specs.clear();
		dormantCoSpecs.clear();

		nextWakeTime = nextYear;
	}
};

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

extern "C"
{
	DLLEXPORT INativeLib* CreateLib(Rococo::Script::IScriptSystem& ss)
	{
		class CoRountinesNativeLib : public INativeLib
		{
		private:
			IScriptSystem& ss;
			Coroutines coroutines;

		public:
			CoRountinesNativeLib(IScriptSystem& _ss) : ss(_ss), coroutines(ss)
			{
			}

		private:
			virtual void AddNativeCalls()
			{
				Sys::AddNativeCalls_SysICoroutineControl(ss, static_cast<Sys::ICoroutineControl*>(&coroutines));
			}

			virtual void ClearResources()
			{
				coroutines.ReleaseAll();
			}

			virtual void Release()
			{
				delete this;
			}
		};
		return new CoRountinesNativeLib(ss);
	}
}

#include <sexy.lib.util.h>
#include <sexy.lib.sexy-util.h>