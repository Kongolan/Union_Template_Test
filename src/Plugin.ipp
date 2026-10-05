// This file is included separately for each engine version

namespace GOTHIC_NAMESPACE
{
    // ==========================================================
    // EIGENE LOGGING-FUNKTION (Fuer detaillierte zSpy-Logs im Hintergrund)
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
    
    // ZURUECK ZU ONDAMAGE: Hier liest die Engine das Symbol für die Ruestungsberechnung aus!
    using TOnDamage = void (oCNpc::*)(oCNpc::oSDamageDescriptor&);
    void __fastcall Union_MinDamage_OnDamage(oCNpc* _this, void* vtable, oCNpc::oSDamageDescriptor& desc);
    
    auto Hook_Union_MinDamage_OnDamage = Union::CreateHook(
        SIGNATURE_OF( static_cast<TOnDamage>(&oCNpc::OnDamage) ), 
        &Union_MinDamage_OnDamage, 
        Union::HookType::Hook_Detours
    );

    void __fastcall Union_MinDamage_OnDamage(oCNpc* _this, void* vtable, oCNpc::oSDamageDescriptor& desc) {
        // INI-Werte auslesen
        int isDynamic    = zoptions->ReadInt("UNION_MINIMUM_DAMAGE", "DynamicMode", 1);
        int settingValue = zoptions->ReadInt("UNION_MINIMUM_DAMAGE", "MinDamageValue", 0);
        
        int targetMinDamage = 5; // Standard-Fallback
        zSTRING attackerName = "Niemand";
        zSTRING targetName = _this ? _this->name[0] : zSTRING("Unbekannt");

        // 2. DAEDALUS SYMBOL SETZEN (Exakt bevor die Engine die Ruestung abzieht)
        zCPar_Symbol* sym = parser ? parser->GetSymbol("NPC_MINIMAL_DAMAGE") : nullptr;
        if (!sym) {
            LogDebug("Fehler NPC_MINIMAL_DAMAGE kann nicht gelesen werden!");
            return;
        }

        if (isDynamic != 1) {
            // Modus ist nicht dynamisch -> Wir nutzen den fixen Wert aus der INI
            targetMinDamage = settingValue;
            sym->single_intdata = settingValue;
            LogDebug("Festwert (INI): " + zSTRING(settingValue));
            return;
        }

        // Logik & Berechnung unseres dynamischen Min-Schadens
        if (!desc.pNpcAttacker) {
            LogDebug("Kein Angreifer (Basiswert 5)");
            sym->single_intdata = 5;
            return;
        }

        attackerName = desc.pNpcAttacker->name[0];
        
        // Direkte Auswertung der Waffe und sofortige Bonus-Berechnung
        if (desc.pItemWeapon && ((desc.pItemWeapon->mainflag & ITM_CAT_FF) || (desc.pItemWeapon->mainflag & ITM_CAT_MUN))) {
            // Zweig 1: Eindeutige Fernkampfwaffe ODER abgefeuertes Projektil
            int dex = desc.pNpcAttacker->attribute[NPC_ATR_DEXTERITY];
            int bonus = (dex / 10) - 1;
            zSTRING calcDetails = "Fernkampf Waffe (DEX: " + zSTRING(dex) + ") -> 5 + " + zSTRING(bonus);
            int targetMinDamage = 5 + bonus;
            // Setzt NPC_MINIMAL_DAMAGE Wert
            sym->single_intdata = targetMinDamage;
            LogDebug(attackerName + " -> " + targetName + " | " + calcDetails + " = " + zSTRING(targetMinDamage));  
            Hook_Union_MinDamage_OnDamage(_this, vtable, desc);
            return;                  
        } else if (desc.pItemWeapon && (desc.pItemWeapon->mainflag & ITM_CAT_NF)) {
            // Zweig 2: Eindeutige Nahkampfwaffe (Schwert/Axt/etc)
            int str = desc.pNpcAttacker->attribute[NPC_ATR_STRENGTH];
            int bonus = (str / 10) - 1;
            zSTRING calcDetails = "Nahkampf Waffe (STR: " + zSTRING(str) + ") -> 5 + " + zSTRING(bonus);
            int targetMinDamage = 5 + bonus;
            // Setzt NPC_MINIMAL_DAMAGE Wert
            sym->single_intdata = targetMinDamage;
            LogDebug(attackerName + " -> " + targetName + " | " + calcDetails + " = " + zSTRING(targetMinDamage));   
            return;
        } else {
            // Zweig 3: Fallback (Monsterangriffe, Faeuste, Magie oder unerkannte Waffen)
            int str = desc.pNpcAttacker->attribute[NPC_ATR_STRENGTH];
            int bonus = (str / 10) - 1;
            zSTRING calcDetails = "Monster/Ohne Waffe (STR: " + zSTRING(str) + ") -> 5 + " + zSTRING(bonus);
            int targetMinDamage = 5 + bonus;
            // Setzt NPC_MINIMAL_DAMAGE Wert
            sym->single_intdata = targetMinDamage;
            LogDebug(attackerName + " -> " + targetName + " | " + calcDetails + " = " + zSTRING(targetMinDamage));   
        }

        // 3. ORIGINALE BERECHNUNG AUSFUEHREN
        // Da wir OnDamage gehookt haben, liest die Engine JETZT unser Symbol aus und wendet den Floor an.
        // Hook_Union_MinDamage_OnDamage(_this, vtable, desc);

        // 4. SOFORTIGE BEREINIGUNG (Anti-Bleeding)
        // Wir erzwingen IMMER eine saubere 5. Egal, was vorher im Savegame stand.
        // if (sym) {
        //     sym->single_intdata = 5;
        // }
    }
}