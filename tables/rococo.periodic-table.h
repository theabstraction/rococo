#pragma once
// Generated by rococo.carpenter. Timestamp: 28/05/2022 20:59:41
// Excel Source: C:\work\rococo\tables\periodic-table.xlsx

#include <rococo.types.h>

namespace Rococo::SI
{
    typedef float ElectronVolts;
}

namespace Rococo::SI
{
    typedef float Kelvin;
}

namespace Rococo::Science::Materials
{
    using namespace Rococo::SI;

    enum class ElementName: int
    {
        None, Hydrogen, Helium, Lithium, Beryllium, Boron, Carbon, Nitrogen, Oxygen, Fluorine, Neon, Sodium,
        Magnesium, Aluminum, Silicon, Phosphorus, Sulfur, Chlorine, Argon, Potassium, Calcium, Scandium, Titanium, Vanadium,
        Chromium, Manganese, Iron, Cobalt, Nickel, Copper, Zinc, Gallium, Germanium, Arsenic, Selenium, Bromine,
        Krypton, Rubidium, Strontium, Yttrium, Zirconium, Niobium, Molybdenum, Technetium, Ruthenium, Rhodium, Palladium, Silver,
        Cadmium, Indium, Tin, Antimony, Tellurium, Iodine, Xenon, Cesium, Barium, Lanthanum, Cerium, Praseodymium,
        Neodymium, Promethium, Samarium, Europium, Gadolinium, Terbium, Dysprosium, Holmium, Erbium, Thulium, Ytterbium, Lutetium,
        Hafnium, Tantalum, Wolfram, Rhenium, Osmium, Iridium, Platinum, Gold, Mercury, Thallium, Lead, Bismuth,
        Polonium, Astatine, Radon, Francium, Radium, Actinium, Thorium, Protactinium, Uranium, Neptunium, Plutonium, Americium,
        Curium, Berkelium, Californium, Einsteinium, Fermium, Mendelevium, Nobelium, Lawrencium, Rutherfordium, Dubnium, Seaborgium, Bohrium,
        Hassium, Meitnerium, Darmstadtium, Roentgenium, Copernicium, Nihonium, Flerovium, Moscovium, Livermorium, Tennessine, Oganesson
    };

    bool TryParse(const fstring& text, ElementName& result);
    fstring ToString(ElementName value);


    enum class ElementSymbol: int
    {
        None, H, He, Li, Be, B, C, N, O, F, Ne, Na,
        Mg, Al, Si, P, S, Cl, Ar, K, Ca, Sc, Ti, V,
        Cr, Mn, Fe, Co, Ni, Cu, Zn, Ga, Ge, As, Se, Br,
        Kr, Rb, Sr, Y, Zr, Nb, Mo, Tc, Ru, Rh, Pd, Ag,
        Cd, In, Sn, Sb, Te, I, Xe, Cs, Ba, La, Ce, Pr,
        Nd, Pm, Sm, Eu, Gd, Tb, Dy, Ho, Er, Tm, Yb, Lu,
        Hf, Ta, W, Re, Os, Ir, Pt, Au, Hg, Tl, Pb, Bi,
        Po, At, Rn, Fr, Ra, Ac, Th, Pa, U, Np, Pu, Am,
        Cm, Bk, Cf, Es, Fm, Md, No, Lr, Rf, Db, Sg, Bh,
        Hs, Mt, Ds, Rg, Cn, Nh, Fl, Mc, Lv, Ts, Og
    };

    bool TryParse(const fstring& text, ElementSymbol& result);
    fstring ToString(ElementSymbol value);


    enum class ElementType: int
    {
        None, Nonmetal, NobleGas, AlkaliMetal, AlkalineEarthMetal, Metalloid, Halogen, Metal, TransitionMetal, Lanthanide, Actinide, Transactinide
    };

    bool TryParse(const fstring& text, ElementType& result);
    fstring ToString(ElementType value);

    struct PeriodicTableRow
    {
        int32 atomicNumber;
        ElementName element;
        ElementSymbol symbol;
        float atomicMass;
        bool metal;
        ElementType elementType;
        float electroNegativity;
        ElectronVolts firstIonization;
        float density;
        Kelvin meltingPoint;
        Kelvin boilingPoint;
    };
}

#include "tables.sxh.h"

namespace Rococo::Science::Materials
{
    ROCOCOAPI IPeriodicTable_MetaData
    {
        virtual fstring GetTitle() const = 0;
        virtual fstring GetOwner() const = 0;
        virtual fstring GetURL() const = 0;
    };

    ROCOCOAPI IPeriodicTable: protected IPeriodicTable_Sexy
    {
        virtual const PeriodicTableRow& GetRow(int32 index) const = 0;
        virtual const int32 NumberOfRows() const = 0;
        // load a table from a binary archive. If [tablePingPath] is null, defaults to !tables/rococo.periodic-table.Elements_Table.bin
        virtual void Load(const IInstallation& installation, cstr tablePingPath = nullptr) = 0;
        virtual const IPeriodicTable_MetaData& Meta() const = 0;
    };

    ROCOCOAPI IPeriodicTableSupervisor: IPeriodicTable
    {
        virtual void Free() = 0;
    };

    IPeriodicTableSupervisor* GetPeriodicTable();
}