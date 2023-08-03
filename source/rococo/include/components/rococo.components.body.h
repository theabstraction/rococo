#pragma once
#include <rococo.ecs.ex.h>
#include <rococo.meshes.h>
#include <rococo.functional.h>

#include <..\components\body\code-gen\body.sxh.h>

namespace Rococo::Components
{
    ROCOCO_INTERFACE IBodyComponent : Generated::IBodyBase
    {
        virtual ID_SYS_MESH Mesh() const = 0;
        virtual void SetMesh(ID_SYS_MESH meshId) = 0;
        virtual cr_m4x4 Model() const = 0;
        virtual Vec3 Scale() const = 0;
    };
}

#ifndef ROCOCO_COMPONENTS_BODY_API
# define ROCOCO_COMPONENTS_BODY_API ROCOCO_API_IMPORT
#endif

DECLARE_SINGLETON_METHODS(ROCOCO_COMPONENTS_BODY_API, IBodyComponent)
