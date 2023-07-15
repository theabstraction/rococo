#include <components/rococo.components.body.h>
#include <rococo.maths.h>
#include <new>

namespace Rococo::Components
{
	struct BodyComponent : IBodyComponent
	{	
        ID_SYS_MESH mesh;
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

        void SetModel(cr_m4x4 model) override
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
            return mesh;
        }

        void SetMesh(ID_SYS_MESH id) override
        {
            mesh = id;
        }
	};

    namespace API::ForIBodyComponent
    {
        IComponentFactory<IBodyComponent>* CreateComponentFactory()
        {
            return new DefaultFactory<IBodyComponent, BodyComponent>();
        }
    }
}