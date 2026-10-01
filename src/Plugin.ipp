// This file is included separately for each engine version

namespace GOTHIC_NAMESPACE
{
    // ==========================================================
    // EIGENE LOGGING-FUNKTION (Fuer detaillierte zSpy-Logs im Hintergrund)
    // ==========================================================
    void LogDebug(const zSTRING& text) {
        int debugMode = zoptions->ReadInt("UNION_MINIMUM_DAMAGE", "DebugMode", 0);
        if (debugMode > 0) {
            zerr->Message("[MinDamage] " + text);
        }
    }

    // ==========================================================
    // 1. SCHADENSBERECHNUNG (OnDamage Root Hook)
    // ==========================================================
    
    using TOnDamage = void (oCNpc::*)(oCNpc::oSDamageDescriptor&);
    void __fastcall Union_MinDamage_OnDamage(oCNpc* _this, void* vtable, oCNpc::oSDamageDescriptor& desc);
    
    auto Hook_Union_MinDamage_OnDamage = Union::CreateHook(
        SIGNATURE_OF( static_cast<TOnDamage>(&oCNpc::OnDamage) ), 
        &Union_MinDamage_OnDamage, 
        Union::HookType::Hook_Detours
    );

    void __fastcall Union_MinDamage_OnDamage(oCNpc* _this, void* vtable, oCNpc::oSDamageDescriptor& desc) {
        // 1. INI-Werte auslesen
        int isDynamic    = zoptions->ReadInt("UNION_MINIMUM_DAMAGE", "DynamicMode", 1);
        int settingValue = zoptions->ReadInt("UNION_MINIMUM_DAMAGE", "MinDamageValue", 0);
        
        int targetMinDamage = 0;
        zSTRING attackerName = "Niemand";
        zSTRING calcDetails = ""; // Speichert die Rechenschritte fuer das On-Screen Debugging

        // 2. Logik & Berechnung
        if (isDynamic == 1) {
            int bonus = 0;
            if (desc.pNpcAttacker) {
                attackerName = desc.pNpcAttacker->name[0];
                bool isRanged = (desc.enuModeWeapon == NPC_WEAPON_BOW || desc.enuModeWeapon == NPC_WEAPON_CBOW);
                
                if (isRanged) {
                    int dex = desc.pNpcAttacker->attribute[NPC_ATR_DEXTERITY];
                    bonus = (dex / 10) - 1;
                    calcDetails = "Fernkampf (DEX: " + zSTRING(dex) + ") -> 5 + " + zSTRING(bonus);
                } else {
                    int str = desc.pNpcAttacker->attribute[NPC_ATR_STRENGTH];
                    bonus = (str / 10) - 1;
                    calcDetails = "Nahkampf (STR: " + zSTRING(str) + ") -> 5 + " + zSTRING(bonus);
                }
            } else {
                calcDetails = "Kein Angreifer (Basiswert)";
            }
            
            targetMinDamage = 5 + bonus;
            if (targetMinDamage < 0) {
                targetMinDamage = 0;
                calcDetails += " (korrigiert auf 0)";
            }
            
        } else {
            calcDetails = "Festwert (INI)";
            targetMinDamage = settingValue;
        }

        // 3. Daedalus-Symbol ueberschreiben
        zCPar_Symbol* sym = parser->GetSymbol("NPC_MINIMAL_DAMAGE");
        if (sym) {
            sym->single_intdata = targetMinDamage;
        }

        // 4. Transparente Ausgabe der Berechnungswege auf dem Bildschirm (nur im DebugMode)
        int debugMode = zoptions->ReadInt("UNION_MINIMUM_DAMAGE", "DebugMode", 0);
        if (debugMode > 0 && ogame && ogame->GetTextView()) {
            zSTRING targetName = _this ? _this->name[0] : "Unbekannt";
            zSTRING screenMsg = "[MinDamage] " + attackerName + " -> " + targetName + " | " + calcDetails + " = " + zSTRING(targetMinDamage);
            ogame->GetTextView()->Printwin(screenMsg);
        }

        // 5. Originale Schadensberechnung der Engine ausfuehren
        Hook_Union_MinDamage_OnDamage(_this, vtable, desc);
    }
}