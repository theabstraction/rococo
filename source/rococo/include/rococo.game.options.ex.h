#pragma once

#include <rococo.game.options.h>
#include <vector>

namespace Rococo::Game::Options
{
	enum class EInquiryType
	{
		Choice,
		Boolean,
		Scalar,
		String
	};

	template<class T>
	struct InquiryFunctionDescriptor
	{
		typedef void (T::* FN_ChoiceInquireFunction)(IChoiceInquiry&);
		typedef void (T::* FN_BoolInquireFunction)(IBoolInquiry&);
		typedef void (T::* FN_ScalarInquireFunction)(IScalarInquiry&);
		typedef void (T::* FN_StringInquireFunction)(IStringInquiry&);

		union
		{
			FN_BoolInquireFunction BoolInquiryFunction = nullptr;
			FN_ChoiceInquireFunction ChoiceInquiryFunction;
			FN_ScalarInquireFunction ScalarInquiryFunction;
			FN_StringInquireFunction StringInquiryFunction;
		} functions;

		cstr name = nullptr;
		EInquiryType type = EInquiryType::Boolean;
		bool isReadOnly = false;
		int stringCapacity = 260;
	};

	template<class T>
	struct OptionSelectedFunctionDescriptor
	{
		typedef void (T::* FN_OnBoolOptionSelected)(bool value);
		typedef void (T::* FN_OnScalarOptionSelected)(double value);
		typedef void (T::* FN_OnChoiceSelected)(cstr value);
		typedef void (T::* FN_OnStringSelected)(cstr value);

		union
		{
			FN_OnBoolOptionSelected BoolSelectedFunction = nullptr;
			FN_OnScalarOptionSelected ScalarSelectedFunction;
			FN_OnChoiceSelected ChoiceSelectedFunction;
			FN_OnStringSelected StringSelectedFunction;
		} functions;

		cstr name = nullptr;
		EInquiryType type = EInquiryType::Boolean;
	};

	template<class T>
	struct OptionDatabase : IOptionDatabase
	{
		std::vector<InquiryFunctionDescriptor<T>> inquiryFunctions;
		std::vector<OptionSelectedFunctionDescriptor<T>> optionSelectedFunctions;

		T& owner;

		OptionDatabase(T& _owner) : owner(_owner)
		{

		}

		virtual ~OptionDatabase()
		{

		}

		void AddOption(cstr name, typename InquiryFunctionDescriptor<T>::FN_BoolInquireFunction inquiry, typename OptionSelectedFunctionDescriptor<T>::FN_OnBoolOptionSelected onSelect)
		{
			for (auto& i : inquiryFunctions)
			{
				if (strcmp(i.name, name) == 0)
				{
					// Duplicate
					return;
				}
			}
			InquiryFunctionDescriptor<T> q;
			q.functions.BoolInquiryFunction = inquiry;
			q.name = name;
			q.type = EInquiryType::Boolean;
			q.isReadOnly = onSelect == nullptr;
			inquiryFunctions.push_back(q);

			if (onSelect != nullptr)
			{
				OptionSelectedFunctionDescriptor<T> r;
				r.functions.BoolSelectedFunction = onSelect;
				r.name = name;
				r.type = q.type;
				optionSelectedFunctions.push_back(r);
			}
		}

		void AddOption(cstr name, typename InquiryFunctionDescriptor<T>::FN_ChoiceInquireFunction inquiry, typename OptionSelectedFunctionDescriptor<T>::FN_OnChoiceSelected onSelect)
		{
			for (auto& i : optionSelectedFunctions)
			{
				if (strcmp(i.name, name) == 0)
				{
					// Duplicate
					return;
				}
			}

			InquiryFunctionDescriptor<T> q;
			q.functions.ChoiceInquiryFunction = inquiry;
			q.name = name;
			q.type = EInquiryType::Choice;
			q.isReadOnly = onSelect == nullptr;
			inquiryFunctions.push_back(q);

			if (onSelect != nullptr)
			{
				OptionSelectedFunctionDescriptor<T> r;
				r.functions.ChoiceSelectedFunction = onSelect;
				r.name = name;
				r.type = q.type;
				optionSelectedFunctions.push_back(r);
			}
		}

		void AddOption(cstr name, typename InquiryFunctionDescriptor<T>::FN_ScalarInquireFunction option, typename OptionSelectedFunctionDescriptor<T>::FN_OnScalarOptionSelected onSelect)
		{
			for (auto& i : optionSelectedFunctions)
			{
				if (strcmp(i.name, name) == 0)
				{
					// Duplicate
					return;
				}
			}

			InquiryFunctionDescriptor<T> q;
			q.functions.ScalarInquiryFunction = option;
			q.name = name;
			q.type = EInquiryType::Scalar;
			q.isReadOnly = onSelect == nullptr;
			inquiryFunctions.push_back(q);

			if (onSelect != nullptr)
			{
				OptionSelectedFunctionDescriptor<T> r;
				r.functions.ScalarSelectedFunction = onSelect;
				r.name = name;
				r.type = q.type;
				optionSelectedFunctions.push_back(r);
			}
		}

		void AddOption(cstr name, int capacity, typename InquiryFunctionDescriptor<T>::FN_StringInquireFunction option, typename OptionSelectedFunctionDescriptor<T>::FN_OnStringSelected onSelect)
		{
			for (auto& i : optionSelectedFunctions)
			{
				if (strcmp(i.name, name) == 0)
				{
					// Duplicate
					return;
				}
			}

			InquiryFunctionDescriptor<T> q;
			q.functions.StringInquiryFunction = option;
			q.name = name;
			q.type = EInquiryType::String;
			q.isReadOnly = onSelect == nullptr;
			q.stringCapacity = capacity;
			inquiryFunctions.push_back(q);

			if (onSelect != nullptr)
			{
				OptionSelectedFunctionDescriptor<T> r;
				r.functions.StringSelectedFunction = onSelect;
				r.name = name;
				r.type = q.type;
				optionSelectedFunctions.push_back(r);
			}
		}

		void Build(IGameOptionsBuilder& builder)
		{
			for (auto& q : inquiryFunctions)
			{
				switch (q.type)
				{
				case EInquiryType::Boolean:
				{
					auto& Q = builder.AddBool(q.name);
					(owner.*q.functions.BoolInquiryFunction)(Q);
					break;
				}
				case EInquiryType::Choice:
				{
					auto& Q = builder.AddChoice(q.name);
					(owner.*q.functions.ChoiceInquiryFunction)(Q);
					break;
				}
				case EInquiryType::Scalar:
				{
					auto& Q = builder.AddScalar(q.name);
					(owner.*q.functions.ScalarInquiryFunction)(Q);
					break;
				}
				case EInquiryType::String:
				{
					auto& Q = builder.AddString(q.name, q.stringCapacity);
					(owner.*q.functions.StringInquiryFunction)(Q);
					break;
				}
				default:
					break;
				}
			}
		}

		void Invoke(cstr name, cstr choice) override
		{
			for (auto& r : optionSelectedFunctions)
			{
				if (Rococo::Strings::Eq(r.name, name) && r.type == EInquiryType::Choice)
				{
					(owner.*r.functions.ChoiceSelectedFunction)(choice);
					break;
				}

				if (Rococo::Strings::Eq(r.name, name) && r.type == EInquiryType::String)
				{
					(owner.*r.functions.StringSelectedFunction)(choice);
					break;
				}
			}
		}

		void Invoke(cstr name, bool boolValue) override
		{
			for (auto& r : optionSelectedFunctions)
			{
				if (Rococo::Strings::Eq(r.name, name) && r.type == EInquiryType::Boolean)
				{
					(owner.*r.functions.BoolSelectedFunction)(boolValue);
					break;
				}
			}
		}

		void Invoke(cstr name, double scalarValue) override
		{
			for (auto& r : optionSelectedFunctions)
			{
				if (Rococo::Strings::Eq(r.name, name) && r.type == EInquiryType::Scalar)
				{
					(owner.*r.functions.ScalarSelectedFunction)(scalarValue);
					break;
				}
			}
		}
	};
}

#define ADD_GAME_OPTIONS(DATABASE, IMPLEMENTOR, FUNCTION_SUFFIX) DATABASE.AddOption(#FUNCTION_SUFFIX, &IMPLEMENTOR::Get##FUNCTION_SUFFIX, &IMPLEMENTOR::Set##FUNCTION_SUFFIX);
#define ADD_GAME_OPTIONS_STRING(DATABASE, IMPLEMENTOR, FUNCTION_SUFFIX, CAPACITY) DATABASE.AddOption(#FUNCTION_SUFFIX, CAPACITY, &IMPLEMENTOR::Get##FUNCTION_SUFFIX, &IMPLEMENTOR::Set##FUNCTION_SUFFIX);
