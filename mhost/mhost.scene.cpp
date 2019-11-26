#include "mhost.h"
#include <rococo.mplat.h>
#include <rococo.renderer.h>
#include <rococo.strings.h>

#include <vector>

#include <rococo.textures.h>

#include <rococo.ui.h>

#include <rococo.maths.h>

using namespace MHost;

struct CameraObject: public ICamera
{
	Matrix4x4 cameraToWorld;
	Matrix4x4 worldToCamera;
	Matrix4x4 cameraToScreen;

	Metres near = 0.1_metres;
	Metres far = 100_metres;
	Degrees fov = 90_degrees;

	float aspectRatio = 1.0f;

	CameraObject()
	{
		cameraToWorld = Matrix4x4::Identity();
		worldToCamera = Matrix4x4::Identity();
		SyncCameraToScreen();
	}

	void SyncCameraToScreen()
	{
		cameraToScreen = Matrix4x4::GetRHProjectionMatrix(fov, aspectRatio, near, far);
	}

	void SetAspectRatio(float aspectRatio) override
	{
		this->aspectRatio = aspectRatio;
		SyncCameraToScreen();
	}

	void SetFieldOfView(Degrees fov) override
	{
		this->fov = fov;
		SyncCameraToScreen();
	}

	void SetPosition(const Vec3& position) override
	{
		cameraToWorld.row0.x = position.x;
		cameraToWorld.row0.y = position.y;
		cameraToWorld.row0.z = position.z;

		worldToCamera.row0.x = -position.x;
		worldToCamera.row0.y = -position.y;
		worldToCamera.row0.z = -position.z;
	}

	void SetOrientation(const MHost::WorldOrientation& wo) override
	{
	}

	void SetPerspective(float near, float far) override
	{
		this->near = Metres{ near };
		this->far = Metres{ far };
		SyncCameraToScreen();
	}

	void GetWorldToCamera(Matrix4x4& m) override
	{
		m = worldToCamera;
	}

	void GetCameraToScreen(Matrix4x4& m)  override
	{
		m = cameraToScreen;
	}
};

struct SceneBuilder : public ISceneBuilderSupervisor
{
	Platform& platform;
	CameraObject camera;
	ID_TEXTURE skyboxId;

	SceneBuilder(Platform& _platform): platform(_platform)
	{

	}

	MHost::ICamera* Camera() override
	{
		return &camera;
	}

	IdTexture CreateCubeTexture(const fstring& path, const fstring& extension) override
	{
		return IdTexture{ platform.renderer.CreateCubeTexture(path, extension).value };
	}

	void SetSkyBox(IdTexture cubeId) override
	{
		skyboxId = ID_TEXTURE{ cubeId.value };
	}

	ID_TEXTURE GetSkyBoxCubeId() const override
	{
		return skyboxId;
	}

	void Free() override
	{
		delete this;
	}
};

namespace MHost
{
	ISceneBuilderSupervisor* CreateSceneBuilder(Platform& platform)
	{
		return new SceneBuilder(platform);
	}
}