#include "unreal-sexy-marshalling.h"

#include <rococo.types.h>
#include <sexy.script.h>
#include <Sexy.S-Parser.h>
#include <rococo.strings.h>
#include <rococo.hashtable.h>

#include "unreal-sexy-marshalling.resolvers.h"

typedef int UnknownType;
class UClass;
class UObject;
class UIKRigDefinition;

#include <stdio.h>
#include <vector>

namespace Rococo::UE::Marshal
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
using namespace Rococo::Script;

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

					for (char c : token)
					{
						sb.AppendChar(c);
					}

					sb.AppendChar('.');

					start = next + 1;
				}
				else
				{
					break;
				}
			}

			ns[sb.Length() - 1] = 0; // Eliminate the trailing dot

			if (sb.Length() >= (sizeof ns) - 1)
			{
				Throw(0, "FormatSexyNamespaceFromPath: Error truncation of %s", path);
			}

			return ns;
		}

		struct SexyNativeRegistry : ISexyNativeRegistrySupervisor, ISexyNativeRegistry
		{
			struct ClassEntry
			{
				HString package;
				HString sexyNS;
				HString className;
				FN_AddSexyNatives_Unreal fnAddNatives;
				int namespaceIndex = -1;
			};

			std::vector<ClassEntry> classes;
			stringmap<std::vector<int>> mapNamespaceToClassEntryIndices;
			stringmap<std::vector<HString>> mapNamespaceToSubspaces;

			std::vector<cstr> namespaces;

			void AddNativeAPI(cstr package, cstr className, FN_AddSexyNatives_Unreal fnAddNatives) override
			{
				int index = (int)classes.size();
				HString ns = FormatSexyNamespaceFromPath(package);
				classes.push_back({ package, ns, className, fnAddNatives });

				auto mapping = mapNamespaceToClassEntryIndices.insert(ns, std::vector<int>()).first;
				mapping->second.push_back(index);
			}

			void LazyInit()
			{
				for (auto& m : mapNamespaceToClassEntryIndices)
				{
					cstr package = m.first;
					auto subpaceMapping = mapNamespaceToSubspaces.insert(package, std::vector<HString>());

					for (auto& other : mapNamespaceToClassEntryIndices)
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
								// Other has the same prefix as first, but other is longer, so if check to see if its a subspace
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
			std::vector<bool> isNamespaceIndexUsed;

			void AddAllClassesInNamespace(cstr ns, Rococo::Script::IPublicScriptSystem& ss, IClassMatch& onMatch)
			{
				auto mapping = mapNamespaceToClassEntryIndices.find(ns);
				if (mapping == mapNamespaceToClassEntryIndices.end())
				{
					Rococo::Throw(0, "Namespace not found: %s. Expecting <namespace>.<class-filter>. E.g 'Game.TopDown.Blueprints.*'", ns);
				}

				for (auto index : mapping->second)
				{
#ifdef _DEBUG
					if (index < 0 || index >= classes.size())
					{
						Rococo::Throw(0, "Bad API. Bad index %d for %s", index, __FUNCTION__);
					}
#endif
					if (!isIndexUsed[index])
					{
						isIndexUsed[index] = true;
						auto& entry = classes[index];

						try
						{
							entry.fnAddNatives(ss);
						}
						catch (ParseException& pex)
						{
							if (pex.Source())
							{
								Rococo::Sex::Throw(*pex.Source(), "Error adding native calls for %s: %s", entry.package.c_str(), pex.Message());
							}
							else
							{
								Rococo::Throw(0, "Error adding native calls for %s: %s.\nSource %s line %d pos %d", entry.package.c_str(), pex.Message(), pex.Name(), pex.Start().y, pex.Start().x);
							}
						}
						catch (IException& ex)
						{
							Rococo::Throw(ex.ErrorCode(), "Error adding native calls for %s: %s", entry.package.c_str(), ex.Message());
						}

						char fullNs[Rococo::MAX_FQ_NAME_LEN];
						SafeFormat(fullNs, "UE.%s", ns);
						onMatch.OnMatch(fullNs, entry.className);

						if (!isNamespaceIndexUsed[entry.namespaceIndex])
						{							
							isNamespaceIndexUsed[entry.namespaceIndex] = true;
						}
					}
				}
			}

			void AddClassToNamespace(cstr ns, cstr className, Rococo::Script::IPublicScriptSystem& ss, IClassMatch& onMatch)
			{
				auto mapping = mapNamespaceToClassEntryIndices.find(ns);
				if (mapping == mapNamespaceToClassEntryIndices.end())
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
					auto& entry = classes[index];
					if (Eq(entry.className, className))
					{
						if (!isIndexUsed[index])
						{
							isIndexUsed[index] = true;
							entry.fnAddNatives(ss);
							char fullNs[Rococo::MAX_FQ_NAME_LEN];
							SafeFormat(fullNs, "UE.%s", ns);
							onMatch.OnMatch(ns, entry.className);

							if (!isNamespaceIndexUsed[entry.namespaceIndex])
							{
								isNamespaceIndexUsed[entry.namespaceIndex] = true;
							}
						}
						return;
					}
				}

				// No className match
				auto subspace = mapNamespaceToSubspaces.find(ns);
				if (subspace == mapNamespaceToSubspaces.end())
				{
					Rococo::Throw(0, "Namespace %s matched but class not found: %s. Expecting <namespace>.<class-filter>. E.g 'Game.TopDown.Blueprints.*'", ns, className);
				}
				else
				{
					Rococo::Throw(0, "Namespace %s.%s matched, but expecting a trailing classname, * or **. Expecting <namespace>.<class-filter>. E.g 'Game.TopDown.Blueprints.*'", ns, className);
				}
			}

			void AddAllClassesInAllSubspacesOfNamespaceRecursive(cstr ns, IPublicScriptSystem& ss, IClassMatch& onMatch)
			{
				AddAllClassesInNamespace(ns, ss, onMatch);

				auto mapping = mapNamespaceToSubspaces.find(ns);
				if (mapping == mapNamespaceToSubspaces.end())
				{
					Rococo::Throw(0, "Namespace not found: %s. Expecting <namespace>.<class-filter>. E.g 'Game.TopDown.Blueprints.*'", ns);
				}

				for (auto& subspace : mapping->second)
				{
					AddAllClassesInAllSubspacesOfNamespaceRecursive(subspace, ss, onMatch);
				}
			}

			void RegisterPackagesByFilters(const ISExpression* referenceSrc, int referenceStartIndex, cstr filters[], int numberOfFilters, IPublicScriptSystem& ss, IClassMatch& onMatch) override
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
					isIndexUsed.resize(classes.size());
				}

				if (isNamespaceIndexUsed.empty())
				{
					for (auto& i : mapNamespaceToClassEntryIndices)
					{
						namespaces.push_back(i.first);
						
						for (int classIndex : i.second)
						{
							classes[classIndex].namespaceIndex = (int) (namespaces.size() - 1);
						}
					}
					isNamespaceIndexUsed.resize(namespaces.size());
				}

				std::fill(isIndexUsed.begin(), isIndexUsed.end(), false);
				std::fill(isNamespaceIndexUsed.begin(), isNamespaceIndexUsed.end(), false);

				for (int i = 0; i < numberOfFilters; i++)
				{
					cstr filter = filters[i];

					NamespaceSplitter filterSplitter(filter);

					cstr nsWithUE, className;
					if (!filterSplitter.SplitTail(OUT nsWithUE, OUT className))
					{
						// No namespace separator!
						Throw(referenceSrc, referenceStartIndex, i, "Bad filter #%d: %s. No namespace separator characters: '.'. Expecting <namespace>.<class-filter>. E.g 'Game.TopDown.Blueprints.*'", i + 1, filters[i]);
					}

					NamespaceSplitter nsSplitter(nsWithUE);

					cstr ue, ns;
					if (!nsSplitter.SplitHead(ue, ns))
					{
						Rococo::Throw(0, "Expecting namespace in filter to begin with UE and have at least one subspace, but no subpace was found");
					}

					if (!Eq(ue, "UE"))
					{
						Rococo::Throw(0, "Expecting namespace in filter to begin with UE but it begain with %s", ue);
					}

					try
					{
						if (Eq(className, "*"))
						{
							AddAllClassesInNamespace(ns, ss, onMatch);
						}
						else if (Eq(className, "**"))
						{
							AddAllClassesInAllSubspacesOfNamespaceRecursive(ns, ss, onMatch);
						}
						else
						{
							AddClassToNamespace(ns, className, ss, onMatch);
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

namespace Rococo::UE::Marshal::Resolver
{
	[[noreturn]] void ThrowResolver(cstr filename, int lineNumber, cstr functionName)
	{
		ThrowException("Rococo::UE::Marshal::Resolver::SetReflectionResolver must be invoked first in call to %s from %s line %d", functionName, filename, lineNumber);
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

namespace Rococo::UE::Marshal
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
