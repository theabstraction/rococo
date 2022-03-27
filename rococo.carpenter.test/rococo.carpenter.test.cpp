// rococo.carpenter.test.cpp : This file contains the 'main' function. Program execution begins and ends there.

#include <rococo.types.h>
#include <..\tables\rococo.periodic-table.h>
#include <cstdio>

using namespace Rococo;
using namespace Rococo::Science::Materials;

int main()
{
    AutoFree<IPeriodicTableSupervisor> periodicTable = GetPeriodicTable();
    puts(periodicTable->Meta().GetTitle());
}

