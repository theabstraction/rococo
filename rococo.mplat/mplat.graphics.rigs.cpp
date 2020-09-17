#include <rococo.mplat.h>
#include <rococo.hashtable.h>
#include <vector>
#include <rococo.strings.h>
#include <algorithm>

namespace
{
	using namespace Rococo;
	using namespace Rococo::Graphics;

    struct NullRig : public IRigSupervisor
    {
        void AppendRigName(IStringPopulator& name) override
        {
        }

        void AppendMeshName(IStringPopulator& name) override
        {
        }

        IRig* Child(int32 index) override
        {
            Throw(0, "NullRig has no children");
        }

        boolean32 Exists() override
        {
            return false;
        }

        int32 ChildCount() override
        {
            return 0;
        }

        void ClearChildren() override
        {
        }

        void GetMatrix(Matrix4x4& output) override
        {
            output = Matrix4x4::Identity();
        }

        void SetMatrix(const Matrix4x4& input) override
        {
        }

        void Free() override
        {

        }
    } s_nullRig;

    struct Rig : public IRigSupervisor
    {
        HString rigName;
        HString meshName;
        IRig* parent;
        Matrix4x4 local;
        Vec3 scale = { 1,1,1 };

        std::vector<IRigSupervisor*> children;

        Rig(const fstring& _rigName, const fstring& _meshName, IRig* _parent, const Matrix4x4& _local):
            rigName(_rigName), meshName(_meshName), parent(_parent), local(_local)
        {
        }

        ~Rig()
        {
            ClearChildren();
        }

        Rig() = delete;
        Rig(const Rig& rg) = delete;

        void Free() override
        {
            delete this;
        }

        void AppendMeshName(IStringPopulator& pop) override
        {
            pop.Populate(meshName.c_str());
        }

        void AppendRigName(IStringPopulator& pop) override
        {
            pop.Populate(rigName.c_str());
        }

        IRig* Child(int32 index) override
        {
            return children[index];
        }

        int32 ChildCount() override
        {
            return (int32) children.size();
        }

        void ClearChildren() override
        {
            for (auto* child : children)
            {
                child->Free();
            }
            children.clear();
        }

        boolean32 Exists() override
        {
            return true;
        }

        void GetMatrix(Matrix4x4& output) override
        {
            output = local;
        }

        void SetMatrix(const Matrix4x4& input) override
        {
            local = input;
        }
    };

    struct Rigs : public IRigsSupervisor
    {
        stringmap<Rig*> rigs;

        Rigs()
        {

        }

        ~Rigs()
        {
            ClearRigs();
        }

        void Free() override
        {
            delete this;
        }

        void AddMeshToRig(const fstring& mesh, const fstring& parentRig, const Matrix4x4& local) override
        {
            if (parentRig.length < 1) Throw(0, "Rigs.AddMeshToRig(...): parentRig length was 0");
            if (mesh.length < 1) Throw(0, "Rigs.AddMeshToRig(...): mesh name length was 0");

            auto i = rigs.find((cstr)parentRig);
            if (i == rigs.end()) Throw(0, "Could not find parent rig: %s", (cstr)parentRig);

            Rig* child = new Rig(""_fstring, mesh, i->second, local);
            i->second->children.push_back(child);
        }

        IRig* AddRig(const fstring& rigName) override
        {
            if (rigName.length < 1) Throw(0, "Rigs.AddRig(...): rigName length was 0");

            auto i = rigs.find((cstr)rigName);
            if (i == rigs.end())
            {
                auto* r = new Rig(rigName, ""_fstring, nullptr, Matrix4x4::Identity());
                i = rigs.insert(rigName, r).first;
            }

            return i->second;
        }

        IRig* GetRig(const fstring& rigName) override
        {
            auto i = rigs.find((cstr)rigName);
            return i != rigs.end() ? i->second : (IRig*) &s_nullRig;
        }

        void ClearRig(const fstring& rigName) override
        {
            auto i = rigs.find((cstr)rigName);
            if (i != rigs.end())
            {
                i->second->Free();
                rigs.erase(i);
            }
        }

        void ClearRigs() override
        {
            for (auto i : rigs)
            {
                i.second->Free();
            }

            rigs.clear();
        }
    };
}

namespace Rococo
{
    namespace Graphics
    {
        IRigsSupervisor* CreateRigs()
        {
            return new Rigs();
        }
    }
}