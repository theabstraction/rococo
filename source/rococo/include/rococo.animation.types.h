// Copyright (c)2025 Mark Anthony Taylor. Email: mark.anthony.taylor@gmail.com. All rights reserved.
#pragma once
#include <rococo.api.h>

namespace Rococo
{
	ROCOCO_ID(ID_SKELETON, uint64, 0);
	ROCOCO_ID(ID_POSE, uint64, 0);

	enum { MAX_POSENAME_LEN = 16 };
}

namespace Rococo::Entities
{
	struct BonePath
	{
		char text[256];
	};

	ROCOCO_INTERFACE IBone
	{
		virtual cr_quat Quat() const = 0;
		virtual void SetQuat(cr_quat q) = 0;
		virtual const Matrix4x4& GetMatrix() const = 0;
		virtual void SetMatrix(const Matrix4x4& m) = 0;
		virtual cstr ShortName() const = 0;
		virtual void GetFullName(BonePath& path) = 0;
		virtual IBone* Parent() const = 0;
		virtual IBone** begin() = 0;
		virtual IBone** end() = 0;
		virtual const IBone** begin() const = 0;
		virtual const IBone** end() const = 0;
		virtual Metres Length() const = 0;
		virtual void SetLength(Metres length) = 0;
		virtual IBone* AttachBone(cr_vec3 offset, cr_quat quat, Metres length, cstr shortName) = 0;

		/*
			Detach the bone from its parent and sets the parent to null.
			After calling this function the skeleton no longer links to the bone
			and you have the responsibility of calling Free() to release the memory
		*/
		virtual void Detach() = 0;

		/* Detach this bone, then delete this bone and all its children */
		virtual void Free() = 0;
	};
}