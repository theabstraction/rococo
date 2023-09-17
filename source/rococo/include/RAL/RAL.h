#pragma once

#include <rococo.renderer.types.h>

namespace Rococo
{
	struct ID_SYS_MESH;
	struct ID_CUBE_TEXTURE;
}

namespace Rococo::Graphics
{
	struct IMeshes;
	struct IRenderContext;
	struct IShaders;
	struct ITextureManager;
}

namespace Rococo::RAL
{
	ROCOCO_INTERFACE IRALDataBuffer
	{
		virtual void CopyDataToBuffer(const void* data, size_t sizeofData) = 0;
		virtual void Free() = 0;
	};

	ROCOCO_INTERFACE IRALVertexDataBuffer : IRALDataBuffer
	{
	};

	ROCOCO_INTERFACE IRALConstantDataBuffer : IRALDataBuffer
	{
		virtual void AssignToGS(int32 constantBufferIndex) = 0;
		virtual void AssignToPS(int32 constantBufferIndex) = 0;
		virtual void AssignToVS(int32 constantBufferIndex) = 0;
	};

	// N.B when RAL is implemented by DX11, the numertical cast of PrimitiveTopology values match D3D_PRIMITIVE_TOPOLOGY exactly
	enum class PrimitiveTopology: uint32
	{
		UNDEFINED = 0,
		POINTLIST = 1,
		LINELIST = 2,
		LINESTRIP = 3,
		TRIANGLELIST = 4,
		TRIANGLESTRIP = 5,
		LINELIST_ADJ = 10,
		LINESTRIP_ADJ = 11,
		TRIANGLELIST_ADJ = 12,
		TRIANGLESTRIP_ADJ = 13,
		_1_CONTROL_POINT_PATCHLIST = 33,
		_2_CONTROL_POINT_PATCHLIST = 34,
		_3_CONTROL_POINT_PATCHLIST = 35,
		_4_CONTROL_POINT_PATCHLIST = 36,
		_5_CONTROL_POINT_PATCHLIST = 37,
		_6_CONTROL_POINT_PATCHLIST = 38,
		_7_CONTROL_POINT_PATCHLIST = 39,
		_8_CONTROL_POINT_PATCHLIST = 40,
		_9_CONTROL_POINT_PATCHLIST = 41,
		_10_CONTROL_POINT_PATCHLIST = 42,
		_11_CONTROL_POINT_PATCHLIST = 43,
		_12_CONTROL_POINT_PATCHLIST = 44,
		_13_CONTROL_POINT_PATCHLIST = 45,
		_14_CONTROL_POINT_PATCHLIST = 46,
		_15_CONTROL_POINT_PATCHLIST = 47,
		_16_CONTROL_POINT_PATCHLIST = 48,
		_17_CONTROL_POINT_PATCHLIST = 49,
		_18_CONTROL_POINT_PATCHLIST = 50,
		_19_CONTROL_POINT_PATCHLIST = 51,
		_20_CONTROL_POINT_PATCHLIST = 52,
		_21_CONTROL_POINT_PATCHLIST = 53,
		_22_CONTROL_POINT_PATCHLIST = 54,
		_23_CONTROL_POINT_PATCHLIST = 55,
		_24_CONTROL_POINT_PATCHLIST = 56,
		_25_CONTROL_POINT_PATCHLIST = 57,
		_26_CONTROL_POINT_PATCHLIST = 58,
		_27_CONTROL_POINT_PATCHLIST = 59,
		_28_CONTROL_POINT_PATCHLIST = 60,
		_29_CONTROL_POINT_PATCHLIST = 61,
		_30_CONTROL_POINT_PATCHLIST = 62,
		_31_CONTROL_POINT_PATCHLIST = 63,
		_32_CONTROL_POINT_PATCHLIST = 64
	};

	struct RALMeshBuffer
	{
		IRALVertexDataBuffer* vertexBuffer;
		IRALVertexDataBuffer* weightsBuffer;
		uint32 numberOfVertices;
		PrimitiveTopology topology;
		ID_PIXEL_SHADER psSpotlightShader;
		ID_PIXEL_SHADER psAmbientShader;
		ID_VERTEX_SHADER vsSpotlightShader;
		ID_VERTEX_SHADER vsAmbientShader;
		bool alphaBlending;
		bool disableShadowCasting;
	};

	// [R]enderer [A]bstraction [L]ayer, provided to the RAL pipeline implementation 
	ROCOCO_INTERFACE IRAL
	{
		virtual IRALConstantDataBuffer* CreateConstantBuffer(size_t sizeofStruct, size_t nElements) = 0;
		virtual IRALVertexDataBuffer* CreateDynamicVertexBuffer(size_t sizeofStruct, size_t nElements) = 0;
		virtual void ClearBoundVertexBufferArray() = 0;
		virtual void BindVertexBuffer(ID_SYS_MESH meshId, size_t sizeofVertex, uint32 offset) = 0;
		virtual void BindVertexBuffer(IRALVertexDataBuffer* vertexBuffer, size_t sizeofVertex, uint32 offset) = 0;
		virtual void CommitBoundVertexBuffers() = 0;
		virtual void Draw(uint32 nVertices, uint32 startPosition) = 0;
		virtual ID_TEXTURE GetWindowDepthBufferId() const = 0;
		virtual void SetEnvironmentMap(ID_CUBE_TEXTURE txId) = 0;
		virtual void ExpandViewportToEntireTexture(ID_TEXTURE depthTarget) = 0;
		virtual Rococo::Graphics::IMeshes& Meshes() = 0;
		virtual Rococo::Graphics::IShaders& Shaders() = 0;
		virtual Rococo::Graphics::ITextureManager& RALTextures() = 0;
		virtual Rococo::Graphics::IRenderContext& RenderContext() = 0;
	};
}