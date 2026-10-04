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
    // 1. SCHADENSBERECHNUNG (OnDamage_Hit Hook)
    // ==========================================================
    
    // FIX: Wir hooken nicht OnDamage, sondern OnDamage_Hit. 
    // Hier findet die tatsaechliche Ruestungsberechnung statt und das Daedalus-Symbol wird abgefragt!
    using TOnDamage_Hit = void (oCNpc::*)(oCNpc::oSDamageDescriptor&);
    void __fastcall Union_MinDamage_OnDamage_Hit(oCNpc* _this, void* vtable, oCNpc::oSDamageDescriptor& desc);
    
    auto Hook_Union_MinDamage_OnDamage_Hit = Union::CreateHook(
        SIGNATURE_OF( static_cast<TOnDamage_Hit>(&oCNpc::OnDamage_Hit) ), 
        &Union_MinDamage_OnDamage_Hit, 
        Union::HookType::Hook_Detours
    );

    void __fastcall Union_MinDamage_OnDamage_Hit(oCNpc* _this, void* vtable, oCNpc::oSDamageDescriptor& desc) {
        // INI-Werte auslesen
        int isDynamic    = zoptions->ReadInt("UNION_MINIMUM_DAMAGE", "DynamicMode", 1);
        int settingValue = zoptions->ReadInt("UNION_MINIMUM_DAMAGE", "MinDamageValue", 0);
        
        int targetMinDamage = 5; // Standard-Fallback
        zSTRING attackerName = "Niemand";
        zSTRING calcDetails = ""; 

        // Logik & Berechnung unseres dynamischen Min-Schadens
        if (isDynamic == 1) {
            int bonus = 0;
            
            if (desc.pNpcAttacker) {
                attackerName = desc.pNpcAttacker->name[0];
                bool isRanged = false;
                
                // Patrix-Fix: Prüfen der Waffe (verhindert den Weapon-Swap-Exploit)
                // ITM_CAT_FF = Fernkampfwaffe (Bogen/Armbrust), ITM_CAT_MUN = Munition (Pfeil/Bolzen)
                if (desc.pItemWeapon) {
                    isRanged = (desc.pItemWeapon->mainflag & ITM_CAT_FF) != 0 || (desc.pItemWeapon->mainflag & ITM_CAT_MUN) != 0;
                } 
                
                // Fallback, falls Waffe nicht eindeutig ist
                if (!isRanged) {
                    // FIX: In C++ ist der Fernkampf-Modus (FMODE_FAR) die 3!
                    isRanged = (desc.pNpcAttacker->fmode == 3);
                }
                
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

        // 2. DAEDALUS SYMBOL SETZEN (Direkt vor der Berechnung)
        zCPar_Symbol* sym = parser ? parser->GetSymbol("NPC_MINIMAL_DAMAGE") : nullptr;
        if (sym) {
            sym->single_intdata = targetMinDamage;
        }

        // 3. ORIGINALE BERECHNUNG AUSFUEHREN
        // Da wir OnDamage_Hit gehookt haben, liest die Engine nun exakt hier unser Symbol 
        // und nutzt es als Floor (Minimum), falls der Schaden an der Ruestung scheitert.
        Hook_Union_MinDamage_OnDamage_Hit(_this, vtable, desc);

        // 4. SOFORTIGE BEREINIGUNG
        // Wir setzen das Symbol im selben Frame sofort wieder auf 5 zurueck.
        // Dadurch blutet nichts in den naechsten Schlag und nichts ins Savegame!
        if (sym) {
            sym->single_intdata = 5;
        }

        // Debug-Ausgabe auf dem Bildschirm
        int debugMode = zoptions->ReadInt("UNION_MINIMUM_DAMAGE", "DebugMode", 0);
        if (debugMode > 0 && ogame && ogame->GetTextView()) {
            zSTRING targetName = _this ? _this->name[0] : zSTRING("Unbekannt");
            zSTRING screenMsg = "[MinDamage] " + attackerName + " -> " + targetName + " | " + calcDetails + " | Effektiver MinDmg = " + zSTRING(targetMinDamage);
            ogame->GetTextView()->Printwin(screenMsg);
        }
    }
}