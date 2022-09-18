#pragma once

#include <rococo.api.h>

namespace Rococo::Graphics
{
	enum class ShaderType : uint32
	{
		NONE,
		VERTEX,
		PIXEL
	};

	struct ShaderId
	{
		uint32 index : 16;
		ShaderType type : 4;
		uint32 unused : 12;
		operator uint32() { return *reinterpret_cast<uint32*>(this); }
	};

	union U64ShaderId
	{
		uint64 u64Value;
		struct
		{
			ShaderId id;
			uint32 zero;
		} uValue;
	};

	static_assert(sizeof U64ShaderId == sizeof uint64);

	inline bool operator == (ShaderId a, ShaderId b) { return (uint32)a == (uint32)b; }
	inline bool operator != (ShaderId a, ShaderId b) { return !(a == b); }

	struct ShaderView
	{
		ShaderId id;
		cstr resourceName;
		int hr;
		cstr errorString;
		const void* blob;
		size_t blobCapacity;
	};

	// N.B the shader thread is locked until OnGrab returns
	// and the contents may change, so copy what you need and do not block within the method
	// A correct implementation should generally allow an exception within the handler
	ROCOCO_INTERFACE IShaderViewGrabber
	{
		// N.B the shader thread is locked until OnGrab returns
		// and the contents may change, so copy what you need and do not block within the method
		virtual void OnGrab(const ShaderView & view) = 0;
	};

	ROCOCO_INTERFACE IShaderCompiler
	{
		// If the language supports it, add a pre-processor definition. The onus is on the caller
		// not to duplicate keys and also the caller should validate the key and value strings
		virtual void AddShaderMacro(cstr key, cstr value) = 0;
		virtual IExpandingBuffer* Compile(ShaderType type, fstring& srcCode, cstr name) = 0;
	};

	ROCOCO_INTERFACE IShaderCache
	{
		virtual	ID_PIXEL_SHADER AddPixelShader(const char* resourceName) = 0;
		virtual ID_VERTEX_SHADER AddVertexShader(const char* resourceName) = 0;
		virtual void GrabShaderObject(ID_PIXEL_SHADER pxId, IShaderViewGrabber& grabber) = 0;
		virtual void GrabShaderObject(ID_VERTEX_SHADER vxId, IShaderViewGrabber& grabber) = 0;
		virtual void GrabShaderObject(ShaderId id, IShaderViewGrabber& grabber) = 0;
		virtual void ReloadShader(const char* resourceName) = 0;
		virtual void ReloadShader(const wchar_t* sysPath) = 0;
		virtual uint32 InputQueueLength() = 0;
		virtual bool TryGrabAndPopNextError(IShaderViewGrabber& grabber) = 0;
		virtual void Free() = 0;
	};

	IShaderCache* CreateShaderCache(IShaderCompiler& compiler, IInstallation& installation);

	ROCOCO_INTERFACE ID3DShaderCompiler: IShaderCompiler
	{
		virtual void Free() = 0;
	};

	ROCOCO_INTERFACE IVulcanShaderCompiler : IShaderCompiler
	{
		virtual void Free() = 0;
	};

	namespace DirectX
	{
		ID3DShaderCompiler* CreateD3DCompiler(IInstallation& installation);
	}

	namespace GL
	{
		IVulcanShaderCompiler* CreateVulcanCompiler(IInstallation& installation);
	}
}
