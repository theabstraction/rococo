// Copyright (c)2025 Mark Anthony Taylor. Email: mark.anthony.taylor@gmail.com. All rights reserved.
#pragma once

#ifdef _WIN32
# include <malloc.h>
# include <excpt.h>
# define TRY_PROTECTED __try
# define CATCH_PROTECTED __except (EXCEPTION_EXECUTE_HANDLER)
#else
# include <CoreServices/CoreServices.h>
# include <mach/mach.h>
# include <mach/mach_time.h>
# define TRY_PROTECTED try
# define CATCH_PROTECTED catch (SignalException& sigEx)
namespace
{
    typedef void(*FN_SignalHandler)(int);

    struct SignalException
    {
        int signalCode;
    };

    struct ThrowOnSignal
    {
        FN_SignalHandler previousHandler;
        int code;

        static void OnSignal(int code)
        {
            throw SignalException{ code };
        }

        ThrowOnSignal(int code)
        {
            this->previousHandler = signal(code, ThrowOnSignal::OnSignal);
            this->code = code;
        }

        ~ThrowOnSignal()
        {
            signal(code, previousHandler);
        }
    };
}
#endif