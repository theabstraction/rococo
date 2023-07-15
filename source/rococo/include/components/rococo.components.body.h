#pragma once
#include <rococo.ecs.h>
#include <rococo.meshes.h>
#include <rococo.functional.h>

#ifndef ROCOCO_COMPONENTS_BODY_API
# define ROCOCO_COMPONENTS_BODY_API ROCOCO_API_IMPORT
#endif

namespace Rococo::Components
{
    ROCOCO_INTERFACE IEntity: IComponentBase
    {
        virtual cr_m4x4 Model() const = 0;
        virtual ROID Parent() const = 0;
        virtual Vec3 Scale() const = 0;
        virtual void SetModel(cr_m4x4 model) = 0;
        virtual void SetParent(ROID parent) = 0;
        virtual void SetScale(cr_vec3 scale) = 0;
    };

    ROCOCO_INTERFACE IBodyComponent : IEntity
    {
        virtual ID_SYS_MESH Mesh() const = 0;
        virtual void SetMesh(ID_SYS_MESH meshId) = 0;
    };

	ROCOCO_COMPONENTS_BODY_API Ref<IBodyComponent> AddBodyComponent(ROID id);
    ROCOCO_COMPONENTS_BODY_API void ForEachBodyComponent(IComponentCallback<IBodyComponent>& cb);
    ROCOCO_COMPONENTS_BODY_API void ForEachBodyComponent(Rococo::Function<EFlowLogic(ROID roid, IBodyComponent& component)> functor);
    ROCOCO_COMPONENTS_BODY_API Ref<IBodyComponent> GetBodyComponent(ROID id);
	ROCOCO_COMPONENTS_BODY_API void BodyComponent_LinkToECS(IECS& ecs);
}