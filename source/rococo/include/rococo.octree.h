// Copyright (c)2025 Mark Anthony Taylor. Email: mark.anthony.taylor@gmail.com. All rights reserved.
#pragma once

namespace Rococo
{
	struct OctreeObject
	{
		Vec3 centre;
		float span;
	};

	struct OctreeCreateContext
	{
		Vec3 centre;
		float span; // The width of the square with the given centre
		float minSpan;
	};

	/// <summary>
	/// It is up to the client of the pocket data how to interpret the members and what is done with them
	/// </summary>
	struct OctreePocket
	{
		uint64 context = 0;
		uint64 flags = 0;
		const OctreeObject object;
	};

	ROCOCO_INTERFACE IOctree
	{
		virtual void Clear() = 0;
		virtual OctreePocket& Insert(const OctreeObject& object) = 0;
		virtual void EnumerateDescendants(const Vec3& point, IEventCallback<OctreePocket>& cb) = 0;
		virtual void GetApproxMemoryUse(size_t& pocketBytes, size_t& nodeBytes) const = 0;
	};

	ROCOCO_INTERFACE IOctreeSupervisor : IOctree
	{
		virtual void Free() = 0;
	};

	IOctreeSupervisor* CreateLooseOctree(OctreeCreateContext& occ);
}