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
#include "Sexy.Compiler.h"

#include <sexy.unordered_map.h>
#include <sexy.vector.h>

#include <algorithm>

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

#include "coroutines.sxh.h"

Sys::ICoroutineControl* FactoryConstructSysCoroutines(Sys::ICoroutineControl* _context)
{
	return _context;
}

#include "coroutines.sxh.inl"

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

typedef TSexyHashMap<int64, CoSpec*> SpecMap;

// A pool of virtual memory for use as stack memory by coroutines
struct StackPool
{
	TSexyVector<uint8*> freePointers;
	TSexyHashMap<uint8*, uint32, std::hash<uint8*>, std::equal_to<uint8*>> allocatedPointers;

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
	// 0 means "pick specs.begin()". Never store a live hash-map iterator across VM calls.
	int64 nextIdToRun = 0;
	// True while Execute/ContinueExecution is running a slice. Add/Release/ReleaseAll are
	// deferred until the slice ends (including Release of the running coroutine). Nested
	// Continue throws immediately — it cannot be deferred without re-entering the VM.
	bool inSlice = false;
	char sliceError[256] = { 0 };
	IScriptSystem& ss;
	IProgramObject& object;
	VM::IVirtualMachine& vm;
	SpecMap specs;
	StackPool stacks;
	int64 nextWakeTime = Constants::nextYear;

	TSexyVector<CoSpec*> dormantCoSpecs;

	enum class PendingKind { Add, Release, ReleaseAll };

	struct PendingOp
	{
		PendingKind kind;
		int64 id = 0;
		CoSpec* spec = nullptr;
	};

	TSexyVector<PendingOp> pendingOps;

	Coroutines(IScriptSystem& _ss) :
		ss(_ss), object(ss.ProgramObject()), vm(object.VirtualMachine())
	{
	}

	int64 Add(Rococo::Sex::CoroutineRef coroutine) override
	{
		InterfacePointer pInterface = (InterfacePointer)coroutine;

		auto* stub = InterfaceToInstance(pInterface);

		ss.ProgramObject().IncrementRefCount(coroutine);

		ID_BYTECODE runId = (*coroutine)->FirstMethodId;

		int64 id = nextId++;
		auto* spec = new CoSpec(id, stub, coroutine, runId);

		if (inSlice)
		{
			PendingOp op;
			op.kind = PendingKind::Add;
			op.id = id;
			op.spec = spec;
			pendingOps.push_back(op);
		}
		else
		{
			specs.insert(std::make_pair(id, spec));
		}

		return id;
	}

	void Wakeup(int64 now)
	{
		if (now >= nextWakeTime)
		{
			// Wake up dormant jobs, by sticking them on the active map

			// N.B the more work we do here the greater the cost of context switching,
			// and the fewer the coroutines that can progress in a given time frame

			int64 nearestWakeUpTime = nextYear;

			for (auto& d : dormantCoSpecs)
			{
				if (now >= d->nextWaitTrigger)
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

	void EraseFromCollections(int64 id)
	{
		auto i = specs.find(id);
		if (i != specs.end())
		{
			specs.erase(i);
		}

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
		dormantCoSpecs.erase(erasePoint, dormantCoSpecs.end());
	}

	void DestroyDetached(CoSpec* spec)
	{
		if (!spec)
		{
			return;
		}

		object.DecrementRefCount(spec->coroutine);

		if (spec->startOfStackMemory)
		{
			stacks.FreeBuffer(spec->startOfStackMemory);
			spec->startOfStackMemory = nullptr;
			spec->endOfStackMemory = nullptr;
		}

		delete spec;
	}

	// Unlink from specs/dormant first so a destructor that calls Release/ReleaseAll cannot double-delete.
	void UnlinkAndDestroy(CoSpec* spec)
	{
		if (!spec)
		{
			return;
		}

		int64 id = spec->id;
		EraseFromCollections(id);

		for (auto& op : pendingOps)
		{
			if (op.kind == PendingKind::Add && op.id == id)
			{
				op.spec = nullptr;
			}
		}

		DestroyDetached(spec);
	}

	void ReleaseNow(int64 id)
	{
		auto i = specs.find(id);
		if (i != specs.end())
		{
			UnlinkAndDestroy(i->second);
			return;
		}

		for (auto* d : dormantCoSpecs)
		{
			if (d->id == id)
			{
				UnlinkAndDestroy(d);
				return;
			}
		}

		for (auto& op : pendingOps)
		{
			if (op.kind == PendingKind::Add && op.id == id && op.spec)
			{
				CoSpec* s = op.spec;
				op.spec = nullptr;
				DestroyDetached(s);
				return;
			}
		}
	}

	void ReleaseAllNow()
	{
		TSexyVector<CoSpec*> all;

		for (auto& i : specs)
		{
			all.push_back(i.second);
		}
		specs.clear();

		for (auto* d : dormantCoSpecs)
		{
			all.push_back(d);
		}
		dormantCoSpecs.clear();

		for (auto& op : pendingOps)
		{
			if (op.kind == PendingKind::Add && op.spec)
			{
				all.push_back(op.spec);
				op.spec = nullptr;
			}
		}
		pendingOps.clear();

		nextIdToRun = 0;
		nextWakeTime = nextYear;

		for (auto* s : all)
		{
			DestroyDetached(s);
		}
	}

	void FlushPending()
	{
		if (pendingOps.empty())
		{
			return;
		}

		TSexyVector<PendingOp> ops = pendingOps;
		pendingOps.clear();

		for (auto& op : ops)
		{
			if (op.kind == PendingKind::ReleaseAll)
			{
				ReleaseAllNow();
			}
			else if (op.kind == PendingKind::Add)
			{
				if (op.spec)
				{
					specs.insert(std::make_pair(op.id, op.spec));
				}
			}
			else if (op.kind == PendingKind::Release)
			{
				ReleaseNow(op.id);
			}
		}
	}

	void AdvanceCursorAfter(int64 ranId)
	{
		auto i = specs.find(ranId);
		if (i == specs.end())
		{
			nextIdToRun = 0;
			return;
		}

		++i;
		nextIdToRun = (i == specs.end()) ? 0 : i->first;
	}

	// Saves the host VM cpu state and always restores it, including when a coroutine throws.
	struct CpuShadow
	{
		VM::CPU& cpu;

		VariantValue mainCpuState[VM::CPU::DATA_REGISTER_COUNT];
		uint8* startOfStackMemory = nullptr;
		uint8* endOfStackMemory = nullptr;
		bool restore = true;

		CpuShadow(VM::CPU& _cpu): cpu(_cpu)
		{
			memcpy(mainCpuState, &cpu.D, sizeof(VariantValue) * VM::CPU::DATA_REGISTER_COUNT);
			startOfStackMemory = cpu.StackStart;
			endOfStackMemory = cpu.StackEnd;
		}

		~CpuShadow()
		{
			Restore();
		}

		void Restore()
		{
			if (!restore)
			{
				return;
			}

			memcpy(&cpu.D, mainCpuState, sizeof(VariantValue) * VM::CPU::DATA_REGISTER_COUNT);
			cpu.StackStart = startOfStackMemory;
			cpu.StackEnd = endOfStackMemory;
			restore = false;
		}
	};

	int64 Continue() override
	{
		if (inSlice)
		{
			cstr msg = "Cannot call ICoroutineControl.Continue while a coroutine is executing";
			size_t n = 0;
			while (msg[n] != 0 && n < 255) { sliceError[n] = msg[n]; n++; }
			sliceError[n] = 0;
			Throw(0, "%s", sliceError);
		}

		int64 now = Rococo::Time::TickCount();

		Wakeup(now);

		// N.B the more work we do here the greater the cost of context switching,
		// and the fewer the coroutines that can progress in a given time frame

		if (specs.empty())
		{
			return dormantCoSpecs.empty() ? 0 : -1;
		}

		auto i = specs.find(nextIdToRun);
		if (i == specs.end())
		{
			i = specs.begin();
		}

		CoSpec* spec = i->second;
		int64 id = spec->id;

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

		inSlice = true;
		sliceError[0] = 0;
		try
		{
			id = Continue(*spec);
		}
		catch (...)
		{
			inSlice = false;
			FlushPending();
			vm.SetStatus(EXECUTERESULT_RUNNING);
			throw;
		}
		inSlice = false;

		// Park only if the wake time is still strictly in the future (i.e. not now >= trigger).
		if (waitMonitor.nextWaitTrigger > now)
		{
			auto j = specs.find(id);
			if (j != specs.end())
			{
				spec = j->second;
				spec->nextWaitTrigger = waitMonitor.nextWaitTrigger;
				spec->isDormant = true;
				dormantCoSpecs.push_back(spec);
				specs.erase(j);
				nextWakeTime = min(waitMonitor.nextWaitTrigger, nextWakeTime);
			}
		}

		FlushPending();
		AdvanceCursorAfter(id);
		return id;
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
			UnlinkAndDestroy(&spec);
			break;
		default:
			{
		char className[256];
		className[0] = 0;
		if (cstr cn = spec.ClassName())
		{
			size_t n = 0;
			while (cn[n] != 0 && n < 255) { className[n] = cn[n]; n++; }
			className[n] = 0;
		}
		int64 id = spec.id;
		UnlinkAndDestroy(&spec);
		Throw(0, "Unknown Virtual Machine state: %u executing coroutine for %s %lld", result, className, id);
			}
			break;
		}
	}

	void FailSlice(CoSpec& spec, cstr resultString)
	{
		char className[256];
		className[0] = 0;
		if (cstr cn = spec.ClassName())
		{
			size_t n = 0;
			while (cn[n] != 0 && n < 255) { className[n] = cn[n]; n++; }
			className[n] = 0;
		}
		UnlinkAndDestroy(&spec);
		vm.SetStatus(EXECUTERESULT_RUNNING);
		if (sliceError[0] != 0)
		{
			char msg[256];
			size_t n = 0;
			while (sliceError[n] != 0 && n < 255) { msg[n] = sliceError[n]; n++; }
			msg[n] = 0;
			sliceError[0] = 0;
			Throw(0, "%s", msg);
		}
		Throw(0, "Coroutine %s crashed. %s", className, resultString);
	}

	int64 Continue(CoSpec& spec)
	{
		auto& cpu = vm.Cpu();

		EXECUTERESULT result;
		VM::ExecutionFlags flags;

		const uint8* lastKnownGoodPC = cpu.ProgramStart;

		{ // Spec lock zone
			OS::Sync sync(spec);
			vm.GetLastFlags(flags);

			if (spec.isStarted)
			{
				cpu.SetPC(cpu.ProgramStart);

				if (spec.runId == 0)
				{
					Throw(0, "Unexpected bad run id: spec %s", spec.ClassName());
				}

				cpu.D[5].byteCodeIdValue = spec.runId;

				cpu.D[VM::REGISTER_SP].uint8PtrValue = spec.registers[VM::REGISTER_SP].uint8PtrValue = spec.startOfStackMemory = stacks.AllocateBuffer();
				spec.endOfStackMemory = spec.startOfStackMemory + StackPool::StackSize;
				cpu.StackStart = spec.startOfStackMemory;
				cpu.StackEnd = spec.endOfStackMemory;

				spec.isStarted = false;

				cpu.Push((int32)0);

				cpu.Push(spec.coroutine);

				// Set SF to the post-push SP explicitly. Do not rely on CorrectSF, which compares
				// the host SF pointer with this freshly allocated stack.
				cpu.SetSF(cpu.SP());

				result = vm.Execute(VM::ExecutionFlags(flags.ThrowToQuit, true, false), nullptr);
			}
			else
			{
				memcpy(&cpu.D, spec.registers, sizeof(VariantValue) * VM::CPU::DATA_REGISTER_COUNT);
				cpu.StackStart = spec.startOfStackMemory;
				cpu.StackEnd = spec.endOfStackMemory;
				lastKnownGoodPC = cpu.PC();

				result = vm.ContinueExecution(VM::ExecutionFlags(flags.ThrowToQuit, true, true), nullptr);
			}
		} // Spec unlocked

		// Coroutine stacks come from AllocBoundedMemory, so on Windows running off either end of the stack faults on a guard page
		// and the slice ends with EXECUTERESULT_SEH. The stack grows upward and Push writes at SP before advancing it, so a push
		// that hits the upper guard leaves SP == StackEnd, while a StackAlloc that crosses it leaves SP > StackEnd.
		bool sehAtStackEnd = result == EXECUTERESULT_SEH && cpu.SP() >= cpu.StackEnd;
		if (sehAtStackEnd || cpu.SP() < cpu.StackStart || cpu.SP() > cpu.StackEnd)
		{
			char overflowMessage[128];
			Strings::SafeFormat(overflowMessage, sizeof overflowMessage, "stack overflow. Coroutine stacks are %d bytes", (int)StackPool::StackSize);
			FailSlice(spec, overflowMessage);
		}

		if (result == EXECUTERESULT_THROWN || result == EXECUTERESULT_SEH || result == EXECUTERESULT_ILLEGAL)
		{
			const uint8* pc = cpu.PC();
			if (pc < cpu.ProgramStart || pc > cpu.ProgramEnd)
			{
				cpu.SetPC(lastKnownGoodPC);
			}

			cstr resultString = "undefined";
			switch (result)
			{
			case EXECUTERESULT_THROWN:
				resultString = "EXECUTERESULT_THROWN";
				break;
			case EXECUTERESULT_SEH:
				resultString = "EXECUTERESULT_SEH";
				break;
			case EXECUTERESULT_ILLEGAL:
				resultString = "EXECUTERESULT_ILLEGAL";
				break;
			}
			FailSlice(spec, resultString);
		}

		memcpy(spec.registers, &cpu.D, sizeof(VariantValue) * VM::CPU::DATA_REGISTER_COUNT);

		int64 id = spec.id;
		UpdateWithResult(spec, result);
		return id;
	}

	void Release(int64 id) override
	{
		if (inSlice)
		{
			PendingOp op;
			op.kind = PendingKind::Release;
			op.id = id;
			pendingOps.push_back(op);
			return;
		}

		ReleaseNow(id);
	}

	void ReleaseAll() override
	{
		if (inSlice)
		{
			PendingOp op;
			op.kind = PendingKind::ReleaseAll;
			pendingOps.push_back(op);
			return;
		}

		ReleaseAllNow();
	}
};

#ifdef _WIN32
using namespace MSWindows;
BOOL DllMain(HINSTANCE hDllHandle, DWORD nReason, LPVOID reserved)
{
	UNUSED(reserved);

	BOOL bSuccess = TRUE;

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
				Sys::Interop::AddNativeCalls_SysICoroutineControl(ss, static_cast<Sys::ICoroutineControl*>(&coroutines));
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
