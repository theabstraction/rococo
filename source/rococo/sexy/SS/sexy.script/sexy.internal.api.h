#pragma once

#include <rococo.types.h>

namespace Rococo::Compiler
{
	DECLARE_ROCOCO_INTERFACE IStructure;
}

namespace Rococo::Script
{
	struct ArrayImage;
	struct MapImage;

	DECLARE_ROCOCO_INTERFACE IScriptSystem;

	void AlignedMemcpy(void* __restrict dest, const void* __restrict source, size_t nBytes);
	void DestroyElements(ArrayImage& a, IScriptSystem& ss);
	void DestroyObject(const Compiler::IStructure& type, uint8* item, IScriptSystem& ss);
	void IncrementRef(MapImage* mapImage);
	void ListRelease(ListImage* l, IScriptSystem& ss);
}