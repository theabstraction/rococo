#pragma once

#include <rococo.types.h>

#ifndef ROCOCO_ASSETS_API
# define ROCOCO_ASSETS_API ROCOCO_API_IMPORT
#endif

namespace Rococo
{
	ROCOCO_INTERFACE IAsset
	{
		~IAsset() = default;
	};

	ROCOCO_INTERFACE IAssetLife
	{
		virtual uint32 ReferenceCount() const = 0;
		virtual const fstring Path() const = 0;
	};

	ROCOCO_INTERFACE IAssetLifeSupervisor: IAssetLife
	{
		virtual uint32 AddRef() noexcept = 0;
		virtual uint32 ReleaseRef() noexcept = 0;
	};

	// Releases a ref to a life when class instance destructs, but does not increase the reference count
	class AssetAutoRelease
	{
	private:
		IAssetLifeSupervisor& life;

	public:
		AssetAutoRelease(IAssetLifeSupervisor& _life): life(_life)
		{

		}

		~AssetAutoRelease()
		{
			life.ReleaseRef();
		}
	};

	template<class IASSET>
	class AssetRef
	{
	private:
		IASSET* asset;
		IAssetLifeSupervisor* life;

	public:
		// Default constructor, which creates an invalid Asset
		AssetRef(): asset(nullptr), life(nullptr)
		{

		}

		AssetRef(IASSET* _asset, IAssetLifeSupervisor* _life):
			asset(_asset), life(_life)
		{
			life->AddRef();
		}

		AssetRef(const AssetRef& other) :
			asset(other.asset), life(other.life)
		{
			life->AddRef();
		}

		AssetRef(const AssetRef&& other) noexcept:
			asset(other.asset), life(other.life) 
		{
			other.asset = nullptr;
			other.life = nullptr;
		}

		~AssetRef()
		{
			if (life) life->ReleaseRef();
		}

		const AssetRef& operator = (const AssetRef& src)
		{
			if (src.asset == asset)
				return src;

			if (life) life->ReleaseRef();
			asset = src.asset;
			life = src.life;
			life->AddRef();
			return *this;
		}

		static AssetRef Invalid()
		{
			return Asset();
		}

		bool operator == (const AssetRef& other) const
		{
			return asset == other.asset;
		}

		operator bool() const
		{
			return asset != nullptr;
		}

		const IASSET& Get() const
		{
			if (asset) return *asset;
			Throw(0, "%s: Asset was Invalid.", __FUNCTION__);
		}

		IASSET& Get() 
		{
			const Asset* This = this;
			const IASSET& constAsset = This->Get();
			return const_cast<IASSET&>(constAsset);
		}

		const IASSET* operator -> () const
		{
			return asset;
		}

		IASSET* operator -> ()
		{
			return asset;
		}

		const IASSET& operator * () const
		{
			return Get();
		}

		IASSET& operator * ()
		{
			return Get();
		}

		void Detach()
		{
			if (life)
			{
				life->ReleaseRef();
				asset = nullptr;
				life = nullptr;
			}
		}

		const IAssetLife& Life() const
		{
			if (life) return *life;
			Throw(0, "%s: Asset was Invalid.", __FUNCTION__);
		}
	};

	namespace Assets
	{
		ROCOCO_INTERFACE IAssetManager
		{

		};

		ROCOCO_INTERFACE IAssetManagerSupervisor : IAssetManager
		{
			virtual void Free() = 0;
		};

		ROCOCO_ASSETS_API IAssetManagerSupervisor* CreateAssetManager();
	} // Assets
} // Rococo