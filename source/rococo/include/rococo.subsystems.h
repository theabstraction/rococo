#pragma once

#include <rococo.api.h>

namespace Rococo
{
	ROCOCO_INTERFACE ISubsystem
	{
		// The immutable subsystem name
		virtual [[nodiscard]] cstr SubsystemName() const = 0;

		// If the subsystem supports reflection, it will provide a reflection target here
		// Otherwise the method returns nullptr
		virtual [[nodiscard]] Reflection::IReflectionTarget* ReflectionTarget() = 0;
	};

	ROCOCO_INTERFACE ISubsystemMonitor
	{
		virtual ID_SUBSYSTEM RegisterAtRoot(ISubsystem& subsystem) = 0;
		virtual ID_SUBSYSTEM Register(ISubsystem& subsystem, ID_SUBSYSTEM parentId) = 0;
		virtual void Unregister(ISubsystem& subsystem) = 0;
	};

	ROCOCO_INTERFACE ISubsystems
	{
		virtual [[nodiscard]] ISubsystem* Find(ID_SUBSYSTEM id) = 0;
		virtual void ForEachSubsystem(Rococo::Function<void(ISubsystem & subsystem, ID_SUBSYSTEM id)> callback) = 0;
		virtual void ForEachRoot(Rococo::Function<void(ISubsystem& subsystem, ID_SUBSYSTEM id)> callback) = 0;
		virtual void ForEachChild(ISubsystem& parentSubSystem, Rococo::Function<void(ISubsystem& childSubSystem, ID_SUBSYSTEM childId)> callback) = 0;
	};

	ROCOCO_INTERFACE ISubsystemsSupervisor : ISubsystems
	{
		virtual void Free() = 0;
		virtual [[nodiscard]] ISubsystemMonitor& Monitor() = 0;
	};

	ISubsystemsSupervisor* CreateSubsystemMonitor();
}