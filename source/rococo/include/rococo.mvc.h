#pragma once
#include <rococo.types.h>

// Model-View-Controller
namespace Rococo::MVC
{
	ROCOCO_INTERFACE IMVC_Host
	{
		virtual void TerminateApp() = 0;
	};

	ROCOCO_INTERFACE IMVC_Controller
	{
		virtual bool IsRunning() const = 0;
	};

	ROCOCO_INTERFACE IMVC_ControllerSupervisor : IMVC_Controller
	{
		virtual void DoHousekeeping(uint64 frameIndex) = 0;
		virtual void Free() = 0;
		virtual void OnExit() = 0;
		virtual void OnInitComplete() = 0;
		virtual void TerminateOnMainWindowClose() = 0;
	};

	ROCOCO_INTERFACE IMVC_View
	{
		virtual void Cast(void** ppInterface, cstr interfaceId) = 0;
	};

	ROCOCO_INTERFACE IMVC_ViewSupervisor : IMVC_View
	{
		virtual void Free() = 0;
	};
}

#ifdef _WIN32
# define MVC_EXPORT_C_API  extern "C" __declspec(dllexport)
#endif