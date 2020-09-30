#include <rococo.mplat.h>
#include <rococo.handles.h>

using namespace Rococo;
using namespace Rococo::Puppet;

struct IPuppetSupervisor : public IPuppet
{
	virtual void SetActive(bool isActive) = 0;
	virtual void Free() = 0;
};

struct PuppetBinding
{
	int64 id;
	IPuppetSupervisor* supervisor;
};

namespace Rococo
{
	namespace Puppet
	{
		IPuppetSupervisor* CreatePuppet(ID_PUPPET id) { return nullptr; }
	}
}

struct Puppets : public IPuppetsSupervisor
{
	typedef HandleTable<IPuppetSupervisor*, 16> TPuppetHandleTable;
	TPuppetHandleTable allPuppets;

	std::vector<ID_PUPPET> activePuppets;

	size_t maxActivePuppets;

	Puppets(size_t maxPuppets, size_t _maxActivePuppets) :
		maxActivePuppets(_maxActivePuppets),
		allPuppets("Puppets", maxPuppets)
	{
		activePuppets.reserve(_maxActivePuppets);
	}

	void KillAllPuppets() override
	{
		auto hPuppet = allPuppets.GetFirstHandle();
		while(hPuppet)
		{
			IPuppetSupervisor* puppet;
			if (allPuppets.TryGet(hPuppet, &puppet))
			{
				puppet->MarkForDeath();
			}

			allPuppets.Destroy(hPuppet, nullptr);

			hPuppet = allPuppets.GetNextHandle(hPuppet);
		}
	}

	Rococo::Puppet::IPuppet* /* puppet */ CreatePuppet(const Rococo::Puppet::NewPuppetDesc& puppetDescription) override
	{
		auto hPuppet = allPuppets.CreateNew();
		try
		{
			auto p = Rococo::Puppet::CreatePuppet(ID_PUPPET{ hPuppet.Value() });
			allPuppets.Set(hPuppet, p);
			return p;
		}
		catch (IException& ex)
		{
			allPuppets.Destroy(hPuppet, nullptr);
			Throw(ex.ErrorCode(), "%s", ex.Message());
		}
	}

	ID_PUPPET GetFirstPuppetId() override
	{
		return ID_PUPPET{ allPuppets.GetFirstHandle() };
	}

	Rococo::Puppet::IPuppet* /* puppet */ FindPuppetById(ID_PUPPET id) override
	{
		TPuppetHandleTable::Handle hPuppet{ id.value };
		
		IPuppetSupervisor* puppet = nullptr;
		return allPuppets.TryGet(hPuppet, &puppet) ? puppet : nullptr;
	}

	ID_PUPPET /* nextId */ GetNextPuppetId(ID_PUPPET id) override
	{
		auto hNext = allPuppets.GetNextHandle(TPuppetHandleTable::Handle{ id.value });
		return ID_PUPPET{ hNext };
	}

	void Free() override
	{
		delete this;
	}
};

namespace Rococo
{
	namespace Puppet
	{
		IPuppetsSupervisor* CreatePuppets(size_t maxPupepts, size_t maxActivePuppets)
		{
			return new Puppets(maxPupepts, maxActivePuppets);
		}
	}
}