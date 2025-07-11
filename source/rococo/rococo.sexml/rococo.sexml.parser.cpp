#include <rococo.compiler.options.h>
#include <sexy.types.h>
#include <Sexy.S-Parser.h>
#include <rococo.sexml.h>
#include <rococo.strings.h>
#include <math.h>
#include <new>
#include <vector>
#include <rococo.hashtable.h>
#include <rococo.functional.h>

using namespace Rococo::Sex;
using namespace Rococo::Strings;

namespace Rococo::Sex::SEXML
{
	template<class T>
	using TVector = std::vector<T>;

	template<class T>
	using TStringMap = Rococo::stringmap<T>;

	template<class T>
	void VALIDATE_SIMPLE_INTERFACE(T* This, cstr functionName)
	{
		UNUSED(functionName);
#ifdef _DEBUG
		ISEXMLAttributeValue* iThis = static_cast<ISEXMLAttributeValue*>(This);
		ISEXMLAttributeValue* iThis2 = reinterpret_cast<ISEXMLAttributeValue*>(This);
		if (iThis != iThis2)
		{
			Rococo::Throw(0, "%s: ISEXMLAttributeValue vtable needs to be first member of any of its implementations", functionName);
		}
#else
		UNUSED(This);
#endif
	}

	struct RawValue : ISEXMLAttributeValue
	{
		cr_sex s;
		const ISEXMLAttribute& a;

		RawValue(const ISEXMLAttribute& _a, cr_sex _s) noexcept : s(_s), a(_a)
		{
			VALIDATE_SIMPLE_INTERFACE(this, __ROCOCO_FUNCTION__);
		}

		const ISEXMLAttribute& Attribute() const override
		{
			return a;
		}

		SEXMLValueType Type() const override
		{
			return SEXMLValueType::Raw;
		}

		cr_sex S() const override
		{
			return s;
		}
	};

	struct NullValue : ISEXMLAttributeValue
	{
		const ISEXMLAttribute& a;

		NullValue(const ISEXMLAttribute& _a): a(_a)
		{

		}

		const ISEXMLAttribute& Attribute() const override
		{
			return a;
		}

		SEXMLValueType Type() const override
		{
			return SEXMLValueType::Null;
		}

		cr_sex S() const override
		{
			return a.S();
		}
	};

	struct StringValue : ISEXMLAttributeStringValue
	{
#ifdef _DEBUG
		cstr value; // having a value as a member increases memory cost, but eases debugging in the watch view
#endif
		cr_sex s;
		const ISEXMLAttribute& a;

		StringValue(const ISEXMLAttribute& _a, cr_sex _s) noexcept : s(_s), a(_a)
		{
#ifdef _DEBUG
			value = s.c_str();
#endif
			VALIDATE_SIMPLE_INTERFACE(this, __ROCOCO_FUNCTION__);
		}

		const ISEXMLAttribute& Attribute() const override
		{
			return a;
		}

		SEXMLValueType Type() const override
		{
			return IsAtomic(s) ? SEXMLValueType::Atomic : SEXMLValueType::StringLiteral;
		}

#ifdef _DEBUG
		cstr c_str() const override
		{
			return value;
		}
#else
		cstr c_str() const override
		{
			return s.c_str();
		}
#endif

		fstring ToFString() const override
		{
			sexstring value = s.String();
			return { value->Buffer, value->Length };
		}

		cr_sex S() const override
		{
			return s;
		}
	};

#pragma pack(push,1)
	struct SmallVectorI : ISEXMLAttributeSmallVectorIValue
	{
		int32 t0 = 0;
		int32 t1 = 0;
		int32 t2 = 0;
		int32 t3 = 0;

		// (#VecN <name> <x> <y> ...) 
		cr_sex sAttribute;
		const ISEXMLAttribute& a;

		SmallVectorI(const ISEXMLAttribute& _a, cr_sex s, const int* args) noexcept :sAttribute(s), a(_a)
		{
			VALIDATE_SIMPLE_INTERFACE(this, __ROCOCO_FUNCTION__);

			int count = NumberOfDimensions();

			int* t = &t0;

			for (int i = 0; i < count; i++)
			{
				t[i] = args[i];
			}
		}

		const ISEXMLAttribute& Attribute() const override
		{
			return a;
		}

		int NumberOfDimensions() const override
		{
			return sAttribute.NumberOfElements() - 2;
		}

		const int* X() const override
		{
			return &t0;
		}

		SEXMLValueType Type() const override
		{
			return SEXMLValueType::SmallVectorI;
		}

		cr_sex S() const override
		{
			return sAttribute;
		}

		static SmallVectorI* Create(ISEXMLAttribute& a, IAllocator& allocator, cr_sex sAttribute)
		{
			int32 items[4];

			int nDimensions = sAttribute.NumberOfElements() - 2;
			if (nDimensions < 2 || nDimensions > 4)
			{
				Throw(sAttribute, "The attribute vector must contain between 2 and 4 elements");
			}

			for (int i = 0; i < nDimensions; ++i)
			{
				cr_sex sValue = sAttribute[i + 2];
				if (!IsAtomic(sValue))
				{
					Throw(sValue, "Expecting a vector component of literal type double. This was not an atomic expression");
				}

				cstr value = sValue.c_str();

				try
				{
					if (StartsWith(value, "0x"))
					{
						if (sscanf_s(value + 2, "%x", &items[i]) != 1)
						{
							Throw(sValue, "Error interpreting %s as a hexadecimal int32", value);
						}
					}
					else
					{
						items[i] = atoi(value);
					}
				}
				catch (...)
				{
					Throw(sValue, "Error interpreting %s as a double", value);
				}
			}

			void* pMemory = allocator.Allocate(sizeof(SmallVectorI));
			return new (pMemory) SmallVectorI(a, sAttribute, items);
		}
	};
#pragma pack(pop)

#pragma pack(push,1)
	struct SmallVector : ISEXMLAttributeSmallVectorValue
	{
		double x = 0.0;
		double y = 0.0;
		double z = 0.0;
		double w = 0.0;

		// (#VecN <name> <x> <y> ...) 
		cr_sex sAttribute;
		const ISEXMLAttribute& a;

		SmallVector(const ISEXMLAttribute& _a, cr_sex s, const double* args) :sAttribute(s), a(_a)
		{
			VALIDATE_SIMPLE_INTERFACE(this, __ROCOCO_FUNCTION__);

			int count = NumberOfDimensions();

			double* x0 = &x;

			for (int i = 0; i < count; i++)
			{
				x0[i] = args[i];
			}
		}

		const ISEXMLAttribute& Attribute() const override
		{
			return a;
		}

		int NumberOfDimensions() const override
		{
			return sAttribute.NumberOfElements() - 2;
		}

		const double* const X() const override
		{
			return &x;
		}

		SEXMLValueType Type() const override
		{
			return SEXMLValueType::SmallVector;
		}

		cr_sex S() const override
		{
			return sAttribute;
		}

		static SmallVector* Create(ISEXMLAttribute& a, IAllocator& allocator, cr_sex sAttribute)
		{
			double items[4];

			int nDimensions = sAttribute.NumberOfElements() - 2;
			if (nDimensions < 2 || nDimensions > 4)
			{
				Throw(sAttribute, "The attribute vector must contain between 2 and 4 elements");
			}

			for (int i = 0; i < nDimensions; ++i)
			{
				cr_sex sValue = sAttribute[i + 2];
				if (!IsAtomic(sValue))
				{
					Throw(sValue, "Expecting a vector component of literal type double. This was not an atomic expression");
				}

				cstr value = sValue.c_str();

				try
				{
					items[i] = atof(value);
				}
				catch (...)
				{
					Throw(sValue, "Error interpreting %s as a double", value);
				}
			}

			void* pMemory = allocator.Allocate(sizeof(SmallVector));
			return new (pMemory) SmallVector(a, sAttribute, items);
		}
	};
#pragma pack(pop)

	struct ListValue : ISEXMLAttributeStringListValue
	{
		cr_sex s;
		SEXMLValueType type;
		const ISEXMLAttribute& a;

		ListValue(const ISEXMLAttribute& _a, cr_sex sAttribute, SEXMLValueType _type) noexcept : s(sAttribute), type(_type), a(_a)
		{
			VALIDATE_SIMPLE_INTERFACE(this, __ROCOCO_FUNCTION__);
		}

		const ISEXMLAttribute& Attribute() const override
		{
			return a;
		}

		cr_sex S() const override
		{
			return s;
		}

		SEXMLValueType Type() const override
		{
			return type;
		}

		size_t NumberOfElements() const override
		{
			return s.NumberOfElements() - 2;
		}

		fstring operator[](size_t index) const override
		{
			int iIndex = (int)index;
			if (iIndex + 2 >= s.NumberOfElements())
			{
				Throw(s, "The expression had %d items. The index passed was %llu", s.NumberOfElements() - 2, iIndex);
			}

			auto item = s[iIndex + 2].String();

			return fstring{ item->Buffer, item->Length };
		}
	};

	namespace Impl
	{
		cstr ValidateAttributeNameAndGet(cr_sex sAttribute, int index)
		{
			if (index >= sAttribute.NumberOfElements())
			{
				Throw(sAttribute, "Expecting <attribute-name> at position %d", index);
			}

			cr_sex sName = sAttribute[index];
			if (!IsAtomic(sName))
			{
				Throw(sName, "Expected an atomic expression for the <attribute-name>");
			}

			cstr name = sName.c_str();

			if (!IsAlphabetical(name[0]))
			{
				Throw(sName, "Expecting either the quote character (') , a #<type> or a name that begins with a capital or lower case letter [A-Z] | [a-z]");
			}

			return name;
		}

		struct Attribute : ISEXMLAttribute
		{
			cstr attributeName = nullptr;

			ISEXMLAttributeValue* a = nullptr;
			SEXMLValueType type;

			ISEXMLRootSupervisor& root;
			cr_sex sAttribute;

			Attribute(ISEXMLRootSupervisor& _root, cr_sex s) : root(_root), sAttribute(s)
			{
				cr_sex attributeFunction = sAttribute[0];

				if (!IsAtomic(s[0]))
				{
					Throw(attributeFunction, "Expected an atomic expression for the (name value) pair for attribute");
				}

				sexstring function = attributeFunction.String();

				if (function->Length >= ISEXMLAttribute::MAX_ATTRIBUTE_NAME_LENGTH)
				{
					Throw(attributeFunction, "Attribute name length was %d characters. The maximum is %u characters", function->Length, ISEXMLAttribute::MAX_ATTRIBUTE_NAME_LENGTH - 1);
				}

				cstr fName = function->Buffer;

				int namePos = 1;

				if (Eq(fName, "'"))
				{
					if (sAttribute.NumberOfElements() != 3)
					{
						Throw(sAttribute, "Expecting three elements in raw attribute (' <name> <raw>)");
					}

					// Raw expression (' <name> ...) - The value of the raw expression is the s-expression of the attribute itself.
					// If we were instead to define the third argument as the value, this would entail wrapping a list of items in parenthesis, adding to bloat.
					type = SEXMLValueType::Raw;
					auto* pMemory = root.Allocator().Allocate(sizeof(RawValue));
					a = new (pMemory) RawValue(*this, sAttribute);
				}
				else if (Eq(fName, "#Vec2"))
				{
					// (#Vec2 <name> <x> <y>)
					type = SEXMLValueType::SmallVector;
					a = SmallVector::Create(*this, root.Allocator(), sAttribute);
					if (a->S().NumberOfElements() != 6)
					{
						Throw(a->S(), "Expecting two arguments (#Vec3 <name> <Vx> <Vy>)");
					}
				}
				else if (Eq(fName, "#Vec3"))
				{
					// (#Vec3 <name> <x> <y> <z>)
					type = SEXMLValueType::SmallVector;
					a = SmallVector::Create(*this, root.Allocator(), sAttribute);
					if (a->S().NumberOfElements() != 6)
					{
						Throw(a->S(), "Expecting three arguments (#Vec3 <name> <Vx> <Vy> <Vz>)");
					}
				}
				else if (Eq(fName, "#Vec4"))
				{
					// (#Vec4 <name> <x> <y> <z> <w>)
					type = SEXMLValueType::SmallVector;
					a = SmallVector::Create(*this, root.Allocator(), sAttribute);
					if (a->S().NumberOfElements() != 6)
					{
						Throw(a->S(), "Expecting four arguments (#Vec4 <name> <Vx> <Vy> <Vz> <Sw>)");
					}
				}
				else if (Eq(fName, "#Quat"))
				{
					// (#Quat <name> <Vx> <Vy> <Vz> <Sw>)
					type = SEXMLValueType::SmallVector;
					a = SmallVector::Create(*this, root.Allocator(), sAttribute);
					if (a->S().NumberOfElements() != 6)
					{
						Throw(a->S(), "Expecting four arguments (#Quat <name> <Vx> <Vy> <Vz> <Sw>)");
					}
				}
				else if (Eq(fName, "#Vec2i"))
				{
					// (#Vec2i <name> <x> <y>)
					type = SEXMLValueType::SmallVectorI;
					a = SmallVectorI::Create(*this, root.Allocator(), sAttribute);
					if (a->S().NumberOfElements() != 4)
					{
						Throw(a->S(), "Expecting two arguments (#Vec2i <name> <x> <y>)");
					}
				}
				else if (Eq(fName, "#Recti"))
				{
					// (#Recti <name> <left> <top> <right> <bottom>)
					type = SEXMLValueType::SmallVectorI;
					a = SmallVectorI::Create(*this, root.Allocator(), sAttribute);
					if (a->S().NumberOfElements() != 6)
					{
						Throw(a->S(), "Expecting four arguments (#Recti <name> <left> <top> <right> <bottom>)");
					}
				}
				else if (Eq(fName, "#List") || Eq(fName, "[]"))
				{
					// set to raw until we determine what is in the list
					type = SEXMLValueType::Raw;

					// (#List <name> <item1> <item2> ... <itemN>). If there are no items, the list is deemed empty
					for (int i = 2; i < sAttribute.NumberOfElements(); ++i)
					{
						cr_sex sItem = sAttribute[i];
						switch (sItem.Type())
						{
						case EXPRESSION_TYPE_ATOMIC:
							if (type == SEXMLValueType::Raw)
							{
								type = SEXMLValueType::AtomicList;
							}
							else if (type == SEXMLValueType::StringLiteralList)
							{
								type = SEXMLValueType::MixedStringList;
							}
							break;
						case EXPRESSION_TYPE_STRING_LITERAL:
							if (type == SEXMLValueType::Raw)
							{
								type = SEXMLValueType::StringLiteralList;
							}
							else if (type == SEXMLValueType::AtomicList)
							{
								type = SEXMLValueType::MixedStringList;
							}
							break;
						default:
							Throw(sItem, "Expecting an atomic or string literal for item %d", i);
						}
					}

					if (type == SEXMLValueType::Raw)
					{
						// This implies no items, so we convert to atomic
						type = SEXMLValueType::AtomicList;
					}

					auto* pMemory = root.Allocator().Allocate(sizeof(ListValue));
					a = new (pMemory) ListValue(*this, sAttribute, type);
				}
				else
				{
					namePos = 0;

					if (fName[0] == '#')
					{
						Throw(attributeFunction, "Unknown attribute type, Known #types: #Vec2, #Vec3, #Vec4, #Quat, #Vec2i, #Recti, #List");
					}

					if (!IsAlphabetical(fName[0]))
					{
						Throw(attributeFunction, "Expecting either the quote character (') ,[], a #<type> or a name that begins with a capital or lower case letter [A-Z] | [a-z]");
					}

					if (s.NumberOfElements() == 1)
					{
						auto* pMemory = root.Allocator().Allocate(sizeof(NullValue));
						a = new (pMemory) NullValue(*this);
					}
					else
					{
						if (s.NumberOfElements() != 2)
						{
							Throw(attributeFunction, "Expecting two elements (name value), but saw %d", s.NumberOfElements());
						}

						cr_sex sValue = sAttribute[1];
						switch (sValue.Type())
						{
						case EXPRESSION_TYPE_ATOMIC:
							type = SEXMLValueType::Atomic;
							break;
						case EXPRESSION_TYPE_STRING_LITERAL:
							type = SEXMLValueType::StringLiteral;
							break;
						default:
							Throw(attributeFunction, "Expecting either an atomic or string literal for the value of the attribute. Perhaps you are missing a colon ':' that specifies the sub-directives from the attributes.");
						}

						auto* pMemory = root.Allocator().Allocate(sizeof(StringValue));
						a = new (pMemory) StringValue(*this, sValue);
					}
				}

				attributeName = ValidateAttributeNameAndGet(sAttribute, namePos);
			}

			virtual ~Attribute()
			{
				a->~ISEXMLAttributeValue();
				// None of our objects require destruction, so we are free to delete the memory
				root.Allocator().FreeData(a);
			}

			cstr Name() const override
			{
				return attributeName;
			}

			cr_sex S() const override
			{
				return sAttribute;
			}

			const ISEXMLAttributeValue& Value() const override
			{
				return *a;
			}
		};

		struct Directive : ISEXMLDirective, ISEXMLDirectiveList
		{
			TVector<Attribute*> attributes;
			TStringMap<Attribute*> nameToAttribute;
			ISEXMLRootSupervisor& root;
			cr_sex sDirective;
			TVector<Directive*> children;
			Directive* parent;

			Directive(ISEXMLRootSupervisor& _root, cr_sex s, Directive* _parent) : root(_root), sDirective(s), parent(_parent)
			{
				// The caller guarantees that the expression is compound and not empty
				cr_sex sName = s[0];

				if (sName.Type() == EXPRESSION_TYPE_STRING_LITERAL)
				{
					Throw(sName, "Expecting a fully qualified namespace without encapsulating quote characters");
				}
				else if (sName.Type() != EXPRESSION_TYPE_ATOMIC)
				{
					Throw(sName, "Expecting a fully qualified namespace, not a subexpression");
				}

				sexstring name = sName.String();
				if (name->Length >= MAX_FQNAME_LENGTH)
				{
					Throw(sName, "The directive namespace length was %d characters. The limit is %d characters", name->Length, MAX_FQNAME_LENGTH);
				}

				try
				{
					Rococo::Strings::ValidateFQNamespace(name->Buffer);
				}
				catch (IException& ex)
				{
					Throw(sName, "Error validating the directive '%s': %s", name->Buffer, ex.Message());
				}

				int firstSub = 0;

				for (int i = 1; i < s.NumberOfElements(); ++i)
				{
					cr_sex sAttribute = s[i];
					if (IsAtomic(sAttribute) && Eq(sAttribute.c_str(), ":"))
					{
						// Sub directive indicator
						firstSub = i + 1;
						break;
					}
					else if (!IsCompound(sAttribute))
					{
						Throw(sAttribute, "Expected a (name value) pair for attribute %d, but the expression was not compound", i - 1);
					}

					void* pMemory = root.Allocator().Allocate(sizeof(Attribute));

					try
					{
						Attribute* a = new (pMemory) Attribute(_root, sAttribute);
						cstr attrname = a->Name();
						auto insertResult = nameToAttribute.insert(attrname, a);
						auto insertIterator = insertResult.first;
						bool wasInserted = insertResult.second;
						if (!wasInserted)
						{
							// we have a duplicate
							Throw(sAttribute, "Duplicate attribute name [%s] at position [%d] in the [%s] directive", attrname, i, name->Buffer);
						}
						else
						{
							attributes.push_back(a);
						}
					}
					catch (...)
					{
						root.Allocator().FreeData(pMemory);
						throw;
					}
				}

				if (firstSub == 0) return;

				for (int i = firstSub; i < s.NumberOfElements(); i++)
				{
					cr_sex sSubdirective = s[i];

					if (!IsCompound(sSubdirective))
					{
						Throw(sSubdirective, "Expecting compound expression for sub directive [%d]", i);
					}

					void* pMemory = root.Allocator().Allocate(sizeof(Impl::Directive));

					try
					{
						Directive* d = new (pMemory) Impl::Directive(root, sSubdirective, this);
						children.push_back(d);
					}
					catch (...)
					{
						root.Allocator().FreeData(pMemory);
						throw;
					}
				}
			}

			const ISEXMLDirective* Parent() const override
			{
				return parent;
			}

			size_t NumberOfDirectives() const override
			{
				return children.size();
			}

			const ISEXMLDirective& operator[](size_t index) const override
			{
				return *children[index];
			}

			virtual ~Directive()
			{
				for (auto* a : attributes)
				{
					a->~Attribute();
					root.Allocator().FreeData(a);
				}

				for (auto* child : children)
				{
					child->~Directive();
					root.Allocator().FreeData(child);
				}
			}

			const ISEXMLDirectiveList& Children() const override
			{
				return *this;
			}

			void Assert(cstr expectedFqName) const override
			{
				if (expectedFqName == nullptr)
				{
					Throw(S(), "%s(nullptr)", __ROCOCO_FUNCTION__);
				}

				if (Strings::Eq(expectedFqName, FQName()))
				{
					return;
				}

				Throw(S(), "FQName: '%s' did not match the expected '%s'", FQName(), expectedFqName);
			}

			cstr FQName() const override
			{
				return sDirective[0].c_str();
			}

			cr_sex S() const override
			{
				return sDirective;
			}

			size_t NumberOfAttributes() const override
			{
				return attributes.size();
			}

			size_t NumberOfChildren() const override
			{
				return children.size();
			}

			const ISEXMLDirective& GetChild(size_t index) const override
			{
				if (index >= children.size())
				{
					Throw(sDirective, "ISEXMLDirective::GetChild(index=%llu).Index was larger than the child count of % llu", index, children.size());
				}

				return *children[index];
			}

			const ISEXMLDirective* FindFirstChild(IN OUT size_t& startIndex, cstr fqName) const override
			{
				if (startIndex >= children.size())
				{
					return nullptr;
				}

				for (size_t i = startIndex; i < children.size(); i++)
				{
					auto& child = children[i];
					if (fqName == nullptr || Strings::Eq(child->FQName(), fqName))
					{
						OUT startIndex = i;
						return children[i];
					}
				}

				return nullptr;
			}

			const ISEXMLDirective& GetDirectivesFirstChild(IN OUT size_t& startIndex, cstr fqName) const override
			{
				const ISEXMLDirective* directive = FindFirstChild(IN OUT startIndex, fqName);
				if (!directive)
				{
					Throw(S(), "%s: Expecting a directive (%s ...)", fqName ? fqName : "<any>");
				}
				return *directive;
			}

			// get the name-value pair by index as it appears in the SEXML file
			const ISEXMLAttribute& GetAttributeByIndex(size_t index) const override
			{
				if (index >= attributes.size())
				{
					Throw(sDirective, "ISEXMLDirective::GetAttributeByIndex(index=%llu). Index out of bounds. There were only %llu attributes", index, attributes.size());
				}

				return *attributes[index];
			}

			// get the name-value pair by name, if it exists, else returns nullptr
			const ISEXMLAttribute* FindAttributeByName(cstr name) const override
			{
				if (name == nullptr || *name == 0)
				{
					Rococo::Sex::Throw(sDirective, "%s(name): [name] was blank", __ROCOCO_FUNCTION__);
				}
				auto i = nameToAttribute.find(name);
				return i != nameToAttribute.end() ? i->second : nullptr;
			}

			const ISEXMLAttribute& GetAttributeByName(cstr name) const override
			{
				auto* a = FindAttributeByName(name);
				if (!a)
				{
					Rococo::Sex::Throw(sDirective, "Cannot find attribute [%s] in %s", name, FQName());
				}
				return *a;
			}
		};

		struct SEXMLParser : ISEXMLRootSupervisor
		{
			TVector<Directive*> topLevelDirectives;

			IAllocator& allocator;
			cr_sex sRoot;

			SEXMLParser(cr_sex _sRoot, IAllocator& _allocator) : allocator(_allocator), sRoot(_sRoot)
			{
				topLevelDirectives.reserve(_sRoot.NumberOfElements());

				for (int i = 0; i < _sRoot.NumberOfElements(); i++)
				{
					cr_sex sDirective = _sRoot[i];

					if (!IsCompound(sDirective))
					{
						Throw(sDirective, "Expecting compound expression for top level directive [%d]", i);
					}

					void* pMemory = allocator.Allocate(sizeof(Impl::Directive));

					try
					{
						Directive* d = new (pMemory) Impl::Directive(*this, sDirective, nullptr);
						topLevelDirectives.push_back(d);
					}
					catch (...)
					{
						allocator.FreeData(pMemory);
						throw;
					}
				}
			}

			virtual ~SEXMLParser()
			{
				for (auto d : topLevelDirectives)
				{
					d->~Directive();
					allocator.FreeData(d);
				}
			}

			IAllocator& Allocator()
			{
				return allocator;
			}

			cr_sex S() const override
			{
				return sRoot;
			}

			void Free() override
			{
				this->~SEXMLParser();
				allocator.FreeData(this);
			}

			size_t NumberOfDirectives() const override
			{
				return sRoot.NumberOfElements();
			}

			const ISEXMLDirective& operator[](size_t index) const override
			{
				if (index >= topLevelDirectives.size())
				{
					Rococo::Sex::Throw(sRoot, "ISEXMLDirective& [][index=%llu]. Index out of bounds. Directive count was %llu", index, topLevelDirectives.size());
				}

				return *topLevelDirectives[index];
			}
		};
	}

	void OnBadParameter(
		crwstr expression,
		crwstr function,
		crwstr file,
		unsigned int line,
		uintptr_t pReserved)
	{
		UNUSED(expression);
		UNUSED(function);
		UNUSED(file);
		UNUSED(pReserved);
		UNUSED(line);
		Rococo::Throw(0, "Bad parameter");
	}

	ROCOCO_SEXML_API [[nodiscard]] ISEXMLRootSupervisor* CreateSEXMLParser(IAllocator& allocator, cr_sex sRoot)
	{
#ifdef _WIN32
		_invalid_parameter_handler old = _set_invalid_parameter_handler(OnBadParameter);
#endif
		void* pMemory = allocator.Allocate(sizeof(Impl::SEXMLParser));

		try
		{
			auto* p = new (pMemory) Impl::SEXMLParser(sRoot, allocator);
#ifdef _WIN32
			_set_invalid_parameter_handler(old);
#endif
			return p;
		}
		catch (...)
		{
#ifdef _WIN32
			_set_invalid_parameter_handler(old);
#endif
			allocator.FreeData(pMemory);
			throw;
		}
	}

	ROCOCO_SEXML_API [[nodiscard]] const ISEXMLDirective* FindDirective(const ISEXMLDirectiveList& items, cstr directiveName, IN OUT size_t& startIndex)
	{
		if (startIndex >= items.NumberOfDirectives())
		{
			return nullptr;
		}

		for (size_t i = startIndex; i < items.NumberOfDirectives(); i++)
		{
			if (Eq(items[i].FQName(), directiveName))
			{
				startIndex = i + 1;
				return &items.operator[](i);
			}
		}

		startIndex = items.NumberOfDirectives();
		return nullptr;
	}

	ROCOCO_SEXML_API [[nodiscard]] const ISEXMLDirective& GetDirective(const ISEXMLDirectiveList& items, cstr directiveName, IN OUT size_t& startIndex)
	{
		const ISEXMLDirective* dir = FindDirective(items, directiveName, startIndex);
		if (!dir)
		{
			Rococo::Sex::Throw(items.S(), "Could not find a directive (%s ...) in the SXML file", directiveName);
		}
		return *dir;
	}

	ROCOCO_SEXML_API [[nodiscard]] const ISEXMLAttributeStringValue& AsString(const ISEXMLAttributeValue& value)
	{
		switch (value.Type())
		{
		case Rococo::Sex::SEXML::SEXMLValueType::Atomic:
		case Rococo::Sex::SEXML::SEXMLValueType::StringLiteral:
			break;
		default:
			Rococo::Sex::Throw(value.S(), "Cannot interpret %s value as a string.", value.Attribute().Name());
		}

		return static_cast<const Rococo::Sex::SEXML::ISEXMLAttributeStringValue&>(value);
	}

	ROCOCO_SEXML_API [[nodiscard]] const ISEXMLAttributeStringListValue& AsStringList(const ISEXMLAttributeValue& value)
	{
		switch (value.Type())
		{
		case Rococo::Sex::SEXML::SEXMLValueType::AtomicList:
		case Rococo::Sex::SEXML::SEXMLValueType::StringLiteralList:
		case Rococo::Sex::SEXML::SEXMLValueType::MixedStringList:
			break;
		default:
			Rococo::Sex::Throw(value.S(), "Cannot interpret value as a string list. Expecting (#List %s <args...>)", value.Attribute().Name());
		}

		return static_cast<const Rococo::Sex::SEXML::ISEXMLAttributeStringListValue&>(value);
	}

	ROCOCO_SEXML_API [[nodiscard]] uint64 AsFlags(const ISEXMLAttributeValue& value, Function<uint64(const fstring& s)> mapNameToFlag)
	{
		uint64 flags = 0;

		const ISEXMLAttributeStringListValue& items = AsStringList(value);
		for (int i = 0; i < items.NumberOfElements(); i++)
		{
			try
			{
				auto item = items[i];
				uint64 flag = mapNameToFlag(item);
				flags |= flag;
			}
			catch (IException& ex)
			{
				Throw(items.S(), "Error parsing element[%d] for %s: %s", i, value.Attribute().Name(), ex.Message());
			}
		}

		return flags;
	}

	ROCOCO_SEXML_API const ISEXMLAttributeSmallVectorIValue& AsSmallIVector(const ISEXMLAttributeValue& value)
	{
		switch (value.Type())
		{
		case Rococo::Sex::SEXML::SEXMLValueType::SmallVectorI:
			break;
		default:
			Rococo::Sex::Throw(value.S(), "Cannot interpret %s value as a Vector. Expecting #Vec<N>", value.Attribute().Name());
		}

		return static_cast<const Rococo::Sex::SEXML::ISEXMLAttributeSmallVectorIValue&>(value);
	}

	ROCOCO_SEXML_API [[nodiscard]] Vec2i AsVec2i(const ISEXMLAttributeValue& value)
	{
		auto& v = AsSmallIVector(value);
		if (v.NumberOfDimensions() != 2)
		{
			Rococo::Sex::Throw(value.S(), "Expecting two integers", value.Attribute().Name());
		}

		return { v.X()[0], v.X()[1] };
	}

	ROCOCO_SEXML_API [[nodiscard]] GuiRect AsGuiRect(const ISEXMLAttributeValue& value)
	{
		auto& v = AsSmallIVector(value);
		if (v.NumberOfDimensions() != 4)
		{
			Rococo::Sex::Throw(value.S(), "Expecting four integers: left top right bottom", value.Attribute().Name());
		}

		return { v.X()[0], v.X()[1], v.X()[2], v.X()[3] };
	}

	ROCOCO_SEXML_API const ISEXMLAttributeSmallVectorValue& AsSmallVector(const ISEXMLAttributeValue& value)
	{
		switch (value.Type())
		{
		case Rococo::Sex::SEXML::SEXMLValueType::SmallVector:
			break;
		default:
			Rococo::Sex::Throw(value.S(), "Cannot interpret %s value as a Vector. Expecting #Vec<N>", value.Attribute().Name());
		}

		return static_cast<const Rococo::Sex::SEXML::ISEXMLAttributeSmallVectorValue&>(value);
	}

	ROCOCO_SEXML_API Vec2i GetOptionalAttribute(const ISEXMLDirective& directive, cstr attributeName, Vec2i defaultValues)
	{
		auto* a = directive.FindAttributeByName(attributeName);
		if (!a)
		{
			return defaultValues;
		}

		const ISEXMLAttributeSmallVectorIValue& value = AsSmallIVector(a->Value());
		if (value.NumberOfDimensions() != 2)
		{
			Throw(value.S(), "Expecting two components");
		}

		auto* x = value.X();

		return Vec2i{ x[0], x[1] };
	}

	ROCOCO_SEXML_API const fstring GetOptionalAttribute(const ISEXMLDirective& directive, cstr attributeName, const fstring defaultValue)
	{
		auto* a = directive.FindAttributeByName(attributeName);
		if (!a)
		{
			return defaultValue;
		}

		auto& value = AsString(a->Value());
		return value.ToFString();
	}

	ROCOCO_SEXML_API [[nodiscard]] int GetOptionalAttribute(const ISEXMLDirective& directive, cstr attributeName, int defaultValue)
	{
		auto* a = directive.FindAttributeByName(attributeName);
		if (!a)
		{
			return defaultValue;
		}

		return AsAtomicInt32(a->Value());
	}

	ROCOCO_SEXML_API GuiRect GetOptionalAttribute(const ISEXMLDirective& directive, cstr attributeName, GuiRect defaultValues)
	{
		auto* a = directive.FindAttributeByName(attributeName);
		if (!a)
		{
			return defaultValues;
		}

		const ISEXMLAttributeSmallVectorIValue& value = AsSmallIVector(a->Value());
		if (value.NumberOfDimensions() != 4)
		{
			Throw(value.S(), "Expecting four components");
		}

		auto* x = value.X();

		return GuiRect{ x[0], x[1], x[2], x[3]};
	}

	ROCOCO_SEXML_API bool GetOptionalAttribute(const ISEXMLDirective& directive, cstr attributeName, bool defaultValue)
	{
		auto* a = directive.FindAttributeByName(attributeName);
		if (!a)
		{
			return defaultValue;
		}

		bool value = AsBool(a->Value());
		return value;
	}

	ROCOCO_SEXML_API bool TryGetOptionalAttribute(const ISEXMLDirective& directive, cstr attributeName, OUT Vec2i& resultValues)
	{
		auto* a = directive.FindAttributeByName(attributeName);
		if (!a)
		{
			return false;
		}

		const ISEXMLAttributeSmallVectorIValue& value = AsSmallIVector(a->Value());
		if (value.NumberOfDimensions() != 2)
		{
			Throw(value.S(), "Expecting two components");
		}

		auto* x = value.X();

		resultValues = Vec2i{ x[0], x[1] };
		return true;
	}

	ROCOCO_SEXML_API bool TryGetOptionalAttribute(const ISEXMLDirective& directive, cstr attributeName, OUT Vec2F64& resultValues)
	{
		auto* a = directive.FindAttributeByName(attributeName);
		if (!a)
		{
			return false;
		}

		const ISEXMLAttributeSmallVectorValue& value = AsSmallVector(a->Value());
		if (value.NumberOfDimensions() != 2)
		{
			Throw(value.S(), "Expecting two components");
		}

		auto* x = value.X();

		resultValues = Vec2F64{ x[0], x[1] };
		return true;
	}

	ROCOCO_SEXML_API const ISEXMLAttributeStringValue& AsAtomic(const ISEXMLAttributeValue& value)
	{
		switch (value.Type())
		{
		case Rococo::Sex::SEXML::SEXMLValueType::Atomic:
			break;
		default:
			Rococo::Sex::Throw(value.S(), "Cannot interpret value as an atomic.");
		}

		return static_cast<const Rococo::Sex::SEXML::ISEXMLAttributeStringValue&>(value);
	}

	ROCOCO_SEXML_API double AsAtomicDouble(const ISEXMLAttributeValue& value)
	{
		cstr text = AsAtomic(value).c_str();

		double dValue = 0.0;
		if (sscanf_s(text, "%lg", &dValue) != 1)
		{
			Throw(value.S(), "Failed to parse %s as a double precision number", text);
		}

		return dValue;
	}

	ROCOCO_SEXML_API int32 AsAtomicInt32(const ISEXMLAttributeValue& value)
	{
		cstr text = AsAtomic(value).c_str();
		if (strlen(text) > 2 && (Strings::Compare(text, "0x", 2) == 0))
		{
			int32 hexValue = 0;
			sscanf_s(text + 2, "%x", &hexValue);
			return hexValue;
		}

		return atoi(text);
	}

	ROCOCO_SEXML_API bool AsBool(const ISEXMLAttributeValue& value)
	{
		cstr text = AsString(value).c_str();
		
		if (Eq(text, "1"))
		{
			return true;
		}

		if (EqI(text, "true"))
		{
			return true;
		}

		if (EqI(text, "yes"))
		{
			return true;
		}

		if (EqI(text, "aye"))
		{
			return true;
		}

		if (EqI(text, "t"))
		{
			return true;
		}

		return false;
	}
}