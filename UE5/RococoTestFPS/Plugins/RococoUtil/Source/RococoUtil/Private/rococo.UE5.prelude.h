// Copyright (c)2025 Mark Anthony Taylor (mark.anthony.taylor@gmail.com). All rights reserved.
// This software is open source, but not free. Check the rococo master branch for the latest Copyright rules that applies to this software

#include "rococo.UE5.h"

#ifdef _WIN32
# pragma warning(disable: 4458) // this is my coding style and I don't see many errors arising out of the choice of style
# pragma warning(disable: 4265) // virtualized destructors are irrelevant for these classes using the Free() pattern
#else
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wshadow"
# pragma clang diagnostic ignored "-Wdelete-non-abstract-non-virtual-dtor"
# pragma clang diagnostic ignored "-Wmissing-braces"
#endif