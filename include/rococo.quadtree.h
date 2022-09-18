#pragma once

namespace Rococo
{
	struct QuadtreeObject
	{
		Vec2 centre;
		float span;
	};

	struct QuadtreeCreateContext
	{
		Vec2 centre;
		float span; // The width of the square with the given centre
		float minSpan;
	};

	/// <summary>
	/// It is up to the client of the pocket data how to interpret the members and what is done with them
	/// </summary>
	struct QuadtreePocket
	{
		uint64 context = 0;
		uint64 flags = 0;
		const QuadtreeObject object;
	};

	ROCOCO_INTERFACE IQuadtree
	{
		virtual void Clear() = 0;
		virtual QuadtreePocket& Insert(const QuadtreeObject& object) = 0;
		virtual void EnumerateDescendants(Vec2 point, IEventCallback<QuadtreePocket>& cb) = 0;
		virtual void GetApproxMemoryUse(size_t& pocketBytes, size_t& nodeBytes) const = 0;
	};

	ROCOCO_INTERFACE IQuadtreeSupervisor : IQuadtree
	{
		virtual void Free() = 0;
	};

	IQuadtreeSupervisor* CreateLooseQuadtree(QuadtreeCreateContext& occ);
}