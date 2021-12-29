#pragma once

// This file defines the Rococo::Script::MapImage structure
// Use of this structure requires an expert level of coding.

#include <rococo.types.h>
#include <sexy.types.h>

#include <list>
#include <vector>

#pragma pack(push, 1)

namespace Rococo::Compiler
{
    struct IStructure;
}

namespace Rococo::Script
{
    struct MapImage;
    struct IScriptSystem;

    struct MapNode;

    struct MapNodeRef
    {
        MapNode* NodePtr;
    };

    struct MapNode
    {
        MapImage* Container;
        MapNode* Previous;
        MapNode* Next;
        int32 IsExistant;
        int32 RefCount;
        int32 HashCode;

        void AddRef()
        {
            RefCount++;
        }
    };

    typedef std::list<MapNode*> TMapNodes;
    typedef std::vector<TMapNodes> TNodeRows;

    struct IKeyResolver
    {
    public:
        virtual void Delete(MapNode* node, IScriptSystem& ss) = 0;
        virtual MapNode* FindItem(VariantValue keySource, MapImage& m) = 0;
        virtual MapNode* InsertKey(VariantValue keySource, MapImage& m, IScriptSystem& ss) = 0;
    };

    struct NullResolver : public IKeyResolver
    {
        virtual void Delete(MapNode* node, IScriptSystem& ss) {}
        virtual MapNode* FindItem(VariantValue keySource, MapImage& m) { return NULL; }
        virtual MapNode* InsertKey(VariantValue keySource, MapImage& m, IScriptSystem& ss) { return NULL; }
    };

    struct MapImage
    {
        int32 NumberOfElements;
        int32 reserved;
        int64 refCount;
        const Rococo::Compiler::IStructure* KeyType;
        const Rococo::Compiler::IStructure* ValueType;
        MapNode* NullNode;
        MapNode* Head;
        MapNode* Tail;
        // These two elements will not show up in reflection, as their size is compiler/library dependent
        TNodeRows rows;
        NullResolver KeyResolver;
    };
}

#pragma pack(pop)