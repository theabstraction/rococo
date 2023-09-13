#pragma once
#include <components/rococo.ecs.ex.h>
#include <rococo.meshes.h>
#include <rococo.functional.h>

#include "../../component.modules/body/code-gen/body.sxh.h"

namespace Rococo::Graphics
{
    struct IMeshBuilderSupervisor;
}

namespace Rococo::Components
{
    // provides a mesh and model and scale parameters for the mesh
    ROCOCO_INTERFACE IBodyComponent : Generated::IBodyBase
    {
        virtual ID_SYS_MESH Mesh() const = 0;
        virtual void SetMesh(ID_SYS_MESH meshId) = 0;
        virtual cr_m4x4 Model() const = 0;
        virtual Vec3 Scale() const = 0;
    };

    namespace Body
    {
        struct BodyComponentCreationArgs
        {
            Rococo::Graphics::IMeshBuilderSupervisor& meshBuilder;
        };
    }
}

#ifndef ROCOCO_COMPONENTS_BODY_API
# define ROCOCO_COMPONENTS_BODY_API ROCOCO_API_IMPORT
#endif

DECLARE_SINGLETON_METHODS_WITH_LINK_ARG(ROCOCO_COMPONENTS_BODY_API, IBodyComponent, Rococo::Components::Body::BodyComponentCreationArgs&)
