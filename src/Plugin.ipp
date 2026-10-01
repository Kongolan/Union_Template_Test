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
            
            // Pruefen, ob es ueberhaupt einen Angreifer gibt (Koennte auch Fallschaden etc. sein)
            if (desc.pNpcAttacker) {
                attackerName = desc.pNpcAttacker->name[0];
                
                // FIX: 'desc.enuModeWeapon' ist bei Projektilen oft leer/unzuverlaessig.
                // Wir fragen stattdessen direkt den "Fight-Mode" (fmode) des Angreifers ab. 
                // Zieht er gerade einen Bogen oder eine Armbrust, ist es sicher ein Fernkampftreffer.
                int weaponMode = desc.pNpcAttacker->fmode;
                bool isRanged = (weaponMode == NPC_WEAPON_BOW || weaponMode == NPC_WEAPON_CBOW);
                
                if (isRanged) {
                    // Fernkampf: Skaliert mit Geschicklichkeit (DEX)
                    int dex = desc.pNpcAttacker->attribute[NPC_ATR_DEXTERITY];
                    bonus = (dex / 10) - 1;
                    calcDetails = "Fernkampf (DEX: " + zSTRING(dex) + ") -> 5 + " + zSTRING(bonus);
                } else {
                    // Nahkampf (oder Magie/Faeuste): Skaliert mit Staerke (STR)
                    int str = desc.pNpcAttacker->attribute[NPC_ATR_STRENGTH];
                    bonus = (str / 10) - 1;
                    calcDetails = "Nahkampf (STR: " + zSTRING(str) + ") -> 5 + " + zSTRING(bonus);
                }
            } else {
                calcDetails = "Kein Angreifer (Basiswert)";
            }
            
            // Minimalschaden zusammensetzen und sicherstellen, dass er nicht negativ wird
            targetMinDamage = 5 + bonus;
            if (targetMinDamage < 0) targetMinDamage = 0;
            
        } else {
            // Modus ist nicht dynamisch -> Wir nutzen den fixen Wert aus der INI
            calcDetails = "Festwert (INI)";
            targetMinDamage = settingValue;
        }

        // 3. DER GOTHIC-PIPELINE-HACK: 
        // Wir fassen das Daedalus-Symbol nicht mehr an!
        // Die Engine hat den Vanilla-Schaden zu diesem Zeitpunkt schon berechnet.
        // Wir lesen das Endergebnis aus und ueberschreiben es direkt im Speicher.
        unsigned long actualDamage = desc.dwDamageTotal;
        
        if (actualDamage < (unsigned long)targetMinDamage) {
            desc.dwDamageTotal = targetMinDamage;
            calcDetails += " | Ueberschrieben: " + zSTRING((int)actualDamage) + " -> " + zSTRING(targetMinDamage);
        } else {
            calcDetails += " | Ignoriert (Regulaerer Schaden " + zSTRING((int)actualDamage) + " ist hoeher)";
        }

        // 4. Transparente Ausgabe auf dem Bildschirm
        int debugMode = zoptions->ReadInt("UNION_MINIMUM_DAMAGE", "DebugMode", 0);
        if (debugMode > 0 && ogame && ogame->GetTextView()) {
            zSTRING targetName = _this ? _this->name[0] : zSTRING("Unbekannt");
            zSTRING screenMsg = "[MinDamage] " + attackerName + " -> " + targetName + " | " + calcDetails;
            ogame->GetTextView()->Printwin(screenMsg);
        }

        // 5. Originale Funktion ausfuehren (zieht nun exakt unseren ueberschriebenen desc.dwDamageTotal vom Leben ab)
        Hook_Union_MinDamage_OnDamage(_this, vtable, desc);
    }
}