#include "hv.h"
#include <rococo.strings.h>

namespace
{
	void ToPoundsAndOunces(Kilograms mass, int32& pounds, float& oz)
	{
		float lbsF32 = mass * 2.204623f;
		float lbsF32Int = floorf(lbsF32);
		constexpr float ozPerLb = 16;
		oz = ozPerLb * (lbsF32 - lbsF32Int);
		pounds = (int32)lbsF32Int;
	}

	void AppendInFeetAndInches(StringBuilder& sb, float x)
	{
		float inches = x * 39.37008f;
		float wholeInches = floorf(inches);
		float partialInches = inches - wholeInches;
		int32 feet = (int32)(wholeInches / 12.0f);

		int32 remainderInches = ((int32)wholeInches) % 12;

		if (feet > 0)
		{
			sb << feet << "' " << remainderInches << "\"";
		}
		else
		{
			float32 outputInches = partialInches + (float)remainderInches;
			sb.AppendFormat("%.2f\"", outputInches);
		}
	}

	void AppendMass(StringBuilder& sb, Kilograms mass)
	{
		if (mass < 1.0)
		{
			float grams = mass * 1000.0f;
			sb << (int32)grams << " grams\n";
		}
		else
		{
			sb.AppendFormat("%.2f kg", mass.value);
		}

		int pounds;
		float oz;
		ToPoundsAndOunces(mass, OUT pounds, OUT oz);

		if (pounds > 0)
		{
			sb.AppendFormat(" / %d lbs", pounds);

			int32 ioz = (int32)oz;
			if (ioz != 0)
			{
				sb.AppendFormat(" %d oz", ioz);
			}
		}
		else
		{
			sb.AppendFormat(" %.2f oz", oz);
		}
	}

	void AppendSpan(StringBuilder& sb, const Vec3& spanMetres)
	{
		auto& span = 1000.0f * spanMetres;

		sb.AppendFormat("%.0fmm x %.0fmm x %0.fmm (", span.x, span.y, span.z);

		AppendInFeetAndInches(sb, spanMetres.x);

		sb << "  x  ";

		AppendInFeetAndInches(sb, spanMetres.y);

		sb << "  x  ";

		AppendInFeetAndInches(sb, spanMetres.z);

		sb << ")\n";
	}

	void AppendEnumToBuilderForHumanReader(StringBuilder& sb, const fstring& name)
	{
		for (int i = 0; i < name.length; ++i)
		{
			char c = name[i];
			if (c >= 'A' && c <= 'Z')
			{
				if (i > 0)
				{
					sb.AppendChar(' ');
				}

				sb.AppendChar((char)(c + 32));
			}
			else
			{
				sb.AppendChar(c);
			}
		}
	}

	void AtomicNumberToString(StringBuilder& sb, int atomicNumber)
	{
		if (atomicNumber < 0)
		{
			sb << "anti-";
			AtomicNumberToString(sb, -atomicNumber);
			return;
		}
		else if (atomicNumber == 0)
		{
			sb << "pure-energy";
			return;
		}

		if (atomicNumber < 200)
		{
			auto& name = HV::Chemicals::ToShortString((HV::Chemicals::Element) atomicNumber);
			if (name.length > 0)
			{
				AppendEnumToBuilderForHumanReader(sb, name);
			}
			else
			{
				sb << "Element #" << atomicNumber;
			}
			return;
		}

		auto& name = HV::Chemicals::ToShortString((HV::Chemicals::Compounds) atomicNumber);
		if (name.length > 0)
		{
			AppendEnumToBuilderForHumanReader(sb, name);
		}
		else
		{
			sb << "Agent #" << atomicNumber;
		}
		return;
	}

	void HardnessToString(StringBuilder& sb, int32 mohsScale)
	{
		if (mohsScale <= 1)
		{
			sb << "crumbly";
			return;
		}

		switch (mohsScale)
		{
		case 2:
			sb << "quite soft";
			return;
		case 3:
			sb << "soft";
			return;
		case 4:
			sb << "not very hard";
			return;
		case 5:
			sb << "hard";
			return;
		case 6:
			sb << "quite hard";
			return;
		case 7:
			sb << "very hard";
			return;
		case 8:
			sb << "very very hard";
			return;
		case 9:
			sb << "almost as hard as diamond";
			return;
		case 10:
			sb << "hard as diamond";
			return;
		default:
			sb << "much harder than any diamond";
			return;
		}
	}

	void ToughnessToString(StringBuilder& sb, int32 toughness)
	{
		if (toughness <= 1)
		{
			sb << "extremely fragile";
			return;
		}

		switch (toughness)
		{
		case 2:
			sb << "very fragile";
			return;
		case 3:
			sb << "fragile";
			return;
		case 4:
			sb << "brittle";
			return;
		case 5:
			sb << "a little brittle";
			return;
		case 6:
			sb << "tough";
			return;
		case 7:
			sb << "quite tough";
			return;
		case 8:
			sb << "very tough";
			return;
		case 9:
			sb << "very very tough";
			return;
		case 10:
			sb << "of unmatched toughness";
			return;
		default:
			sb << "unphysically tough";
			return;
		}
	}
}

namespace HV
{
	void FormatEquipmentInfo(char* buffer, size_t capacity, IObjectPrototype& p)
	{
		StackStringBuilder sb(buffer, capacity);

		auto& melee = p.Melee();

		sb << p.ShortName() << "\n\n";
		sb << " * " << p.Description() << "\n";

		auto& dynamics = p.Dynamics();

		sb << "\n * Mass: ";
		AppendMass(sb, dynamics.mass);

		sb << "\n * Dimensions: ";
		AppendSpan(sb, dynamics.span);

		const auto& mats = p.Mats();

		sb << "\n * The material appears to be ";
		AtomicNumberToString(sb, mats.atomicNumber);
		sb << " - it is ";

		HardnessToString(sb, mats.mohsHardness);

		if ((mats.mohsHardness < 5 && mats.toughness < 5) ||
			(mats.mohsHardness >= 5 && mats.toughness >= 5))
		{
			sb << " and ";
		}
		else
		{
			sb << " but it is ";
		}
		ToughnessToString(sb, mats.toughness);

		sb << "\n";

		if (melee.baseDamage > 0)
		{
			if (melee.baseDamage < 2)
			{
				sb << " * The weapon is no better than a big wet blouse";
			}
			else if (melee.baseDamage < 8)
			{
				sb << " * The weapon is little better than a sharp stick";
			}
			else if (melee.baseDamage < 16)
			{
				sb << " * If you can get a hit in, this weapon would give you good odds in armed combat";
			}
			else if (melee.baseDamage < 24)
			{
				sb << " * The weapon looks like it could inflict a grevious wound";
			}
			else if (melee.baseDamage < 32)
			{
				sb << " * One blow from this weapon could fell any man";
			}
			else
			{
				sb << " * One blow from this weapon could split a horse in two";
			}
		}
	}
}