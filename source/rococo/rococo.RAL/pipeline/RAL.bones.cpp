#include <rococo.types.h>

// #define RAL_PIPELINE_API ROCOCO_API_EXPORT

#include <rococo.renderer.types.h>
#include <rococo.renderer.h>
#include <rococo.time.h>
#include <rococo.visitors.h>
#include <RAL\RAL.h>
#include <RAL\RAL.pipeline.h>

using namespace Rococo;
using namespace Rococo::Graphics;
using namespace Rococo::RAL;

struct RALBoneStateBuffer : IRAL_BoneStateBufferSupervisor
{
	IRAL& ral;
	IRenderStates& renderStates;

	BoneMatrices boneMatrices = { 0 };

	AutoFree<IRALConstantDataBuffer> boneMatricesStateBuffer;

	RALBoneStateBuffer(IRAL& _ral, IRenderStates& _renderStates) :
		ral(_ral), renderStates(_renderStates)
	{
		boneMatricesStateBuffer = ral.CreateConstantBuffer(sizeof BoneMatrices, 1);
	}

	void Free() override
	{
		delete this;
	}

	void SyncToGPU() override
	{
		boneMatricesStateBuffer->CopyDataToBuffer(&boneMatrices, sizeof boneMatrices);
		boneMatricesStateBuffer->AssignToVS(CBUFFER_INDEX_BONE_MATRICES);
	}

	void SetBoneMatrix(uint32 index, cr_m4x4 m) override
	{
		if (index >= BoneMatrices::BONE_MATRIX_CAPACITY)
		{
			Throw(0, "Bad bone index #%u", index);
		}

		auto& target = boneMatrices.bones[index];
		target = m;
		target.row3 = Vec4{ 0, 0, 0, 1.0f };
	}
};

namespace Rococo::RAL
{
	IRAL_BoneStateBufferSupervisor* CreateRALBoneStateBuffer(IRAL& ral, IRenderStates& renderStates)
	{
		return new RALBoneStateBuffer(ral, renderStates);
	}
}