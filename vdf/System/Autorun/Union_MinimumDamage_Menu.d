META
{
    Parser    = Menu;
    After     = zUnionMenu.d;
    Namespace = MinDamage;
};

// ------ Constants ------
const int Start_PY  = 1400;
const int Title_PY  = 450;
const int Menu_DY   = 550;

// Text Layout
const int Text_PX   = 400;  
const int Text_SX   = 8000; 
const int Text_SY   = 750;  
const int Text_DY   = 120;  

// Choice Layout
const int Choice_PX = 6400; 
const int Choice_SX = 1500; 
const int Choice_SY = 350;  
const int Choice_DY = 120;  

var int CurrentMenuItem_PY;

// ------ Prototypes & Bases ------
// Das ist der Fix für das ITEM_BACK Problem! Es erbt von C_MENU (leer) und nutzt Unions Back-Button.
prototype C_EMPTY_MENU_DEF(C_MENU)
{
    backpic    = MENU_BACK_PIC;
    items[0]   = "";
    items[100] = "Union_menuitem_back"; 
    flags      = flags | MENU_SHOW_INFO;
};

instance C_MENU_ITEM_TEXT_BASE(C_MENU_ITEM_DEF)
{
    backpic        = MENU_ITEM_BACK_PIC;
    posx           = Text_PX;
    posy           = Start_PY;
    dimx           = Text_SX;
    dimy           = Text_SY;
    flags          = flags | IT_EFFECTS_NEXT;
    onselaction[0] = SEL_ACTION_UNDEF;
};

instance C_MENUITEM_CHOICE_BASE(C_MENU_ITEM_DEF)
{
    backpic  = MENU_CHOICE_BACK_PIC;
    type     = MENU_ITEM_CHOICEBOX;
    fontname = MENU_FONT_SMALL;
    posx     = Choice_PX;
    posy     = Start_PY + Choice_DY;
    dimx     = Choice_SX;
    dimy     = Choice_SY;
    flags    = flags & ~IT_SELECTABLE;
    flags    = flags | IT_TXT_CENTER;
};

// ------ Menu Entry ------
INSTANCE MenuItem_Union_Auto_MinDamage(C_MENU_ITEM_UNION_DEF)
{
    text[0]        = "Minimum Damage Optionen"; 
    text[1]        = "Modus und Werte fuer den Mindestschaden konfigurieren.";
    onSelAction[0] = SEL_ACTION_STARTMENU;
    onSelAction_S[0] = "MinDamage:Menu_Opt_MinDamage"; 
};

// ------ Main Menu ------
INSTANCE Menu_Opt_MinDamage(C_EMPTY_MENU_DEF)
{
    Menu_SearchItems("MinDamage:MENUITEM_OPT_MINDAMAGE_*");
};

// ------ Headline ------
INSTANCE MenuItem_Opt_MinDamage_Headline(C_MENU_ITEM_DEF)
{
    type    = MENU_ITEM_TEXT;
    posx    = 0;
    posy    = Title_PY;
    dimx    = 8100;
    flags   = flags & ~IT_SELECTABLE;
    flags   = flags | IT_TXT_CENTER;
    text[0] = "MINIMUM DAMAGE EINSTELLUNGEN";
};

// ------ 1. Modus (Dynamisch vs Fest) ------
INSTANCE MenuItem_Opt_MinDamage_01_Mode(C_MENU_ITEM)
{
    CurrentMenuItem_PY = 1;
    C_MENU_ITEM_TEXT_BASE();
    posy += Menu_DY * CurrentMenuItem_PY + Text_DY;
    
    text[0] = "Schadens-Modus";
    text[1] = "Dynamisch (nach Attributen) oder Fester Wert?";
};

INSTANCE MenuItem_Opt_MinDamage_01_Mode_Choice(C_MENU_ITEM_DEF)
{
    C_MENUITEM_CHOICE_BASE();
    posy += Menu_DY * CurrentMenuItem_PY;
    
    onchgsetoption        = "DynamicMode";
    onchgsetoptionsection = "UNION_MINIMUM_DAMAGE";
    text[0]               = "Fester Wert|Dynamisch";
};

// ------ 2. Fester Wert ------
INSTANCE MenuItem_Opt_MinDamage_02_Val(C_MENU_ITEM)
{
    CurrentMenuItem_PY = 2;
    C_MENU_ITEM_TEXT_BASE();
    posy += Menu_DY * CurrentMenuItem_PY + Text_DY;
    
    text[0] = "Fester Mindestschaden";
    text[1] = "Greift nur, wenn Modus auf 'Fester Wert' steht.";
};

INSTANCE MenuItem_Opt_MinDamage_02_Val_Choice(C_MENU_ITEM_DEF)
{
    C_MENUITEM_CHOICE_BASE();
    posy += Menu_DY * CurrentMenuItem_PY;
    
    onchgsetoption        = "MinDamageValue";
    onchgsetoptionsection = "UNION_MINIMUM_DAMAGE";
    text[0]               = "0|1|2|3|4|5|6|7|8|9|10|11|12|13|14|15|16|17|18|19|20";
};

// ------ 3. Magie-Durchschlag ------
INSTANCE MenuItem_Opt_MinDamage_03_Magic(C_MENU_ITEM)
{
    CurrentMenuItem_PY = 3;
    C_MENU_ITEM_TEXT_BASE();
    posy += Menu_DY * CurrentMenuItem_PY + Text_DY;
    
    text[0] = "Magie-Durchschlag";
    text[1] = "Vanilla (0), Max Mana, Aktuelles Mana, oder 10% Rohschaden";
};

INSTANCE MenuItem_Opt_MinDamage_03_Magic_Choice(C_MENU_ITEM_DEF)
{
    C_MENUITEM_CHOICE_BASE();
    posy += Menu_DY * CurrentMenuItem_PY;
    
    onchgsetoption        = "MagicMode";
    onchgsetoptionsection = "UNION_MINIMUM_DAMAGE";
    text[0]               = "Vanilla|Max Mana|Akt. Mana|10% Rohschaden"; // Index 0=Vanilla, 1=MaxMana, 2=CurMana, 3=10%
};

// ------ 4. Debug Modus ------
INSTANCE MenuItem_Opt_MinDamage_04_Debug(C_MENU_ITEM)
{
    CurrentMenuItem_PY = 4;
    C_MENU_ITEM_TEXT_BASE();
    posy += Menu_DY * CurrentMenuItem_PY + Text_DY;
    
    text[0] = "Debug-Modus";
    text[1] = "Gibt die Schadensberechnung live auf dem Bildschirm aus.";
};

INSTANCE MenuItem_Opt_MinDamage_04_Debug_Choice(C_MENU_ITEM_DEF)
{
    C_MENUITEM_CHOICE_BASE();
    posy += Menu_DY * CurrentMenuItem_PY;
    
    onchgsetoption        = "DebugMode";
    onchgsetoptionsection = "UNION_MINIMUM_DAMAGE";
    text[0]               = "Aus|An"; // Index 0 = Aus, Index 1 = An
};