namespace HV
{
	struct ActionFactoryCreateContext : IIActionFactoryCreateContext, IGlobalVariables
	{
		IGlobalVariables& GetGlobals() override { return *this; }

		mutable stringmap<int32> int32Variables;

		bool GetValue(cstr name, int32& outputValue) const override
		{
			auto i = int32Variables.find(name);
			if (i != int32Variables.end())
			{
				outputValue = i->second;
				return true;
			}
			else
			{
				int32Variables[name] = outputValue;
				return false;
			}
		}

		int32 SetValue(cstr name, int32 value) override
		{
			auto i = int32Variables.find(name);
			if (i != int32Variables.end())
			{
				int32 output = i->second;
				i->second = value;
				return output;
			}
			else
			{
				int32Variables[name] = value;
				return 0;
			}
		}

		void ValidateName(cstr name) const override
		{
			if (name == nullptr || *name == 0) Throw(0, "Global variable name was null.");
			for (auto p = name; *p != 0; p++)
			{
				char c = *p;
				if (!IsAlphaNumeric(c) && c != '.' && c != '_')
				{
					Throw(0, "Global variable %s had bad character at position %u. Must be alphanumeric, . or _", name, p - name);
				}
			}

			if (strlen(name) >= IGlobalVariables::MAX_VARIABLE_NAME_LENGTH)
			{
				Throw(0, "Global variable too long %s. Max length is ", name, IGlobalVariables::MAX_VARIABLE_NAME_LENGTH);
			}
		}
	};

}