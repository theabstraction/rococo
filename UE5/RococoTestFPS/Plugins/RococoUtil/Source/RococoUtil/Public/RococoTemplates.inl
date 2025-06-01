#pragma once

template<class TARGET_CLASS>
TARGET_CLASS& LoadDefaultObjectElseThrow(const FSoftObjectPath& assetPath)
{
	UObject* obj = assetPath.ResolveObject();
	if (!obj)
	{
		obj = assetPath.TryLoad();
	}

	if (!obj)
	{
		Rococo::Throw(0, "Could not load UObject from path: %ls. FSoftObjectPath.ResolveObject & FSoftObjectPath.TryLoad returned null", *assetPath.ToString());
	}

	auto* bpClass = Cast<UBlueprintGeneratedClass>(obj);
	if (!bpClass)
	{
		Rococo::Throw(0, "Could not cast UBlueprintGeneratedClass from UObject: %ls. Object was class %ls", *assetPath.ToString(), *obj->GetClass()->GetName());
	}

	auto* defaultObject = bpClass->GetDefaultObject();
	auto* defaultInstance = Cast<TARGET_CLASS>(defaultObject);
	if (defaultInstance == nullptr)
	{
		Rococo::Throw(0, "Could not get default %ls from UObject: %ls", *TARGET_CLASS::StaticClass()->GetFName().ToString(), *assetPath.ToString());
	}

	return *defaultInstance;
}