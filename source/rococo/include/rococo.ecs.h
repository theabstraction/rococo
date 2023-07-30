#pragma once

#include <rococo.ecs.roid.h>

namespace Rococo
{
    struct IECS;
}

#include <..\rococo.ecs\code-gen\rococo.ecs.sxh.h>

#ifndef ROCOCO_ECS_API 
# define ROCOCO_ECS_API ROCOCO_API_IMPORT
#endif

namespace Rococo
{
    struct IECS;
}

namespace Rococo
{
    using namespace Rococo::Components;

    ROCOCO_INTERFACE IECS_ROID_LockedSection
    {
        virtual void OnLock(ROID roid, IECS& ecs) = 0;
    };

    // The (E)ntity (C)omponent (S)ystem
	ROCOCO_INTERFACE IECS: IECSBase
	{
        // Returns the number of ROIDS in use. 
        [[nodiscard]] virtual size_t ActiveRoidCount() const = 0;

        // Returns the number of ROIDS available for use. 
        [[nodiscard]] virtual size_t AvailableRoidCount() const = 0;

        // Called periodically to remove deprecated and unreferenced entities and components
        virtual void CollectGarbage() = 0;

        // Mark all ROIDs as deprecated. Best followed up by a CollectGarbage.
        virtual void DeprecateAll() = 0;

        virtual void Enumerate(IROIDCallback& cb) = 0;

        virtual bool TryLockedOperation(ROID roid, IECS_ROID_LockedSection& section) = 0;
	};
}
