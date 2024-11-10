#pragma once

#include <rococo.domme.h>

namespace Rococo::Animals
{
    struct ICat
    {
        virtual void MakeBiscuits(int32 nBiscuits, float dt) = 0;
        virtual void Mew() = 0;
    };

    struct ICatSupervisor: ICat
    {
        virtual void _Free() = 0;
        virtual void _Terminate() = 0;
    };

    ICatSupervisor* CreateCat(Rococo::Domme::ScriptingResources& scripting, cstr sourceFile);
}

DECLARE_DOMME_INTERFACE(Rococo::Animals::ICat)

