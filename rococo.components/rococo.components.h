#pragma once
#include <rococo.types.h>

// Example components API

#include <vector>

namespace Rococo::Components
{
	struct ENTITY_ID
	{
		int64 index;
	};

	struct IEntity;

	class SharedEntityRef
	{
	private:
		IEntity* entity;

	public:
		IEntity* operator -> () { return entity; }
	};

	struct IEntities
	{
		virtual void DeprecateEntity(ENTITY_ID id) = 0;
		virtual SharedEntityRef CreateEntity() = 0;
		virtual SharedEntityRef FindEntity(ENTITY_ID id) = 0;
	};

	struct IRefCounted
	{
		virtual int64 AddRef() = 0;
		virtual int64 Release() = 0;
	};

	struct IEntity: IRefCounted
	{
		virtual ENTITY_ID Id() const = 0;
		virtual IEntities& Entities() const = 0;
	};

	struct IParticleCloud
	{

	};

	class ParticleCloud: IParticleCloud
	{
		
	};

	struct IParticleSystemComponent
	{
		virtual IParticleCloud& Add() = 0;
		virtual IEntity& Entity() = 0;
	};

	IParticleSystemComponent& FindParticleSystemComponent(IEntity& entity);
	IParticleSystemComponent& CreateParticleSystemComponent(IEntity& entity);
	void DeleteParticleSystemComponent(IEntity& entity);
	size_t SizeOfParticleSystemComponent();

	/*
		MakeComponent(ParticleCloud, IParticleCloud)
	
	
	*/
}
