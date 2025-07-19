#include "ReflectedGameOptionsBuilder.h"

#include <rococo.great.sex.h>
#include <rococo.game.options.h>

DECLARE_LOG_CATEGORY_EXTERN(LogReflectedBuilder, Log, All);
DEFINE_LOG_CATEGORY(LogReflectedBuilder);

using namespace Rococo::Game::Options;

namespace Rococo::GreatSex
{
	namespace Implementation
	{
		struct ReflectedGameOptions : IGameOptions, IOptionDatabase
		{
			TObjectPtr<UObject> optionObject;

			ReflectedGameOptions(UObject* object): optionObject(object)
			{

			}

			ReflectedGameOptions(const ReflectedGameOptions& src)
			{
				optionObject = src.optionObject;
			}

			void AddOptions(IGameOptionsBuilder& builder) override
			{

			}

			IOptionDatabase& DB() override
			{
				return *this;
			}

			void Accept() override
			{

			}

			void Revert() override
			{

			}

			bool IsModified() const override
			{
				return false;
			}

			void Invoke(cstr name, cstr choice) override
			{

			}

			void Invoke(cstr name, bool boolValue) override
			{

			}

			void Invoke(cstr name, double scalarValue) override
			{

			}
		};

		struct ReflectedGameOptionsBuilder : IReflectedGameOptionsBuilder
		{
			TMap<UObject*, ReflectedGameOptions> mapObjectToOptions;

			ReflectedGameOptionsBuilder()
			{

			}

			virtual ~ReflectedGameOptionsBuilder()
			{

			}

			void ReflectIntoGenerator(UObject& object, Rococo::GreatSex::IGreatSexGenerator& generator) override
			{
				UClass* objectClass = object.GetClass();

				UE_LOG(LogReflectedBuilder, Display, TEXT("Reflecting %s"), *objectClass->GetFName().ToString());

				for (TFieldIterator<UFunction> i(objectClass); i; ++i)
				{
					UFunction* method = *i;
					FString methodName = *method->GetFName().ToString();

					UE_LOG(LogReflectedBuilder, Display, TEXT("method %s"), *methodName);

					if (methodName.StartsWith(TEXT("AddChoice_")))
					{
						
					}
				}

				ReflectedGameOptions options(&object);
				auto& refOptions = mapObjectToOptions.Add(&object, options);
				generator.AddOptions(refOptions, "didhum");;
			}

			void AddMethods(UObject& object) override
			{

			}

			void Free() override
			{
				delete this;
			}
		};
	}

	ROCOCOGUI_API IReflectedGameOptionsBuilder* CreateReflectedGameOptionsBuilder()
	{
		return new Implementation::ReflectedGameOptionsBuilder();
	}
}