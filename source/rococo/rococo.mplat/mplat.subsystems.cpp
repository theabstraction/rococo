#include <rococo.mplat.h>
#include <unordered_map>

namespace Rococo
{
	struct SubsystemDesc
	{
		ID_SUBSYSTEM Id;
		ID_SUBSYSTEM ParentId; // 0 if a root system
		std::vector<ID_SUBSYSTEM> children;
	};

	template<class T, class ITEM> void InsertUnique(T& container, ITEM item)
	{
		if (std::find(container.begin(), container.end(), item) == container.end())
		{
			container.push_back(item);
		}
	}

	class Subsystems : public ISubsystemsSupervisor, public ISubsystemMonitor
	{
		std::unordered_map<ID_SUBSYSTEM, ISubsystem*, ID_SUBSYSTEM> idToSubsystem;
		std::unordered_map<ISubsystem*, SubsystemDesc> subSystemToDesc;
		std::vector<ID_SUBSYSTEM> rootSystems;

		int32 nextId = 1;
	public:
		void Free() override
		{
			delete this;
		}

		ISubsystemMonitor& Monitor() override
		{
			return *this;
		}

		ID_SUBSYSTEM RegisterAtRoot(ISubsystem& subsystem) override
		{
			auto rootId = Register(subsystem, ID_SUBSYSTEM(0));
			InsertUnique(rootSystems, rootId);
			return rootId;
		}

		ID_SUBSYSTEM Register(ISubsystem& subsystem, ID_SUBSYSTEM parentId) override
		{
			auto i = subSystemToDesc.insert(std::make_pair(&subsystem, SubsystemDesc{ ID_SUBSYSTEM::Invalid(), ID_SUBSYSTEM::Invalid() }));
			if (i.second)
			{
				// Was newly inserted
				auto id = ID_SUBSYSTEM(nextId++);
				i.first->second.Id = id;
				i.first->second.ParentId = parentId;
				idToSubsystem[i.first->second.Id] = &subsystem;

				auto parent = idToSubsystem.find(parentId);
				if (parent != idToSubsystem.end())
				{
					auto* parentSystem = parent->second;
					auto parentDesc = subSystemToDesc.find(parentSystem);
					if (parentDesc != subSystemToDesc.end())
					{
						InsertUnique(parentDesc->second.children, id);
					}
				}
			}

			return i.first->second.Id;
		}

		void Unregister(ISubsystem& subsystem) override
		{
			auto i = subSystemToDesc.find(&subsystem);
			if (i != subSystemToDesc.end())
			{
				auto id = i->second.Id;
				idToSubsystem.erase(id);
				subSystemToDesc.erase(i);

				auto erasePoint = std::remove(rootSystems.begin(), rootSystems.end(), id);
				rootSystems.erase(erasePoint);
			}
		}

		[[nodiscard]] ISubsystem* Find(ID_SUBSYSTEM id) override
		{
			auto i = idToSubsystem.find(id);
			return i != idToSubsystem.end() ? i->second : nullptr;
		}

		void ForEachSubsystem(Rococo::Function<void(ISubsystem& subsystem, ID_SUBSYSTEM id)> callback) override
		{
			for (auto& i : idToSubsystem)
			{
				callback.Invoke(*i.second, i.first);
			}
		}

		void ForEachRoot(Rococo::Function<void(ISubsystem& subsystem, ID_SUBSYSTEM id)> callback) override
		{
			for (auto id : rootSystems)
			{
				auto i = idToSubsystem.find(id);
				if (i != idToSubsystem.end())
				{
					callback.Invoke(*i->second, i->first);
				}
			}
		}

		void ForEachChild(ISubsystem& parentSubSystem, Rococo::Function<void(ISubsystem& childSubSystem, ID_SUBSYSTEM childId)> callback) override
		{
			auto i = subSystemToDesc.find(&parentSubSystem);
			if (i != subSystemToDesc.end())
			{
				for (auto childId : i->second.children)
				{
					auto child = idToSubsystem.find(childId);
					if (child != idToSubsystem.end())
					{
						callback.Invoke(*child->second, childId);
					}
				}
			}
		}
	};

	ISubsystemsSupervisor* CreateSubsystemMonitor()
	{
		return new Subsystems();
	}

	void RegisterSubsystems(ISubsystemsSupervisor& subsystems, Platform& platform)
	{
		auto& monitor = subsystems.Monitor();

		platform.graphics.renderer.RegisterSubsystem(monitor);
	}
}