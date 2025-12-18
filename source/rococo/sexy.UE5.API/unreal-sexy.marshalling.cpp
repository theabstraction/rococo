#include "unreal-sexy-marshalling.h"

#include <rococo.types.h>
#include <sexy.script.h>
#include <rococo.strings.h>
#include <rococo.hashtable.h>

#include "unreal-sexy-marshalling.resolvers.h"

typedef int UnknownType;
class UClass;
class UObject;
class UIKRigDefinition;

#include <stdio.h>
#include <vector>

namespace Rococo::UE5::Marshal
{
	struct MarshalException : IException
	{
		char msg[256] = { 0 };

		const char* Message() const override
		{
			return msg;
		}

		int ErrorCode() const override
		{
			return 0;
		}

		Debugging::IStackFrameEnumerator* StackFrames() override
		{
			return nullptr;
		}
	};

	[[noreturn]] void ThrowException(cstr format, ...)
	{
		MarshalException ex;
		va_list args;
		va_start(args, format);
		vsprintf_s(ex.msg, sizeof ex.msg, format, args);
		va_end(args);

		throw ex;
	}
}

using namespace Rococo::Sex;
using namespace Rococo::Strings;

namespace Rococo::UE
{
	namespace Native
	{
		void RegisterNatives(ISexyNativeRegistry& registry);
	}
	namespace Implementation
	{
		HString FormatSexyNamespaceFromPath(cstr path)
		{
			if (!path || *path != '/')
			{
				Throw(0, "Unexpected, path did not begin with a forward slash");
			}

			char ns[256];
			StackStringBuilder sb(ns, sizeof ns);

			cstr start = path + 1;

			while (*start != 0)
			{
				cstr next = FindChar(start, '/');
				if (next)
				{
					Substring token{ start, next };

					char buffer[256];
					token.CopyWithTruncate(buffer, sizeof buffer);

					sb << buffer;
					sb << ".";

					start = next + 1;
				}
				else
				{
					sb << start;
					break;
				}
			}

			return ns;
		}

		struct SexyNativeRegistry : ISexyNativeRegistrySupervisor, ISexyNativeRegistry
		{
			struct Entry
			{
				HString package;
				HString sexyNS;
				HString className;
				FN_AddSexyNatives_Unreal fnAddNatives;
			};

			std::vector<Entry> entries;
			stringmap<std::vector<int>> mapPackageToEntries;
			stringmap<std::vector<HString>> mapPackageToSubspaces;

			void AddNativeAPI(cstr package, cstr className, FN_AddSexyNatives_Unreal fnAddNatives) override
			{
				int index = (int) entries.size();
				entries.push_back({ package, FormatSexyNamespaceFromPath(package), className, fnAddNatives });

				auto mapping = mapPackageToEntries.insert(package, std::vector<int>()).first;
				mapping->second.push_back(index);
			}

			void LazyInit()
			{
				for (auto& m : mapPackageToEntries)
				{
					cstr package = m.first;
					auto subpaceMapping = mapPackageToSubspaces.insert(package, std::vector<HString>());

					for (auto& other : mapPackageToEntries)
					{
						cstr otherName = other.first;
						if (otherName == package)
						{
							// Same pointer, so skip.
							continue;
						}

						if (other.first.length() > m.first.length())
						{
							if (strncmp(otherName, package, m.first.length()) == 0)
							{
								// Other has the same prefix, but dont word prefix match, but namespace match
								if (otherName[m.first.length()] == '.')
								{
									subpaceMapping.first->second.push_back(otherName);
								}
							}
						}
					}
				}
			}

			void Free() override
			{
				delete this;
			}

			void Throw(const Rococo::Sex::ISExpression* referenceSrc, int referenceStartIndex, int index, cstr format, ...)
			{
				char err[512];

				va_list args;
				va_start(args, format);
				SafeVFormat(err, sizeof err, format, args);
				va_end(args);

				if (referenceSrc)
				{
					const Sex::ISExpression* src = referenceSrc;
					if (referenceStartIndex > 0 && referenceStartIndex + index < referenceSrc->NumberOfElements())
					{
						src = &referenceSrc->GetElement(referenceStartIndex + index);
					}

					Rococo::Sex::Throw(*src, "%s", err);
				}
				else
				{
					Rococo::Throw(0, "%s", err);
				}
			}

			std::vector<bool> isIndexUsed;

			void AddAllClassesInNamespace(cstr ns, Rococo::Script::IPublicScriptSystem& ss)
			{
				auto mapping = mapPackageToEntries.find(ns);
				if (mapping == mapPackageToEntries.end())
				{
					Rococo::Throw(0, "Namespace not found: %s. Expecting <namespace>.<class-filter>. E.g 'Game.TopDown.Blueprints.*'", ns);
				}

				for (auto index : mapping->second)
				{
#ifdef _DEBUG
					if (index < 0 || index >= entries.size())
					{
						Rococo::Throw(0, "Bad API. Bad index %d for %s", index, __FUNCTION__);
					}
#endif
					if (!isIndexUsed[index])
					{
						isIndexUsed[index] = true;
						auto& entry = entries[index];
						entry.fnAddNatives(ss);
					}
				}
			}

			void AddClassToNamespace(cstr ns, cstr className, Rococo::Script::IPublicScriptSystem& ss)
			{
				auto mapping = mapPackageToEntries.find(ns);
				if (mapping == mapPackageToEntries.end())
				{
					Rococo::Throw(0, "Namespace not found: %s. Expecting <namespace>.<class-filter>. E.g 'Game.TopDown.Blueprints.*'", ns);
				}


				for (auto index : mapping->second)
				{
#ifdef _DEBUG
					if (index < 0 || index >= entries.size())
					{
						Rococo::Throw(0, "Bad API. Bad index %d for %s", index, __FUNCTION__);
					}
#endif
					auto& entry = entries[index];
					if (Eq(entry.className, className))
					{
						if (!isIndexUsed[index])
						{
							isIndexUsed[index] = true;
							entry.fnAddNatives(ss);
						}
						return;
					}
				}

				// No className match
				auto subspace = mapPackageToSubspaces.find(ns);
				if (subspace == mapPackageToSubspaces.end())
				{
					Rococo::Throw(0, "Namespace %s matched but class not found: %s. Expecting <namespace>.<class-filter>. E.g 'Game.TopDown.Blueprints.*'", ns, className);
				}
				else
				{
					Rococo::Throw(0, "Namespace %s.%s matched, but expecting a trailing classname, * or **. Expecting <namespace>.<class-filter>. E.g 'Game.TopDown.Blueprints.*'", ns, className);
				}
			}

			void AddAllClassesInAllSubspacesOfNamespaceRecursive(cstr ns, Rococo::Script::IPublicScriptSystem& ss)
			{
				AddAllClassesInNamespace(ns, ss);

				auto mapping = mapPackageToSubspaces.find(ns);
				if (mapping == mapPackageToSubspaces.end())
				{
					Rococo::Throw(0, "Namespace not found: %s. Expecting <namespace>.<class-filter>. E.g 'Game.TopDown.Blueprints.*'", ns);
				}

				for (auto& subspace : mapping->second)
				{
					AddAllClassesInAllSubspacesOfNamespaceRecursive(subspace, ss);
				}
			}

			void RegisterPackagesByFilters(const Rococo::Sex::ISExpression* referenceSrc, int referenceStartIndex, cstr filters[], int numberOfFilters, Rococo::Script::IPublicScriptSystem& ss) override
			{
				/* Filters have the format
				   <Namespace>.* => select all classes in BranchN, with subspaces ignored
				   <Namespace>.** => select all classes of namespace and all subspaces of namespace (i.e recursive descent)
				   <Namespace>.<ClassName> => select class in BranchN 

				   Example:
				   Game.TopDown.Blueprints.* to select all classes in namespace Game.TopDown.Blueprints
				*/

				if (isIndexUsed.empty())
				{
					LazyInit();
					isIndexUsed.resize(entries.size());
				}

				std::fill(isIndexUsed.begin(), isIndexUsed.end(), false);

				for (int i = 0; i < numberOfFilters; i++)
				{
					cstr filter = filters[i];

					NamespaceSplitter splitter(filter);

					cstr ns, className;
					if (!splitter.SplitTail(OUT ns, OUT className))
					{
						// No namespace separator!
						Throw(referenceSrc, referenceStartIndex, i, "Bad filter #%d: %s. No namespace separator characters: '.'. Expecting <namespace>.<class-filter>. E.g 'Game.TopDown.Blueprints.*'", i + 1, filters[i]);
					}

					try
					{
						if (Eq(className, "*"))
						{
							AddAllClassesInNamespace(ns, ss);
						}
						else if (Eq(className, "**"))
						{
							AddAllClassesInAllSubspacesOfNamespaceRecursive(ns, ss);
						}
						else
						{
							AddClassToNamespace(ns, className, ss);
						}
					}
					catch (IException& ex)
					{
						Throw(referenceSrc, referenceStartIndex, i, "Bad filter #%d: %s. %s", i + 1, filters[i], ex.Message());
					}
				}
			}
		};
	}

	SEXY_MARSHALLING_API ISexyNativeRegistrySupervisor* CreateRegistryForEverything()
	{
		auto* registry = new Implementation::SexyNativeRegistry();
		Native::RegisterNatives(*registry);
		return registry;
	}
}

namespace Rococo::UE5::Marshal::Resolver
{
	[[noreturn]] void ThrowResolver(cstr filename, int lineNumber, cstr functionName)
	{
		ThrowException("Rococo::UE5::Marshal::Resolver::SetReflectionResolver must be invoked first in call to %s from %s line %d", functionName, filename, lineNumber);
	}

	struct UndefinedResolver : IResolver
	{
		UObject* ConstructObject(UObject* outer, UClass* classRef) override
		{
			UNUSED(outer);
			UNUSED(classRef);
			ThrowResolver(__FILE__, __LINE__, __FUNCTION__);
		}

		UClass* FindStaticClassRef(const TCHAR* fullPath)
		{
			UNUSED(fullPath);
			ThrowResolver(__FILE__, __LINE__, __FUNCTION__);
		}

		UFunction* FindMethod(UClass& classRef, const TCHAR* methodName)
		{
			UNUSED(classRef);
			UNUSED(methodName);
			ThrowResolver(__FILE__, __LINE__, __FUNCTION__);
		}

		void InvokeMethod(UObject* object, UFunction* methodRef, void* args)
		{
			UNUSED(object);
			UNUSED(methodRef);
			UNUSED(args);
			ThrowResolver(__FILE__, __LINE__, __FUNCTION__);
		}

		void LogMessage(EMessageLevel level, cstr msg) override
		{
			UNUSED(level);
			UNUSED(msg);
			ThrowResolver(__FILE__, __LINE__, __FUNCTION__);
		}

		UObject* GetObjectFromHandle(ObjectHandle hObject) override
		{
			UNUSED(hObject);
			ThrowResolver(__FILE__, __LINE__, __FUNCTION__);
		}

		ObjectHandle GetHandleFromObject(UObject* object) override
		{
			UNUSED(object);
			ThrowResolver(__FILE__, __LINE__, __FUNCTION__);
		}
	} s_undefinedResolver;

	IResolver* s_resolver = &s_undefinedResolver;

	SEXY_MARSHALLING_API void SetReflectionResolver(IResolver* staticResolver)
	{
		s_resolver = staticResolver;
	}

	inline UClass* FindClassByPath(crwstr fullPath)
	{
		return s_resolver->FindStaticClassRef(fullPath);
	}

	inline UFunction* FindMethod(UClass& classRef, crwstr methodName)
	{
		return s_resolver->FindMethod(classRef, methodName);
	}

	inline void InvokeMethod(UObject* object, UFunction* method, void* args)
	{
		s_resolver->InvokeMethod(object, method, args);
	}

	inline void LogMarshallingErrorDirect(cstr msg)
	{
		s_resolver->LogMessage(EMessageLevel::Error, msg);
	}

	UObject* GetObjectFromHandle(ObjectHandle hObject)
	{
		auto* object = s_resolver->GetObjectFromHandle(hObject);
		if (object == nullptr && hObject.handleIndex != 0)
		{
			ThrowException(__FUNCTION__ ": object handle was 0x%llx, but object pointer returned was not zero. Bad API", hObject.handleIndex);
		}
		return object;
	}

	inline UObject* ConstructObject(UObject* outer, UClass* classRef)
	{
		return s_resolver->ConstructObject(outer, classRef);
	}

	ObjectHandle GetHandleFromObject(UObject* object)
	{
		auto handle = s_resolver->GetHandleFromObject(object);
		if (handle.handleIndex == 0 && object != nullptr)
		{
			ThrowException(__FUNCTION__ ": <object> was not null, but method returned 0");
		}
		return handle;
	}
}

namespace Rococo::UE5::Marshal
{
	int64 ConstructUObject(Rococo::Script::NativeCallEnvironment& e)
	{
		int64 outerObjectHandle = e.ss.GetScriptContext();
		UObject* outerObject = Resolver::GetObjectFromHandle(Resolver::ObjectHandle{ outerObjectHandle });

		UClass* classRef = reinterpret_cast<UClass*>(e.context);

		UObject* newObject = Resolver::ConstructObject(outerObject, classRef);

		// The classRef in Unreal Engine can be used to construct an object. We must return a 64-bit handle to the object, ideally by unwrapping the API from TObjectPtr<UObject>
		return Resolver::GetHandleFromObject(newObject).handleIndex;
	}

	UFunction* GetNCEUMethod(Rococo::Script::NativeCallEnvironment& e)
	{
		UFunction* method = reinterpret_cast<UFunction*>(e.context);
		return method;
	}

	UObject* GetNCEUObject(Rococo::Script::NativeCallEnvironment& e, int64 objectHandle)
	{
		UNUSED(e);
		auto* object = Resolver::GetObjectFromHandle(Resolver::ObjectHandle{ objectHandle });
		return object;
	}

	void ValidateArgs(UFunction* methodRef, void* args, size_t argSize)
	{
		UNUSED(methodRef);
		UNUSED(args);
		UNUSED(argSize);
	}

	void ProcessEvent(UObject* object, UFunction* methodRef, void* args)
	{
		Resolver::InvokeMethod(object, methodRef, args);
	}

	void LogMarshallingError(cstr format, ...)
	{
		va_list args;
		va_start(args, format);
		char msg[1024];
		vsprintf_s(msg, sizeof msg, format, args);
		va_end(args);

		Resolver::LogMarshallingErrorDirect(msg);
	}

	void ScriptUFunction(Rococo::Script::IPublicScriptSystem& ss, const Rococo::Compiler::INamespace& ns, cstr implementationName, int lineNumber, UClass& classRef, Rococo::Script::FN_NATIVE_CALL nativeCall, crwstr methodName, cstr scriptSignature)
	{
		UFunction* method = Resolver::FindMethod(classRef, methodName);
		if (method == nullptr)
		{
			LogMarshallingError("No method found named '%ls' in class marshalling code defined at %s line %d", methodName, implementationName, lineNumber);
			return;
		}
		ss.AddNativeCall(ns, nativeCall, method, scriptSignature, implementationName, lineNumber);
	}

	UClass& GetStaticClassRef(crwstr fullPath)
	{
		auto* classPtr = Resolver::FindClassByPath(fullPath);
		if (!classPtr)
		{
			ThrowException("Could not resolve path into class: %ls", fullPath);
		}
		return *classPtr;
	}
}
