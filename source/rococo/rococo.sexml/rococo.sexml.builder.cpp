#include <rococo.compiler.options.h>
#define ROCOCO_SEXML_API ROCOCO_API_EXPORT
#include <rococo.types.h>
#include <rococo.strings.h>
#include <rococo.sexml.h>
#include <vector>

using namespace Rococo::Strings;

namespace Rococo::Sex::SEXML
{
	namespace Impl
	{
		enum State
		{
			BUILDING_ROOT,
			BUILDING_ATTRIBUTES,
			BUILDING_CHILDREN,
			BUILDING_ATTRIBUTE_LIST
		};

		struct Builder : ISEXMLBuilder
		{
			StringBuilder& sb;
			int depth = 0; // 0 is the top level root
			std::vector<State> statestack;
			std::vector<HString> stateNames;
			bool isCompact;

			Builder(StringBuilder& _sb, bool _isCompact): sb(_sb), isCompact(_isCompact)
			{

			}

			void AddTab()
			{
				sb.AppendChar(isCompact ? ' ': '\t');
			}

			void AddDepthTabs()
			{
				for (int i = 0; i < depth; i++)
				{
					AddTab();
				}
			}

			void PrepareNewToken()
			{	
				if (isCompact)
				{
					sb << " ";
				}
				else
				{
					sb << "\n";
					AddDepthTabs();
				}
			}

			ISEXMLBuilder& AddDirective(cstr name) override
			{
				try
				{
					Rococo::Strings::ValidateFQNamespace(name);
				}
				catch (IException& ex)
				{
					char msg[1024];
					StackStringBuilder sb(msg, sizeof msg);
					sb.AppendFormat("Error validating directive name [%s]: %s\n", name, ex.Message());
					if (depth == 0)
					{
						sb << "Directive was at the root";
					}
					else
					{
						sb << "Error adding new directive to: ";
						for (auto& d : stateNames)
						{
							sb << "/";
							sb << d.c_str();
						}
					}
					Rococo::Throw(ex.ErrorCode(), "%s", msg);
				}
				
				if (depth == 0)
				{
					AddDepthTabs();

					depth++;

					sb.AppendChar('(');
					sb << name;
					statestack.push_back(BUILDING_ATTRIBUTES);
					stateNames.push_back(name);
					return *this;
				}

				switch (statestack.back())
				{
				case BUILDING_ATTRIBUTES:
					sb << " :";
					statestack.back() = BUILDING_CHILDREN;

				case BUILDING_CHILDREN:
					sb << "\n";
					AddDepthTabs();

					depth++;

					sb.AppendChar('(');
					sb << name;
					statestack.push_back(BUILDING_ATTRIBUTES);
					stateNames.push_back(name);
					break;
				default:
					Rococo::Throw(0, "Cannot add a child directive at this time. Was in the middle of another expression");
				}

				return *this;
			}

			ISEXMLBuilder& CloseDirective() override
			{
				if (depth == 0)
				{
					Rococo::Throw(0, "Cannot close directive. None were open");
				}
				depth--;
				sb << "\n";
				AddDepthTabs();
				sb << ")";

				if (!isCompact)
				{
					sb << " // ";
					sb << stateNames.back().c_str();
				}

				sb << "\n";

				statestack.pop_back();
				stateNames.pop_back();
				return *this;
			}

			void ValidateAttributeMode()
			{
				if (depth == 0)
				{
					Rococo::Throw(0, "There is no current directive in action");
				}
				
				switch (statestack.back())
				{
				case BUILDING_ATTRIBUTES:
					return;
				case BUILDING_ATTRIBUTE_LIST:
					Rococo::Throw(0, "Attribute could not be added. The write mode is set to append to an attribute list");
				case BUILDING_CHILDREN:
					Rococo::Throw(0, "Attribute could not be added. The write mode is set for building children. No more attributes can be added");
				default:
					Rococo::Throw(0, "Attribute could not be added - bad state %d", statestack.back());
				}
			}

			void ValidateAttributeName(cstr name)
			{
				ValidateAttributeMode();

				if (!name)
				{
					Rococo::Throw(0, "[name] was nullptr");
				}

				if (!IsAlphabetical(*name))
				{
					Rococo::Throw(0, "Expected attribute name [%s] to begin with A-Z | a-z", name);
				}

				for (cstr p = name + 1; *p != 0; p++)
				{
					if (!IsAlphaNumeric(*p))
					{
						switch (*p)
						{
						case '-':
						case '_':
						case '.':
							break;;
						default:
							Rococo::Throw(0, "Attribute name [%s] character was not valid. Expecting characters A-Z | a-z | one of -_. ", name);
						}
					}
				}
			}

			ISEXMLBuilder& AddAtomicAttribute(cstr name, cstr value) override
			{
				ValidateAttributeName(name);
				PrepareNewToken();
				sb << "(";
				sb << name;
				sb << " ";

				for (cstr p = value; *p != 0; p++)
				{
					if (*p <= 32)
					{
						Rococo::Throw(0, "Cannot write attribute [%s] as atomic string. It contained a glyph with UTF-8 value <= 32", name, *p);
					}
					
					switch (*p)
					{
					case '"':
					case ':':
					case '\'':
					case '(':
					case ')':
					case '/':
						Rococo::Throw(0, "Cannot write attribute [%s] as atomic string. It contained a glyph illegal in atomics: [%c]", name, *p);
					}
				}

				sb << value;

				sb << ")";

				return *this;
			}

			ISEXMLBuilder& AddAtomicAttribute(cstr name, int32 value) override
			{
				ValidateAttributeName(name);
				PrepareNewToken();
				sb << "(";
				sb << name;
				sb << " ";
				sb << value;
				sb << ")";
				return *this;
			}
	
			ISEXMLBuilder& AddAtomicAttribute(cstr name, int64 value)  override
			{
				ValidateAttributeName(name);
				PrepareNewToken();
				sb << "(";
				sb << name;
				sb << " ";
				sb << value;
				sb << ")";
				return *this;
			}

			ISEXMLBuilder& AddAtomicAttribute(cstr name, uint64 value) override
			{
				ValidateAttributeName(name);
				PrepareNewToken();
				sb.AppendFormat("(%s 0x%llX)", name, value);
				return *this;
			}

			ISEXMLBuilder& AddAtomicAttribute(cstr name, uint32 value) override
			{
				ValidateAttributeName(name);
				PrepareNewToken();
				sb.AppendFormat("(%s 0x%X)", name, value);
				return *this;
			}

			ISEXMLBuilder& AddAtomicAttribute(cstr name, bool value) override
			{
				ValidateAttributeName(name);
				PrepareNewToken();
				sb << "(";
				sb << name;
				sb << " ";
				sb << (value ? "true" : "false");
				sb << ")";
				return *this;
			}

			ISEXMLBuilder& AddAtomicAttribute(cstr name, float value) override
			{
				ValidateAttributeName(name);
				PrepareNewToken();
				sb << "(";
				sb << name;
				sb << " ";
				sb << value;
				sb << ")";
				return *this;
			}

			ISEXMLBuilder& AddAtomicAttribute(cstr name, double value) override
			{
				ValidateAttributeName(name);
				PrepareNewToken();
				sb << "(";
				sb << name;
				sb << " ";
				sb << value;
				sb << ")";
				return *this;
			}

			ISEXMLBuilder& AddStringLiteral(cstr  name, cstr value) override
			{
				ValidateAttributeName(name);
				PrepareNewToken();
				sb << "(";
				sb << name;
				sb << " \"";
				AppendEscapedSexyString(sb, value);
				sb << "\")";
				return *this;
			}

			ISEXMLBuilder& AddVec2(cstr name, double x, double y) override
			{
				ValidateAttributeName(name);
				PrepareNewToken();
				sb.AppendFormat("(#Vec2 %s %g %g)", name, x, y);
				return *this;
			}

			ISEXMLBuilder& AddVec3(cstr name, double x, double y, double z) override
			{
				ValidateAttributeName(name);
				PrepareNewToken();
				sb.AppendFormat("(#Vec3 %s %g %g %g)", name, x, y, z);
				return *this;
			}

			ISEXMLBuilder& AddVec4(cstr name, double x, double y, double z, double w) override
			{
				ValidateAttributeName(name);
				PrepareNewToken();
				sb.AppendFormat("(#Vec4 %s %g %g %g %g)", name, x, y, z, w);
				return *this;
			}

			bool isFirstQuat = true;

			ISEXMLBuilder& AddQuat(cstr name, double Vx, double Vy, double Vz, double scalar, bool addLayoutComment) override
			{
				if (isFirstQuat && addLayoutComment)
				{
					sb << " /* Quat components in this file are ordered thus: [VecX VecY VecZ Scalar] */ ";
					isFirstQuat = false;
				}

				ValidateAttributeName(name);
				PrepareNewToken();
				sb.AppendFormat("(#Quat %s %g %g %g %g)", name, Vx, Vy, Vz, scalar);
				return *this;
			}

			ISEXMLBuilder& AddVec2i(cstr name, int32 x, int32 y) override
			{
				ValidateAttributeName(name);
				PrepareNewToken();
				sb.AppendFormat("(#Vec2i %s %d %d)", name, x, y);
				return *this;
			}

			bool isFirstRect = true;

			ISEXMLBuilder& AddRecti(cstr name, int32 left, int32 top, int32 right, int32 bottom, bool addLayoutComment) override
			{
				if (isFirstRect && addLayoutComment)
				{
					sb << " /* Rect components in this file are ordered thus: [left top right bottom] */ ";
					isFirstRect = false;
				}

				ValidateAttributeName(name);
				PrepareNewToken();
				sb.AppendFormat("(#Recti %s %d %d %d %d)", name, left, top, right, bottom);
				return *this;
			}

			ISEXMLBuilder& OpenListAttribute(cstr name) override
			{
				ValidateAttributeName(name);
				PrepareNewToken();
				sb.AppendFormat("(#List %s", name);
				statestack.back() = BUILDING_ATTRIBUTE_LIST;
				return *this;
			}

			Rococo::Strings::StringBuilder& GetListBuilder() override
			{
				return sb;
			}

			ISEXMLBuilder& AddEscapedStringToList(cstr s) override
			{
				PrepareNewToken();
				AddTab();
				sb << "\"";
				AppendEscapedSexyString(sb, s);
				sb << "\"";
				return *this;
			}

			ISEXMLBuilder& CloseListAttribute() override
			{
				if (statestack.back() != BUILDING_ATTRIBUTE_LIST)
				{
					Rococo::Throw(0, "Cannot close attribute list, as no attribute list is open");
				}
				PrepareNewToken();
				sb << ")";

				statestack.back() = BUILDING_ATTRIBUTES;
				return *this;
			}

			void Free() override
			{
				delete this;
			}

			void ValidateClosed()
			{
				if (depth == 0) return;
				
				switch (statestack.back())
				{
				case BUILDING_ATTRIBUTES:
				case BUILDING_CHILDREN:
					Rococo::Throw(0, "SEXML builder not closed. Call CloseDirective to reduce depth to 0. Depth is currently %d", depth);
				case BUILDING_ATTRIBUTE_LIST:
					Rococo::Throw(0, "SEXML builder still building an attribute list. Close the list, then close the directives until depth is 0. Current depth is %d", depth);
				default:
					Rococo::Throw(0, "SEXML builder is in an unknown state. Close the directive until depth is 0. Current depth is %d", depth);
				}
			}
		};
	}

	ROCOCO_SEXML_API ISEXMLBuilder* CreateSEXMLBuilder(Rococo::Strings::StringBuilder& sb, bool isCompact)
	{
		return new Impl::Builder(sb, isCompact);
	}
}