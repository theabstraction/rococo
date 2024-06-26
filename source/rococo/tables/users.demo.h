#pragma once
// Generated by rococo.carpenter. Timestamp: 01 May 2023 19:05:58// Excel Source: C:\work\rococo\tables\XL\users.demo.xlsx

#include <tables/rococo.carpenter.test.declarations.h>
#include <rococo.types.h>

Rococo::Test::UserDemo::IUsers_Sexy* FactoryConstructRococoTestUserDemoGetUserTable(Rococo::IInstallation* installation);

namespace Rococo::Test::UserDemo
{
    ROCOCO_INTERFACE IUsersRow
    {
        virtual fstring GetOwnerId() const = 0;
        virtual int64 GetPurchaseId() const = 0;
    };

    #pragma pack(push,4)
    struct UsersRowSexy
    {
        Rococo::Script::InterfacePointerToStringBuilder ownerId;
        int64 purchaseId;
    };
    #pragma pack(pop)
}

#include "rococo.carpenter.test.sxh.h"

namespace Rococo::Test::UserDemo
{
    ROCOCO_INTERFACE IUsers_MetaData
    {
        virtual fstring GetTitle() const = 0;
        virtual fstring GetOwner() const = 0;
    };

    ROCOCO_INTERFACE IUsers
    {
        virtual IUsers_Sexy& GetSexyInterface() = 0;
        virtual const IUsersRow& GetRow(int32 index) const = 0;
        virtual const int32 NumberOfRows() const = 0;
        virtual const IUsers_MetaData& Meta() const = 0;
        virtual const IUsersRow* FindRowByOwnerId(fstring ownerId, int32& index) const = 0;
        virtual const IUsersRow* FindRowByPurchaseId(int64 purchaseId, int32& index) const = 0;
    };

    ROCOCO_INTERFACE IUsersSupervisor: IUsers
    {
        virtual void Free() = 0;
    };

    IUsersSupervisor* GetUserTable(IInstallation& installation);
}
