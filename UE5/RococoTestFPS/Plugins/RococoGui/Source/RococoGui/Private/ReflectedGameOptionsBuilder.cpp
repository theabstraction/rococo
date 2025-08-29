#include "ReflectedGameOptionsBuilder.h"

#include <rococo.great.sex.h>
#include <rococo.game.options.h>
#include <GameOptionBuilder.h>

DECLARE_LOG_CATEGORY_EXTERN(LogReflectedBuilder, Log, All);
DEFINE_LOG_CATEGORY(LogReflectedBuilder);

using namespace Rococo::Game::Options;

namespace Rococo::GreatSex
{
	namespace Implementation
	{
		// Note that option methods have the signature AddOption_<OPTION-NAME> -> (<TYPE> currentValue)(<SPEC> currentSpec)
		// We support <TYPE> where <TYPE> is one of bool, double, or FString, the spec is coupled to the <TYPE>
		static const FString addOptionPrefix = FString(TEXT("AddOption_"));
		static const FString onChoicePrefix = FString(TEXT("OnChoice_"));
		static const FString onScalarPrefix = FString(TEXT("OnScalar_"));
		static const FString onBoolPrefix = FString(TEXT("OnBool_"));

		void ToAscii(char* buffer, size_t capacity, const FString& s)
		{
			if (buffer == nullptr || capacity == 0)
			{
				return;
			}

			if (s.Len() >= capacity)
			{
				snprintf(buffer, capacity, "<truncated!>");
				return;
			}

			int i = 0;
			for (; i < s.Len(); i++)
			{
				buffer[i] = (char)s[i];
			}

			buffer[i] = 0;
		}

		struct GameOptionMethodDesc
		{
			UFunction* addFuction = nullptr;
			UFunction* handlerFunction = nullptr;
		};

		struct ReflectedGameOptions : IGameOptions, IOptionDatabase
		{
			TObjectPtr<UObject> optionObject;
			TArray<UFunction*> addMethods;
			TArray<UFunction*> handlerMethods;
			TArray<char> asciiBuffer;

			ReflectedGameOptions(UObject& object) : optionObject(&object)
			{

			}

			ReflectedGameOptions(const ReflectedGameOptions& src)
			{
				optionObject = src.optionObject;
			}

			virtual ~ReflectedGameOptions()
			{

			}

			void OnError(UFunction* method, FProperty* property, FString msg)
			{
				if (optionObject->Implements<URococoGameOptionBuilder>())
				{
					auto builder = TScriptInterface<IRococoGameOptionBuilder>(optionObject);
					builder->Execute_OnError(optionObject, method ? method->GetFName().ToString() : TEXT("<no method>"), property ? property->GetFName().ToString() : TEXT(""), msg);
				}
			}

			void AddAddChoiceMethodElseLogError(UFunction* method)
			{
				int index = 0;

				for (TFieldIterator<FProperty> i(method, EFieldIteratorFlags::ExcludeSuper); i; ++i, ++index)
				{
					FProperty* property = *i;

					if (!property->HasAnyPropertyFlags(CPF_Parm))
					{
						// Not a parameter, probably a local variable
						continue;
					}

					if (!property->HasAnyPropertyFlags(CPF_OutParm))
					{
						OnError(method, property, TEXT("Expecting no inputs"));
					}

					if (index == 0)
					{
						auto* sp = CastField<FStrProperty>(property);
						if (!sp)
						{
							// Signature mismatch
							OnError(method, property, TEXT("Expecting first method argument to be of type FString"));
							return;
						}
					}
					else if (index == 1)
					{
						auto* sp = CastField<FStructProperty>(property);
						if (!sp)
						{
							OnError(method, property, TEXT("Expecting second method argument to be a struct of type FRococoGameOptionChoice"));
						}

						if (sp->Struct != FRococoGameOptionChoice::StaticStruct())
						{
							// Signature mismatch
							OnError(method, property, TEXT("Expecting first method argument to be of type FRococoGameOptionChoice"));
							return;
						}
					}
					else
					{
						OnError(method, property, TEXT("Expecting only two method arguments"));
						return;
					}
				}

				addMethods.Add(method);
			}

			void AddAddBoolMethodElseLogError(UFunction* method)
			{
				int index = 0;

				for (TFieldIterator<FProperty> i(method, EFieldIteratorFlags::ExcludeSuper); i; ++i)
				{
					FProperty* property = *i;

					if (!property->HasAnyPropertyFlags(CPF_Parm))
					{
						// Not a parameter, probably a local variable
						continue;
					}

					if (!property->HasAnyPropertyFlags(CPF_OutParm))
					{
						OnError(method, property, TEXT("Expecting no method input and two outputs, bool and FRococoGameOptionBool by value"));
						return;
					}

					index++;

					if (index == 1)
					{
						auto* sp = CastField<FBoolProperty>(property);
						if (!sp)
						{
							// Signature mismatch
							OnError(method, property, TEXT("Expecting first method output argument to be of type bool"));
							return;
						}
					}
					else if (index == 2)
					{
						auto* sp = CastField<FStructProperty>(property);
						if (!sp)
						{
							OnError(method, property, TEXT("Expecting second method output argument to be a struct of type FRococoGameOptionBool"));
						}

						if (sp->Struct != FRococoGameOptionBool::StaticStruct())
						{
							// Signature mismatch
							OnError(method, property, TEXT("Expecting second method argument to be of type FRococoGameOptionBool"));
							return;
						}
					}
					else
					{
						OnError(method, property, TEXT("Expecting only two method arguments"));
						return;
					}
				}

				if (index != 2)
				{
					OnError(method, nullptr, TEXT("Expecting no method input and two outputs, bool and FRococoGameOptionBool by value"));
					return;
				}

				addMethods.Add(method);
			}

			void AddAddScalarMethodElseLogError(UFunction* method)
			{
				int index = 0;

				for (TFieldIterator<FProperty> i(method, EFieldIteratorFlags::ExcludeSuper); i; ++i)
				{
					FProperty* property = *i;

					if (!property->HasAnyPropertyFlags(CPF_Parm))
					{
						// Not a parameter, probably a local variable
						continue;
					}

					if (!property->HasAnyPropertyFlags(CPF_OutParm))
					{
						OnError(method, property, TEXT("Expecting no inputs"));
					}

					if (index == 0)
					{
						index++;

						auto* sp = CastField<FDoubleProperty>(property);
						if (!sp)
						{
							// Signature mismatch
							OnError(method, property, TEXT("Expecting first method argument to be of type double"));
							return;
						}
					}
					else if (index == 1)
					{
						index++;

						auto* sp = CastField<FStructProperty>(property);
						if (!sp)
						{
							OnError(method, property, TEXT("Expecting second method argument to be a struct of type FRococoGameOptionScalar"));
						}

						if (sp->Struct != FRococoGameOptionScalar::StaticStruct())
						{
							// Signature mismatch
							OnError(method, property, TEXT("Expecting first method argument to be of type FRococoGameOptionScalar"));
							return;
						}
					}
					else
					{
						OnError(method, property, TEXT("Expecting only two method arguments"));
						return;
					}
				}

				if (index != 2)
				{
					OnError(method, nullptr, TEXT("Expecting no method input and two outputs, double and FRococoGameOptionScalar by value"));
					return;
				}

				addMethods.Add(method);
			}

			void AddOnChoiceMethodElseLogError(UFunction* method)
			{
				int index = 0;

				for (TFieldIterator<FProperty> i(method, EFieldIteratorFlags::ExcludeSuper); i; ++i)
				{
					FProperty* property = *i;

					if (!property->HasAnyPropertyFlags(CPF_Parm))
					{
						// Not a parameter, probably a local variable
						continue;
					}

					if (property->HasAnyPropertyFlags(CPF_OutParm))
					{
						OnError(method, property, TEXT("Expecting no outputs. Input but be FString by value."));
						return;
					}

					if (index++ == 0)
					{
						auto* sp = CastField<FStrProperty>(property);
						if (!sp)
						{
							// Signature mismatch
							OnError(method, property, TEXT("Expecting first method argument to be of type FString"));
							return;
						}
					}
					else
					{
						OnError(method, property, TEXT("Expecting only one method argument. FString by value"));
						return;
					}
				}

				if (index != 1)
				{
					OnError(method, nullptr, TEXT("Expecting one input argument of type FString"));
					return;
				}

				handlerMethods.Add(method);
			}

			void AddOnBoolMethodElseLogError(UFunction* method)
			{
				int index = 0;

				for (TFieldIterator<FProperty> i(method, EFieldIteratorFlags::ExcludeSuper); i; ++i)
				{
					FProperty* property = *i;

					if (!property->HasAnyPropertyFlags(CPF_Parm))
					{
						// Not a parameter, probably a local variable
						continue;
					}

					if (property->HasAnyPropertyFlags(CPF_OutParm))
					{
						OnError(method, property, TEXT("Expecting no outputs"));
						return;
					}

					if (index++ == 0)
					{
						auto* sp = CastField<FBoolProperty>(property);
						if (!sp)
						{
							// Signature mismatch
							OnError(method, property, TEXT("Expecting first method argument to be of type bool"));
							return;
						}
					}
					else
					{
						OnError(method, property, TEXT("Expecting only one method argument"));
						return;
					}
				}

				if (index != 1)
				{
					OnError(method, nullptr, TEXT("Expecting one input argument of type bool"));
					return;
				}

				handlerMethods.Add(method);
			}

			FProperty* GetMethodArgument(UFunction* method, int requiredIndex)
			{
				int index = 0;

				for (TFieldIterator<FProperty> i(method, EFieldIteratorFlags::ExcludeSuper); i; ++i)
				{
					FProperty* property = *i;

					if (!property->HasAnyPropertyFlags(CPF_Parm))
					{
						// Not a parameter, probably a local variable
						continue;
					}

					if (index == requiredIndex)
					{
						return property;
					}

					index++;
				}

				return nullptr;
			}
			/*
					if (property->HasAnyPropertyFlags(CPF_OutParm))
					{
						OnError(method, property, TEXT("Expecting no outputs"));
						return;
					}

					if (index++ == 0)
					{
						auto* sp = CastField<FDoubleProperty>(property);
						if (!sp)
						{
							// Signature mismatch
							OnError(method, property, TEXT("Expecting first method argument to be of type double"));
							return;
						}
					}
					else
					{
						OnError(method, property, TEXT("Expecting only one method argument"));
						return;
					}
			}

			*/

			void AddOnScalarMethodElseLogError(UFunction* method)
			{
				int index = 0;

				for (TFieldIterator<FProperty> i(method, EFieldIteratorFlags::ExcludeSuper); i; ++i)
				{
					FProperty* property = *i;

					if (!property->HasAnyPropertyFlags(CPF_Parm))
					{
						// Not a parameter, probably a local variable
						continue;
					}

					if (property->HasAnyPropertyFlags(CPF_OutParm))
					{
						OnError(method, property, TEXT("Expecting no outputs"));
						return;
					}

					if (index++ == 0)
					{
						auto* sp = CastField<FDoubleProperty>(property);
						if (!sp)
						{
							// Signature mismatch
							OnError(method, property, TEXT("Expecting first method argument to be of type double"));
							return;
						}
					}
					else
					{
						OnError(method, property, TEXT("Expecting only one method argument"));
						return;
					}
				}

				if (index != 1)
				{
					OnError(method, nullptr, TEXT("Expecting one input argument of type double"));
					return;
				}

				handlerMethods.Add(method);
			}

			void AddAddOptionMethodElseLogError(UFunction* method)
			{
				FProperty* argType = GetMethodArgument(method, 0);
				auto* doubleArg = CastField<FDoubleProperty>(argType);
				if (doubleArg)
				{
					AddAddScalarMethodElseLogError(method);
					return;
				}

				auto* boolArg = CastField<FBoolProperty>(argType);
				if (boolArg)
				{
					AddAddBoolMethodElseLogError(method);
					return;
				}

				auto* choiceArg = CastField<FStrProperty>(argType);
				if (choiceArg)
				{
					AddAddChoiceMethodElseLogError(method);
					return;
				}

				OnError(method, choiceArg, TEXT("Expecting first argument to be of type double, bool or string"));
			}

			void AddMethod(UFunction* method)
			{
				FString methodName = method->GetFName().ToString();

				FString knownPrefices[] = { onBoolPrefix, onChoicePrefix, onScalarPrefix, addOptionPrefix };

				bool wasMatched = false;

				for (auto& prefix : knownPrefices)
				{
					if (methodName.StartsWith(prefix))
					{
						if (method->HasAnyFunctionFlags(EFunctionFlags::FUNC_Static))
						{
							OnError(method, nullptr, TEXT("Expecting method to NOT be static"));
							return;
						}

						UE_LOG(LogReflectedBuilder, Display, TEXT("option method: %s"), *methodName);

						if (methodName.Len() > asciiBuffer.Num())
						{
							asciiBuffer.SetNum(methodName.Len());
						}

						wasMatched = true;
						break;
					}
				}

				if (!wasMatched)
				{
					return;
				}

				if (methodName.StartsWith(onBoolPrefix))
				{
					AddOnBoolMethodElseLogError(method);
					return;
				}

				if (methodName.StartsWith(onChoicePrefix))
				{
					AddOnChoiceMethodElseLogError(method);
					return;
				}

				if (methodName.StartsWith(onScalarPrefix))
				{
					AddOnScalarMethodElseLogError(method);
					return;
				}

				if (methodName.StartsWith(addOptionPrefix))
				{
					AddAddOptionMethodElseLogError(method);
					return;
				}

				OnError(method, nullptr, TEXT("knownPrefices not matched to handler. Program bug."));
			}

			const char* GetVolatileAsciiTrailingString(const FString& s, const FString& prefix)
			{
				if (!s.StartsWith(prefix))
				{
					return nullptr;
				}

				int j = 0;

				for (int i = prefix.Len(); i < s.Len(); i++, j++)
				{
					asciiBuffer[j] = (char)s[i];
				}

				asciiBuffer[j] = 0;
				return asciiBuffer.GetData();
			}

			void AddChoiceOption(UFunction* method, const char* choiceId, IGameOptionsBuilder& builder)
			{
				struct FRococoGameOptionChoicePackage
				{
					FString currentChoice;
					FRococoGameOptionChoice spec;
				};

				FRococoGameOptionChoicePackage args;

				optionObject->ProcessEvent(method, &args);

				auto& c = builder.AddChoice(choiceId);

				for (auto& item : args.spec.Items)
				{
					char choiceName[256];
					ToAscii(choiceName, sizeof choiceName, item.Id);

					char choiceHint[256];
					ToAscii(choiceHint, sizeof choiceHint, item.Hint);

					char choiceText[128];
					ToAscii(choiceText, sizeof choiceText, item.Text);

					c.AddChoice(choiceName, choiceText, choiceHint);
				}

				char currentChoice[128];
				ToAscii(currentChoice, sizeof currentChoice, args.currentChoice);

				c.SetActiveChoice(currentChoice);

				char choiceHint[256];
				ToAscii(choiceHint, sizeof choiceHint, args.spec.Hint);
				c.SetHint(choiceHint);

				char choiceTitle[256];
				ToAscii(choiceTitle, sizeof choiceTitle, args.spec.Title);

				c.SetTitle(choiceTitle);
			}

			void AddBoolOption(UFunction* method, const char* boolId, IGameOptionsBuilder& builder)
			{
				struct FRococoGameOptionBoolPackage
				{
					uint32 currentBool;
					FRococoGameOptionBool spec;
				};

				FRococoGameOptionBoolPackage args;

				optionObject->ProcessEvent(method, &args);

				auto& b = builder.AddBool(boolId);

				b.SetActiveValue(args.currentBool == 0 ? false : true);

				char hint[256];
				ToAscii(hint, sizeof hint, args.spec.Hint);

				b.SetHint(hint);

				char title[256];
				ToAscii(title, sizeof title, args.spec.Title);

				b.SetTitle(title);
			}

			void AddScalarOption(UFunction* method, const char* scalarId, IGameOptionsBuilder& builder)
			{
				struct FRococoGameOptionScalarPackage
				{
					double currentScalar;
					FRococoGameOptionScalar spec;
				};

				FRococoGameOptionScalarPackage args;

				optionObject->ProcessEvent(method, &args);

				auto& s = builder.AddScalar(scalarId);

				s.SetActiveValue(args.currentScalar);

				char hint[256];
				ToAscii(hint, sizeof hint, args.spec.Hint);

				s.SetHint(hint);

				char title[256];
				ToAscii(title, sizeof title, args.spec.Title);

				s.SetTitle(title);

				s.SetRange(args.spec.MinValue, args.spec.MaxValue, args.spec.QuantumDelta);

				s.SetDecimalPlaces(FMath::Clamp(args.spec.DecimalPlaces, 0, 9));
			}

			void AddOptions(IGameOptionsBuilder& builder) override
			{
				// Enumerate methods and populate builder

				auto byNameAscending = [](const UFunction& a, const UFunction& b) -> bool
				{
					FString aName = a.GetFName().ToString();
					FString bName = b.GetFName().ToString();
					return a < b;
				};
				addMethods.Sort(byNameAscending);

				for (auto* method : addMethods)
				{
					FString methodName = method->GetFName().ToString();
					const char* optionName = GetVolatileAsciiTrailingString(methodName, addOptionPrefix);
					if (optionName)
					{
						auto* argType = GetMethodArgument(method, 0);

						auto* doubleArg = CastField<FDoubleProperty>(argType);
						if (doubleArg)
						{
							AddScalarOption(method, optionName, builder);
							continue;
						}

						auto* boolArg = CastField<FBoolProperty>(argType);
						if (boolArg)
						{
							AddBoolOption(method, optionName, builder);
							continue;
						}

						auto* choiceArg = CastField<FStrProperty>(argType);
						if (choiceArg)
						{
							AddChoiceOption(method, optionName, builder);
							continue;
						}
					}
				}
			}

			IOptionDatabase& DB() override
			{
				return *this;
			}

			void Accept() override
			{
				// invoke accept on the object
			}

			void Revert() override
			{
				// invoke revert on the object
			}

			bool IsModified() const override
			{
				// invoke isModified on the object
				return false;
			}

			UFunction* GetInvokeMethod(FString fullname)
			{
				for (auto* method : handlerMethods)
				{
					FString methodName = method->GetFName().ToString();
					if (methodName == fullname)
					{
						return method;
					}
				}

				return nullptr;
			}

			void Invoke(cstr name, cstr choice) override
			{
				// invoke OnChoice on the object
				FString invokeName = FString::Printf(TEXT("OnChoice_%hs"), name);
				auto* method = GetInvokeMethod(invokeName);
				if (!method)
				{
					OnError(nullptr, nullptr, FString::Printf(TEXT("Cannot find method %s"), *invokeName));
					return;
				}

				FString arg = FString::Printf(TEXT("%hs"), choice);
				optionObject->ProcessEvent(method, &arg);
			}

			void Invoke(cstr name, bool boolValue) override
			{
				// Invoke OnBool on the object
				FString invokeName = FString::Printf(TEXT("OnBool_%hs"), name);
				auto* method = GetInvokeMethod(invokeName);
				if (!method)
				{
					OnError(nullptr, nullptr, FString::Printf(TEXT("Cannot find method %s"), *invokeName));
					return;
				}
				
				uint32 u32bool = boolValue == false ? 0 : 1;
				optionObject->ProcessEvent(method, &boolValue);
			}

			void Invoke(cstr name, double scalarValue) override
			{
				// Invoke OnScalar on the object
				FString invokeName = FString::Printf(TEXT("OnScalar_%hs"), name);
				auto* method = GetInvokeMethod(invokeName);
				if (!method)
				{
					OnError(nullptr, nullptr, FString::Printf(TEXT("Cannot find method %s"), *invokeName));
					return;
				}

				optionObject->ProcessEvent(method, &scalarValue);
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

			void ReflectIntoGenerator(UObject& object, const FString& optionCategory, Rococo::GreatSex::IGreatSexGenerator& generator) override
			{
				UClass* objectClass = object.GetClass();

				UE_LOG(LogReflectedBuilder, Display, TEXT("Reflecting %s"), *objectClass->GetFName().ToString());

				ReflectedGameOptions options(object);
				auto& refOptions = mapObjectToOptions.Add(&object, options);

				char name[256];
				ToAscii(name, sizeof name, optionCategory);

				generator.AddOptions(refOptions, name);

				for (TFieldIterator<UFunction> i(objectClass); i; ++i)
				{
					UFunction* method = *i;
					refOptions.AddMethod(method);
				}
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