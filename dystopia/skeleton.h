#ifndef DYSTOPIA_SKELETON
#define DYSTOPIA_SKELETON

namespace Dystopia
{
	ROCOCOAPI ISkeleton
	{
		// TODO - virtual void SetNextAnimation(AnimationType type) = 0;
		virtual void SetCurrentAnimation(AnimationType type) = 0;
	};

	ROCOCOAPI ISkeletonSupervisor
	{
		virtual void Free() = 0;
		virtual void Render(IRenderContext& rc, const ObjectInstance& instance, float gameTime, float dt) = 0;
	};

	ISkeletonSupervisor* CreateSkeleton(Environment& e);

	ROCOCOAPI IBoneLibrary
	{
		
	};

	ROCOCOAPI IBoneLibrarySupervisor: public IBoneLibrary
	{
		virtual void Free() = 0;
		virtual void Reload(const wchar_t* libraryName) = 0;
	};

	IBoneLibrarySupervisor* CreateBoneLibrary(IInstallation& installation, IRenderer& renderer, ISourceCache& sources);
}

#endif
