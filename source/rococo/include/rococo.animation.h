// Copyright (c)2025 Mark Anthony Taylor. Email: mark.anthony.taylor@gmail.com. All rights reserved.
#pragma once
#include <rococo.SI.h>
#include <rococo.animation.types.h>

namespace Rococo::Entities
{
	ROCOCO_INTERFACE ISkeleton
	{
		virtual cstr Name() const = 0;
		virtual IBone* Root() = 0;
	};

	ROCOCO_INTERFACE ISkeletons
	{
		virtual void Clear() = 0;

		/// <summary>
		/// Try to get a skeleton pointer from an unique name. 
		/// The implementation should guarantee lookup speed in
		/// contant time, typically implemented using a hashtable.  
		/// If no skeleton is found the second argument is ignored.
		/// </summary>
		/// <param name="name">- Unique name associated with the skeleton</param>
		/// <param name="ppSkeleton">- Pointer to the skeleton pointer</param>
		/// <returns>The id of the skeleton, which casts to false on failure</returns>
		[[nodiscard]] virtual ID_SKELETON GetByNameAndReturnId(cstr name, ISkeleton** ppSkeleton) = 0;

		/// <summary>
		/// Try get a skeleton pointer from an id. 
		/// This method should be very quick, quicker than calling a hashtable.
		/// Typically it uses a handle table internally. 
		/// If no skeleton is found the second argument is ignored.
		/// </summary>
		/// <param name="id">- id of the skeleton, can be retrieved with TryGet(cstr name,...)</param>
		/// <param name="ppSkeleton">- Pointer to the skeleton pointer</param>
		/// <returns>true if and only if *ppSkeleton was set to a valid skeleton pointer</returns>
		[[nodiscard]] virtual bool TryGet(ID_SKELETON id, ISkeleton** ppSkeleton) = 0;
	};

	struct AnimationAdvanceArgs
	{
		ISkeleton& puppet;
		ISkeletons& poses;
		const Seconds dt;
	};
}