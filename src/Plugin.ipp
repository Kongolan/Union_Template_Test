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
        // INI-Werte auslesen
        int isDynamic    = zoptions->ReadInt("UNION_MINIMUM_DAMAGE", "DynamicMode", 1);
        int settingValue = zoptions->ReadInt("UNION_MINIMUM_DAMAGE", "MinDamageValue", 0);
        
        int targetMinDamage = 5; // Standard-Fallback
        zSTRING attackerName = "Niemand";
        zSTRING calcDetails = ""; // Speichert die Rechenschritte fuer das On-Screen Debugging

        // Logik & Berechnung
        if (isDynamic == 1) {
            int bonus = 0;
            
            // Pruefen, ob es ueberhaupt einen Angreifer gibt (Koennte auch Fallschaden etc. sein)
            if (desc.pNpcAttacker) {
                attackerName = desc.pNpcAttacker->name[0];
                
                // Wir fragen direkt den "Fight-Mode" (fmode) des Angreifers ab. 
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

        // Daedalus-Symbol anpassen
        zCPar_Symbol* sym = parser ? parser->GetSymbol("NPC_MINIMAL_DAMAGE") : nullptr;
        int previousMinDamage = 5;

        if (sym) {
            previousMinDamage = sym->single_intdata;
            sym->single_intdata = targetMinDamage;
        }

        // Debug-Ausgabe auf dem Bildschirm
        int debugMode = zoptions->ReadInt("UNION_MINIMUM_DAMAGE", "DebugMode", 0);
        if (debugMode > 0 && ogame && ogame->GetTextView()) {
            zSTRING targetName = _this ? _this->name[0] : zSTRING("Unbekannt");
            zSTRING screenMsg = "[MinDamage] " + attackerName + " -> " + targetName + " | " + calcDetails + " | NPC_MINIMAL_DAMAGE = " + zSTRING(targetMinDamage);
            ogame->GetTextView()->Printwin(screenMsg);
        }

        // Originale Schadensberechnung ausfuehren
        Hook_Union_MinDamage_OnDamage(_this, vtable, desc);

        // Sofortige Bereinigung
        if (sym) {
            sym->single_intdata = previousMinDamage;
        }
    }

    // ==========================================================
    // 2. SICHERHEITSNETZ GEGEN BLEEDING BEIM LADEN
    // ==========================================================
    
    void ResetMinDamageSymbol() {
        if (parser) {
            zCPar_Symbol* sym = parser->GetSymbol("NPC_MINIMAL_DAMAGE");
            if (sym) {
                sym->single_intdata = 5;
            }
        }
    }

    // ==========================================================
    // 3. UNION HOOKS FUER LADE- UND LEVEL-EVENTS
    // ==========================================================

    // Hook: Neues Spiel laden
    void __fastcall oCGame_LoadGame(oCGame* self, void* vtable, int slot, const zSTRING& levelPath);
    auto Hook_oCGame_LoadGame = Union::CreateHook(SIGNATURE_OF(&oCGame::LoadGame), &oCGame_LoadGame, Union::HookType::Hook_Detours);
    void __fastcall oCGame_LoadGame(oCGame* self, void* vtable, int slot, const zSTRING& levelPath)
    {
        ResetMinDamageSymbol();
        Hook_oCGame_LoadGame(self, vtable, slot, levelPath);
    }

    // Hook: Spielstand laden (Savegame)
    void __fastcall oCGame_LoadSaveGame(oCGame* self, void* vtable, int slot, zBOOL loadGlobals);
    auto Hook_oCGame_LoadSaveGame = Union::CreateHook(SIGNATURE_OF(&oCGame::LoadSavegame), &oCGame_LoadSaveGame, Union::HookType::Hook_Detours);
    void __fastcall oCGame_LoadSaveGame(oCGame* self, void* vtable, int slot, zBOOL loadGlobals)
    {
        ResetMinDamageSymbol();
        Hook_oCGame_LoadSaveGame(self, vtable, slot, loadGlobals);
    }

    // Hook: Level wechseln (z.B. von Khorinis ins Minental)
    void __fastcall oCGame_ChangeLevel(oCGame* self, void* vtable, const zSTRING& levelpath, const zSTRING& startpoint);
    auto Hook_oCGame_ChangeLevel = Union::CreateHook(SIGNATURE_OF(&oCGame::ChangeLevel), &oCGame_ChangeLevel, Union::HookType::Hook_Detours);
    void __fastcall oCGame_ChangeLevel(oCGame* self, void* vtable, const zSTRING& levelpath, const zSTRING& startpoint)
    {
        ResetMinDamageSymbol();
        Hook_oCGame_ChangeLevel(self, vtable, levelpath, startpoint);
    }

    // Hook: Trigger-Level-Change (Teleport/Zonenwechsel)
    void __fastcall oCGame_TriggerChangeLevel(oCGame* self, void* vtable, const zSTRING& levelpath, const zSTRING& startpoint);
    auto Hook_oCGame_TriggerChangeLevel = Union::CreateHook(SIGNATURE_OF(&oCGame::TriggerChangeLevel), &oCGame_TriggerChangeLevel, Union::HookType::Hook_Detours);
    void __fastcall oCGame_TriggerChangeLevel(oCGame* self, void* vtable, const zSTRING& levelpath, const zSTRING& startpoint)
    {
        ResetMinDamageSymbol();
        Hook_oCGame_TriggerChangeLevel(self, vtable, levelpath, startpoint);
    }
}