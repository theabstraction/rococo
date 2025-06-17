#define ROCOCO_COMPONENTS_BODY_API __declspec(dllexport)
#include <components/rococo.components.body.h>
#include <rococo.maths.h>
#include <components/rococo.ecs.builder.inl>
#include <new>

namespace Rococo::Components
{
	struct BodyComponent : IBodyComponent
	{	
        const ROID roid;
        Body::IBodyMeshDictionary& meshDictonary;

        BodyComponent(Rococo::Components::Body::BodyComponentCreationArgs& args, InstanceInfo& instance): roid(instance.roid), meshDictonary(args.meshDictionary)
        {
        }

        ComponentTypeInfo TypeInfo() const override
        {
            return ComponentTypeInfo { "BodyComponent" };
        };

        void Reflect(ComponentReflectionInfo& info) override
        {
            UNUSED(info);
        }

        Rococo::Components::Body::BodyMeshEntry mesh;

        Matrix4x4 model = Matrix4x4::Identity();
        ROID parent;
        Vec3 scale{ 1.0f, 1.0f, 1.0f };

        cr_m4x4 Model() const override
        {
            return model;
        }

        ROID Parent() const override
        {
            return parent;
        }

        Vec3 Scale() const override
        {
            return scale;
        }

        void GetScale(Vec3& scale) const override
        {
            scale = this->scale;
        }

        void GetModel(Matrix4x4& model) const override
        {
            model = this->model;;
        }

        void SetMeshByName(const fstring& name) override
        {
            if (!meshDictonary.TryGetByName(name, OUT mesh))
            {
                Throw(0, "%s: Could not find mesh %s in the mesh dictionary", __ROCOCO_FUNCTION__, name.buffer);
            }
        }

        void SetModelMatrix(cr_m4x4 model) override
        {
            this->model = model;
        }

        void SetParent(ROID parentId) override
        {
            this->parent = parentId;
        }

        void SetScale(cr_vec3 scale) override
        {
            this->scale = scale;
        }

        ID_SYS_MESH Mesh() const override
        {
            return mesh.sysId;
        }

        void SetMesh(ID_SYS_MESH id) override
        {
            mesh.sysId = id;
        }
	};
}

namespace Module::ForIBodyComponent
{
    using namespace Rococo::Components;
    IComponentFactory<IBodyComponent>* CreateComponentFactory(Rococo::Components::Body::BodyComponentCreationArgs& args)
    {
        return new FactoryWithOneArg<IBodyComponent, BodyComponent, Rococo::Components::Body::BodyComponentCreationArgs>(args);
    }
}

DEFINE_AND_EXPORT_SINGLETON_METHODS_WITH_LINKARG(ROCOCO_COMPONENTS_BODY_API, IBodyComponent, Rococo::Components::Body::BodyComponentCreationArgs&)
