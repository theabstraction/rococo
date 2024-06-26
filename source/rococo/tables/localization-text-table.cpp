// Generated by rococo.carpenter. Timestamp: 01/05/2023 19:05:58
// Excel Source: C:\work\rococo\tables\XL\localization-text-table.xlsx

#include "tables\localization-text-table.h"

#include <rococo.api.h>
using namespace Rococo;
using namespace Rococo::Strings;
using namespace Rococo::Strings;

static const LocalizedTextRow defaultRows[2] = 
{
    { // #0
        TextId::None, to_fstring((cstr)""), to_fstring((cstr)"")
    },
    { // #1
        TextId::Introduction_MarcusAndronicus, to_fstring((cstr)u8R"[RAW](Princes, that strive by factions and by friends
Ambitiously for rule and empery,
Know that the people of Rome, for whom we stand
A special party, have, by common voice,
In election for the Roman empery,
Chosen Andronicus, surnamed Pius
For many good and great deserts to Rome:
A nobler man, a braver warrior,
Lives not this day within the city walls:
He by the senate is accit'd home
From weary wars against the barbarous Goths;
That, with his sons, a terror to our foes,
Hath yoked a nation strong, train'd up in arms.
Ten years are spent since first he undertook
This cause of Rome and chastised with arms
Our enemies' pride: five times he hath return'd
Bleeding to Rome, bearing his valiant sons
In coffins from the field;
And now at last, laden with horror's spoils,
Returns the good Andronicus to Rome,
Renowned Titus, flourishing in arms.
Let us entreat, by honour of his name,
Whom worthily you would have now succeed.
And in the Capitol and senate's right,
Whom you pretend to honour and adore,
That you withdraw you and abate your strength;
Dismiss your followers and, as suitors should,
Plead your deserts in peace and humbleness.)[RAW]"), to_fstring((cstr)u8R"[RAW](Prinzen, die nach Parteien und Freunden streben
Ehrgeizig für Herrschaft und Herrschaft,
Wisse, dass die Menschen in Rom, für die wir stehen
Eine besondere Party, haben, mit gemeinsamer Stimme,
Bei der Wahl zum römischen Kaisertum
Auserwählter Andronicus, Beiname Pius
Für viele gute und große Wüsten nach Rom:
Ein edlerer Mann, ein tapferer Krieger,
Lebt heute nicht innerhalb der Stadtmauern:
Er wird vom Senat nach Hause geschickt
Von müden Kriegen gegen die barbarischen Goten;
Das mit seinen Söhnen ein Schrecken für unsere Feinde,
Hat ein starkes Volk unterjocht, zu Waffen erzogen.
Zehn Jahre sind seit dem ersten Unterfangen vergangen
Diese Ursache von Rom und mit Waffen gezüchtigt
Der Stolz unserer Feinde: fünfmal ist er zurückgekehrt
Nach Rom blutend, seine tapferen Söhne tragend
In Särgen vom Feld;
Und nun endlich, beladen mit Schreckensbeute,
Bringt den guten Andronicus nach Rom zurück,
Der berühmte Titus, der in Waffen blüht.
Lasst uns zu Ehren seines Namens bitten,
Wem würdig hättest du jetzt Erfolg gehabt.
Und rechts vom Kapitol und Senat
Wen du vorgibst zu ehren und zu verehren,
Dass du dich zurückziehst und deine Kraft nachlässt;
Entlassen Sie Ihre Anhänger und, wie es Bewerber tun sollten,
Verteidige deine Wüsten in Frieden und Demut.)[RAW]")
    }
};

namespace ANON
{
    using namespace Rococo::Strings;

    struct LocalizedText_Table_Implementation: ILocalizedText, ILocalizedText_MetaData, private ILocalizedText_Sexy
    {
        Rococo::Strings::ILocalizedText_Sexy& GetSexyInterface() override
        {
            return *this;
        }

        const LocalizedTextRow& GetRow(int32 index) const override
        {
            return defaultRows[index];
        }

        // Sexy Interface Method
        void GetRow(int32 index, struct LocalizedTextRowSexy& row) override
        {
            if (index < 0 || index > NumberOfRows()) Throw(0, "%s: [index] out of range.", __FUNCTION__);
            const auto& nativeRow = GetRow(index);
            row.id = nativeRow.id;
            Rococo::Script::PopulateStringBuilder(row.english, nativeRow.english);
            Rococo::Script::PopulateStringBuilder(row.german, nativeRow.german);
        }

        const int32 NumberOfRows() const override
        {
            return 2;
        }

        // Sexy Interface Method
        int32 NumberOfRows() override
        {
            return 2;
        }

        const LocalizedTextRow* begin() const override
        {
            return defaultRows;
        }

        const LocalizedTextRow* end() const override
        {
            return defaultRows + 2;
        }

        const ILocalizedText_MetaData& Meta() const override
        {
            return *this;
        }

        fstring GetTitle() const override
        {
            return "Localization Table"_fstring;
        }

        fstring GetOwner() const override
        {
            return "Mark Anthony Taylor"_fstring;
        }
    };
}

namespace
{
    static ANON::LocalizedText_Table_Implementation globalInstance;
}

namespace Rococo::Strings
{
    ILocalizedText& LocalizedText()
    {
        return globalInstance;
    }
}

Rococo::Strings::ILocalizedText_Sexy* FactoryConstructRococoStringsLocalizedText(Rococo::IInstallation* installation)
{
    return &globalInstance.GetSexyInterface();
}

#include <string.h>
namespace Rococo::Strings
{
    fstring ToString(TextId value)
    {
        using enum TextId;

        switch(value)
        {
        case None: return "None"_fstring;
        case Introduction_MarcusAndronicus: return "Introduction_MarcusAndronicus"_fstring;
        default: return {nullptr,0};
        }
    }

    bool AppendString(TextId value, IStringPopulator& populator)
    {
        fstring s = ToString(value);
        if (s.length == 0) return false;
        populator.Populate(s);
        return true;
    }

    bool TryParse(const fstring& text, TextId& result)
    {
        using enum TextId;
        struct Binding { cstr key; TextId value; }; 
        static Binding bindings[] = {
            {"None", None},{"Introduction_MarcusAndronicus", Introduction_MarcusAndronicus}
        };

        for(auto& b: bindings)
        {
            if (strcmp(b.key, text) == 0)
            {
                result = b.value;
                return true;
            }
        }
        result = TextId::None;
        return false;
    }

    tuple<boolean32,TextId> TryParseTextId(const fstring& text)
    {
        TextId value = TextId();
        boolean32 wasFound = TryParse(text, value) ? 1 : 0;
        return { wasFound, value };
    }
}

