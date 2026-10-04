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
        zSTRING calcDetails = ""; // Speichert die Rechenschritte fuer das On-Screen Debugging

        // Logik & Berechnung unseres dynamischen Min-Schadens
        if (isDynamic == 1) {
            int bonus = 0;
            
            if (desc.pNpcAttacker) {
                attackerName = desc.pNpcAttacker->name[0];
                
                // Direkte Auswertung der Waffe und sofortige Bonus-Berechnung
                if (desc.pItemWeapon && ((desc.pItemWeapon->mainflag & ITM_CAT_FF) || (desc.pItemWeapon->mainflag & ITM_CAT_MUN))) {
                    // Zweig 1: Eindeutige Fernkampfwaffe ODER abgefeuertes Projektil
                    int dex = desc.pNpcAttacker->attribute[NPC_ATR_DEXTERITY];
                    bonus = (dex / 10) - 1;
                    calcDetails = "Fernkampf Waffe (DEX: " + zSTRING(dex) + ") -> 5 + " + zSTRING(bonus);
                    
                } else if (desc.pItemWeapon && (desc.pItemWeapon->mainflag & ITM_CAT_NF)) {
                    // Zweig 2: Eindeutige Nahkampfwaffe (Schwert/Axt/etc)
                    int str = desc.pNpcAttacker->attribute[NPC_ATR_STRENGTH];
                    bonus = (str / 10) - 1;
                    calcDetails = "Nahkampf Waffe (STR: " + zSTRING(str) + ") -> 5 + " + zSTRING(bonus);
                    
                } else {
                    // Zweig 3: Fallback (Monsterangriffe, Faeuste, Magie oder unerkannte Waffen)
                    int str = desc.pNpcAttacker->attribute[NPC_ATR_STRENGTH];
                    bonus = (str / 10) - 1;
                    calcDetails = "Monster/Ohne Waffe (STR: " + zSTRING(str) + ") -> 5 + " + zSTRING(bonus);
                }
            } else {
                calcDetails = "Kein Angreifer (Basiswert 5)";
            }
            
            // Minimalschaden zusammensetzen und sicherstellen, dass er nicht negativ wird
            targetMinDamage = 5 + bonus;
            if (targetMinDamage < 0) {
                targetMinDamage = 0;
                calcDetails += " (korrigiert auf 0)";
            }
        } else {
            // Modus ist nicht dynamisch -> Wir nutzen den fixen Wert aus der INI
            calcDetails = "Festwert (INI)";
            targetMinDamage = settingValue;
        }

        // 2. DAEDALUS SYMBOL SETZEN (Exakt bevor die Engine die Ruestung abzieht)
        zCPar_Symbol* sym = parser ? parser->GetSymbol("NPC_MINIMAL_DAMAGE") : nullptr;
        if (sym) {
            sym->single_intdata = targetMinDamage;
        }

        // Debug-Ausgabe auf dem Bildschirm
        int debugMode = zoptions->ReadInt("UNION_MINIMUM_DAMAGE", "DebugMode", 0);
        if (debugMode > 0 && ogame && ogame->GetTextView()) {
            zSTRING targetName = _this ? _this->name[0] : zSTRING("Unbekannt");
            zSTRING screenMsg = "[MinDamage] " + attackerName + " -> " + targetName + " | " + calcDetails + " | Effektiver MinDmg = " + zSTRING(targetMinDamage);
            ogame->GetTextView()->Printwin(screenMsg);
        }

        // 3. ORIGINALE BERECHNUNG AUSFUEHREN
        // Da wir OnDamage gehookt haben, liest die Engine JETZT unser Symbol aus und wendet den Floor an.
        Hook_Union_MinDamage_OnDamage(_this, vtable, desc);

        // 4. SOFORTIGE BEREINIGUNG (Anti-Bleeding)
        // Wir erzwingen IMMER eine saubere 5. Egal, was vorher im Savegame stand.
        if (sym) {
            sym->single_intdata = 5;
        }
    }
}