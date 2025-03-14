// Generated by gen_domme_objects.sxy on Mon Nov 25 19:05:57 2024

#include <Domme/Animals/ICat.hpp>
#include <rococo.domme.h>

using namespace Rococo;
using namespace Rococo::Domme;

namespace Rococo::Animals::Implementation
{
    struct Cat : ICatSupervisor
    {
        DommeObject D;

        Cat(ScriptingResources _scripting, cstr sourceName) : D(_scripting, sourceName, "Rococo.Animals", "ICat")
        {
            makeBiscuitsIndex = D.GetMethodIndex("MakeBiscuits", 0002, 0000);
            mewIndex = D.GetMethodIndex("Mew", 0000, 0000);
        }

        ~Cat()
        {
        }


        void SV_Free() override
        {
            delete this;
        }

        void SV_Terminate() override
        {
            D.Terminate();
        }

        int makeBiscuitsIndex = -1;

        void MakeBiscuits(int32 nBiscuits, float dt) override
        {
            REGISTER_DOMME_CALL(D);

            D.Push(nBiscuits);
            D.Push(dt);
            D.CallVirtualMethod(makeBiscuitsIndex);
            D.PopBytes(sizeof(int) + sizeof(float));

            VALIDATE_REGISTERS;
        }

        int mewIndex = -1;

        void Mew() override
        {
            REGISTER_DOMME_CALL(D);

            D.CallVirtualMethod(mewIndex);

            VALIDATE_REGISTERS;
        }
    };
}

namespace Rococo::Animals
{
    ICatSupervisor* CreateCat(ScriptingResources& scripting, cstr sourceFile)
    {
        return new Implementation::Cat(scripting, sourceFile);
    }
}

