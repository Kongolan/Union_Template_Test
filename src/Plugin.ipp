// This file is included separately for each engine version

namespace GOTHIC_NAMESPACE
{
    // ==========================================================
    // EIGENE LOGGING-FUNKTION
    // ==========================================================
    void LogDebug(const zSTRING& text) {
        int debugMode = zoptions->ReadInt("UNION_MINIMUM_DAMAGE", "DebugMode", 0);
        if (debugMode != 1) return;
        
        zerr->Message("[MinDamage] " + text);

        // Debug-Ausgabe auf dem Bildschirm
        if (!ogame || !ogame->GetTextView()) return;
        
        ogame->GetTextView()->Printwin("[MinDamage] " + text);
    }

    // ==========================================================
    // 1. SCHADENSBERECHNUNG (OnDamage Root Hook)
    // ==========================================================
    
    // ZURUECK ZU ONDAMAGE: Wir greifen ein, bevor die Engine interne Caches baut!
    using TOnDamage = void (oCNpc::*)(oCNpc::oSDamageDescriptor&);
    void __fastcall Union_MinDamage_OnDamage(oCNpc* _this, void* vtable, oCNpc::oSDamageDescriptor& desc);
    
    auto Hook_Union_MinDamage_OnDamage = Union::CreateHook(
        SIGNATURE_OF( static_cast<TOnDamage>(&oCNpc::OnDamage) ), 
        &Union_MinDamage_OnDamage, 
        Union::HookType::Hook_Detours
    );

    void __fastcall Union_MinDamage_OnDamage(oCNpc* _this, void* vtable, oCNpc::oSDamageDescriptor& desc) {
        // INI-Werte auslesen
        int isDynamic = zoptions->ReadInt("UNION_MINIMUM_DAMAGE", "DynamicMode", 1);
        int targetMinDamage = 5; // Standard-Fallback
        zSTRING calcDetails = "";
        
        zSTRING attackerName = desc.pNpcAttacker ? desc.pNpcAttacker->name[0] : zSTRING("Niemand");
        zSTRING targetName = _this ? _this->name[0] : zSTRING("Unbekannt");

        // Guard Clause 1: Festwert-Modus
        if (isDynamic != 1) {
            targetMinDamage = zoptions->ReadInt("UNION_MINIMUM_DAMAGE", "MinDamageValue", 0);
            calcDetails = "Festwert (INI)";
        }
        // Guard Clause 2: Kein Angreifer vorhanden (z.B. Fallschaden, Skripttod)
        else if (!desc.pNpcAttacker) {
            calcDetails = "Kein Angreifer (Basiswert 5)";
        }
        // Hauptlogik: Wir haben einen Angreifer und der Modus ist dynamisch
        else {
            int bonus = 0;
            
            // 1. ZWEIG: FERNKAMPF (Bogen, Armbrust, Munition)
            if (desc.pItemWeapon && ((desc.pItemWeapon->mainflag & ITM_CAT_FF) || (desc.pItemWeapon->mainflag & ITM_CAT_MUN))) {
                int dex = desc.pNpcAttacker->attribute[NPC_ATR_DEXTERITY];
                bonus = (dex / 10) - 1;
                calcDetails = "Fernkampf Waffe (DEX: " + zSTRING(dex) + ") -> 5 + " + zSTRING(bonus);
            } 
            // 2. ZWEIG: NAHKAMPF (Schwert, Axt, etc.)
            else if (desc.pItemWeapon && (desc.pItemWeapon->mainflag & ITM_CAT_NF)) {
                int str = desc.pNpcAttacker->attribute[NPC_ATR_STRENGTH];
                bonus = (str / 10) - 1;
                calcDetails = "Nahkampf Waffe (STR: " + zSTRING(str) + ") -> 5 + " + zSTRING(bonus);
            } 
            // 3. ZWEIG: MAGIE (Zauber-ID vorhanden ODER Schadenstyp ist Feuer(8) / Magie(32))
            else if (desc.nSpellID > 0 || (desc.enuModeDamage & 8) || (desc.enuModeDamage & 32)) {
                int magicMode = zoptions->ReadInt("UNION_MINIMUM_DAMAGE", "MagicMode", 1);
                
                if (magicMode == 0) {
                    // Vanilla: Prallt komplett ab (0 Schaden)
                    bonus = -5; // Loescht den Basiswert von 5 aus
                    calcDetails = "Magie (Vanilla) -> 0";
                }
                else if (magicMode == 1) {
                    // Max Mana
                    int maxMana = desc.pNpcAttacker->attribute[NPC_ATR_MANAMAX];
                    bonus = (maxMana / 10) - 1;
                    calcDetails = "Magie (Max Mana: " + zSTRING(maxMana) + ") -> 5 + " + zSTRING(bonus);
                }
                else if (magicMode == 2) {
                    // Kinetisch (Aktuelles Mana)
                    int curMana = desc.pNpcAttacker->attribute[NPC_ATR_MANA];
                    bonus = (curMana / 10) - 1;
                    calcDetails = "Magie (Aktuelles Mana: " + zSTRING(curMana) + ") -> 5 + " + zSTRING(bonus);
                }
                else { 
                    // Modus 3: 10% des magischen Rohschadens
                    int rawDmg = (int)desc.fDamageTotal;
                    bonus = (rawDmg / 10) - 5; // Die -5 zieht den Standard-Basiswert ab, damit exakt 10% rauskommen
                    calcDetails = "Magie (10% Rohschaden: " + zSTRING(rawDmg) + ") -> 5 + " + zSTRING(bonus);
                }
            }
            // 4. ZWEIG: FALLBACK (Monster ohne Waffen, Faustkampf)
            else {
                int str = desc.pNpcAttacker->attribute[NPC_ATR_STRENGTH];
                bonus = (str / 10) - 1;
                calcDetails = "Monster/Ohne Waffe (STR: " + zSTRING(str) + ") -> 5 + " + zSTRING(bonus);
            }
            
            targetMinDamage = 5 + bonus;
        }

        // Sicherstellen, dass MinDamage niemals negativ wird
        if (targetMinDamage < 0) {
            targetMinDamage = 0;
            calcDetails += " (korrigiert auf 0)";
        }

        // ==========================================================
        // AUSFUEHRUNG: DER CACHE-FIX (GRATUŚ)
        // ==========================================================
        
        // Die G2A-Engine cacht NPC_MINIMAL_DAMAGE hart im Speicher.
        // Wir ueberschreiben exakt diese Cache-Adresse direkt.
        // zSwitch(G1, G1A, G2, G2A) verhindert Abstuerze auf anderen Gothic-Versionen.
        int cacheAddress = zSwitch(0, 0, 0, 0x00AAC610);
        if (cacheAddress != 0) {
            int& min_damage_cache = *reinterpret_cast<int*>(cacheAddress);
            min_damage_cache = targetMinDamage;
        }
        LogDebug(attackerName + " -> " + targetName + " | " + calcDetails + " = " + zSTRING(targetMinDamage));

        // Parallel das Daedalus-Symbol ueberschreiben (fuer G1, den ersten Pfeil oder andere Mods ohne Cache)
        zCPar_Symbol* sym = parser ? parser->GetSymbol("NPC_MINIMAL_DAMAGE") : nullptr;
        if (sym) {
            sym->single_intdata = targetMinDamage;
        }

        // Originale Engine-Berechnung ausfuehren. 
        // Die Engine greift nun auf unseren erzwungenen Cache-Speicher zu.
        Hook_Union_MinDamage_OnDamage(_this, vtable, desc);

        // HINWEIS: Ein Zuruecksetzen auf 5 ist nicht mehr noetig.
        // Da dieser Hook bei absolut jedem Treffer ausloest, ist der Speicher 
        // ohnehin immer exakt mit dem Wert gefuellt, der fuer den aktuellen Schlag berechnet wurde.
    }
}