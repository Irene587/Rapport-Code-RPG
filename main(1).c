#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "graphic.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// =====================================================
// CONSTANTS & MACROS
// =====================================================

#define INVENTORY_SIZE        20
#define MAX_TITLE_LEN         50
#define MONSTER_COUNT         34
#define DUNGEON_NAME_LEN      64
#define BOSS_EVERY            5
#define MAX_ACTIONS           3
#define MAX_SKILLS_PER_CLASS  3
#define MAX_LIMBS             4
#define HP_BAR_WIDTH          30

// =====================================================
// CORE DATA TYPES
// =====================================================

typedef struct {
    int HP, PM;
    int STR, MAG;
    int DEF, RES;
    int SPD, CRT;
} Stat;

typedef enum { ITEM_POTION, ITEM_ETHER } ItemType;
typedef enum { FIGHT_WIN, FIGHT_LOSE, FIGHT_FLED } FightResult;

typedef struct {
    ItemType type;
    int quantity;
} ItemStack;

typedef struct {
    ItemStack items[INVENTORY_SIZE];
    int count;
} Inventory;

// =====================================================
// LIMB TARGETING SYSTEM
// =====================================================

typedef enum {
    DEBUFF_NONE,
    DEBUFF_STR_DOWN,
    DEBUFF_DEF_DOWN,
    DEBUFF_SPD_DOWN,
    DEBUFF_RES_DOWN,
    DEBUFF_CRT_DOWN,
    DEBUFF_MAG_DOWN,
    DEBUFF_BLEED,
    DEBUFF_STUN,
    DEBUFF_IMMOBILIZE
} DebuffType;

static const char* DEBUFF_NAMES[] = {
    "None", "STR Down", "DEF Down", "SPD Down", "RES Down",
    "CRT Down", "MAG Down", "Bleeding", "Stunned", "Immobilized"
};

typedef struct {
    char name[24];
    int  hp;
    int  max_hp;
    bool broken;
    DebuffType debuff;
    int  debuff_magnitude;
    char break_msg[80];
} Limb;

typedef struct {
    DebuffType type;
    int magnitude;
    int turns_remaining; // -1 = permanent
} ActiveDebuff;

#define MAX_ACTIVE_DEBUFFS 8

typedef struct {
    Limb limbs[MAX_LIMBS];
    int  limb_count;
    ActiveDebuff debuffs[MAX_ACTIVE_DEBUFFS];
    int  debuff_count;
    int  broken_limb_count; // tracks how many limbs broken for damage bonus
} LimbState;

// =====================================================
// LIMB TEMPLATE DEFINITIONS
// =====================================================

typedef struct {
    int monster_id;
    int limb_count;
    struct {
        char name[24];
        int  max_hp;
        DebuffType debuff;
        int  debuff_magnitude;
        char break_msg[80];
    } limb_defs[MAX_LIMBS];
} LimbTemplate;

#define LT(id, n, ...) {id, n, {__VA_ARGS__}}

static LimbTemplate LIMB_DB[] = {
    LT(0, 2,
        {"Core Membrane",  12, DEBUFF_DEF_DOWN, 4,  "The slime's membrane tears! Its gooey defence crumbles."},
        {"Acid Sac",       8,  DEBUFF_BLEED,    3,  "Acid ruptures over your foe! It starts corroding itself."}
    ),
    LT(1, 3,
        {"Sword Arm",      18, DEBUFF_STR_DOWN, 6,  "The bandit's sword arm goes limp. Attacks weaken."},
        {"Shield Arm",     15, DEBUFF_DEF_DOWN, 5,  "Shield arm shattered! Defence collapses."},
        {"Legs",           14, DEBUFF_IMMOBILIZE,0, "Legs crippled. The bandit can no longer retreat."}
    ),
    LT(2, 2,
        {"Claws",          10, DEBUFF_STR_DOWN, 4,  "Claws severed! The goblin scratches feebly."},
        {"Head",           8,  DEBUFF_CRT_DOWN, 8,  "Bashed in the skull! Goblin stumbles, focus lost."}
    ),
    LT(3, 3,
        {"Draw Arm",       10, DEBUFF_STR_DOWN, 5,  "Draw arm disabled! Arrows fly wild."},
        {"Eyes",           6,  DEBUFF_CRT_DOWN, 15, "Eyes gouged! The archer can no longer aim."},
        {"Quiver Strap",   8,  DEBUFF_SPD_DOWN, 4,  "Quiver destroyed! Reloading slows everything down."}
    ),
    LT(4, 3,
        {"Weapon Hand",    22, DEBUFF_STR_DOWN, 8,  "Weapon hand crushed! Fighting becomes desperate."},
        {"Armour Plates",  25, DEBUFF_DEF_DOWN, 8,  "Armour buckles and falls away!"},
        {"Legs",           18, DEBUFF_SPD_DOWN, 5,  "Legs hammered. Movement slows noticeably."}
    ),
    LT(5, 2,
        {"Jaw",            12, DEBUFF_STR_DOWN, 6,  "Jaw dislocated! Biting power reduced."},
        {"Hind Legs",      10, DEBUFF_SPD_DOWN, 6,  "Hind legs injured. Dog can barely charge."}
    ),
    LT(6, 2,
        {"Left Wing",      8,  DEBUFF_SPD_DOWN, 5,  "Left wing torn! Bat spirals erratically."},
        {"Right Wing",     8,  DEBUFF_SPD_DOWN, 5,  "Right wing torn! Bat crashes to the ground."}
    ),
    LT(7, 3,
        {"Left Fist",      35, DEBUFF_STR_DOWN, 10, "Left fist crumbles to rubble! Crushing strength halved."},
        {"Chest Core",     40, DEBUFF_DEF_DOWN, 12, "Chest core fractured! Structural integrity fails."},
        {"Stone Legs",     30, DEBUFF_SPD_DOWN, 3,  "Stone legs crack. The golem shambles even slower."}
    ),
    LT(8, 3,
        {"Fang",           10, DEBUFF_STR_DOWN, 4,  "Fang broken off! Venom delivery impaired."},
        {"Spinneret",      8,  DEBUFF_SPD_DOWN, 4,  "Spinneret burst! Web trap threat neutralised."},
        {"Two Front Legs", 12, DEBUFF_DEF_DOWN, 5,  "Front legs snapped! Agile defence crumbles."}
    ),
    LT(9, 3,
        {"Venom Gland",    14, DEBUFF_BLEED,    4,  "Venom gland ruptured! Venom sprays everywhere."},
        {"Chelicerae",     12, DEBUFF_STR_DOWN, 6,  "Chelicerae crushed! Paralytic bite fails."},
        {"Abdomen",        16, DEBUFF_RES_DOWN, 5,  "Abdomen pierced! Toxin resistance breaks down."}
    ),
    LT(10, 3,
        {"Club Arm",       50, DEBUFF_STR_DOWN, 15, "Club arm smashed! The troll howls in rage."},
        {"Thick Hide",     45, DEBUFF_DEF_DOWN, 10, "Hide ripped open! Troll's armour is gone."},
        {"Skull",          30, DEBUFF_STUN,     2,  "Skull cracked! Troll reels, losing next actions."}
    ),
    LT(11, 3,
        {"Crystal Carapace",30, DEBUFF_DEF_DOWN,12, "Crystals shatter! Armour stripped away."},
        {"Tail Spike",     20, DEBUFF_STR_DOWN, 8,  "Tail spike broken! Rear strike threat gone."},
        {"Maw",            25, DEBUFF_RES_DOWN, 8,  "Maw cracked open! Magic seeps inside."}
    ),
    LT(12, 2,
        {"Jaw",            18, DEBUFF_STR_DOWN, 7,  "Jaw dislocated with a sickening crack!"},
        {"Hind Legs",      15, DEBUFF_SPD_DOWN, 6,  "Hind legs buckle. The wolf limps badly."}
    ),
    LT(13, 4,
        {"Mandibles",      30, DEBUFF_STR_DOWN, 10, "Mandibles snapped! Gripping attack fails."},
        {"Antennae",       12, DEBUFF_CRT_DOWN, 10, "Antennae ripped off! Senses dulled."},
        {"Thorax Plates",  35, DEBUFF_DEF_DOWN, 12, "Thorax plates caved in! Exoskeleton exposed."},
        {"Mid Legs",       25, DEBUFF_SPD_DOWN, 5,  "Mid legs crushed! Scuttling speed drops."}
    ),
    LT(14, 2,
        {"Tusks",          20, DEBUFF_STR_DOWN, 8,  "Tusks sheared off! Goring power gone."},
        {"Hooves",         16, DEBUFF_SPD_DOWN, 5,  "Hooves damaged. Charge force reduced."}
    ),
    LT(15, 2,
        {"Spectral Core",  25, DEBUFF_MAG_DOWN, 10, "Spirit core disrupted! Magic output falters."},
        {"Root Tendrils",  18, DEBUFF_STR_DOWN, 6,  "Tendrils severed! Entangle power fails."}
    ),
    LT(16, 3,
        {"Bark Skin",      28, DEBUFF_DEF_DOWN, 10, "Bark skin peeled away! Dryad is exposed."},
        {"Branch Arms",    20, DEBUFF_STR_DOWN, 8,  "Branch arms splintered!"},
        {"Flower Crown",   12, DEBUFF_MAG_DOWN, 8,  "Crown shattered! Nature magic weakened."}
    ),
    LT(17, 3,
        {"Right Paw",      30, DEBUFF_STR_DOWN, 10, "Right paw crushed! Swipe damage falls."},
        {"Left Paw",       30, DEBUFF_STR_DOWN, 10, "Left paw mangled! The bear fights one-pawed."},
        {"Hind Legs",      25, DEBUFF_SPD_DOWN, 4,  "Hind legs buckle. Bear drops from rearing stance."}
    ),
    LT(18, 2,
        {"Ritual Dagger Hand",18, DEBUFF_STR_DOWN, 6, "Dagger hand broken! Ritual strikes fail."},
        {"Chanting Throat", 12, DEBUFF_MAG_DOWN, 8,  "Throat crushed! Chanting stops - spell power drops."}
    ),
    LT(19, 3,
        {"Staff Hand",     16, DEBUFF_MAG_DOWN, 12, "Staff hand shattered! Conduit destroyed."},
        {"Vestment Wards", 20, DEBUFF_RES_DOWN, 10, "Wards on vestments torn away! Magic resistance drops."},
        {"Eyes",           10, DEBUFF_CRT_DOWN, 12, "Eyes blinded! Precision spellwork collapses."}
    ),
    LT(20, 4,
        {"Stone Shield",   50, DEBUFF_DEF_DOWN, 15, "Ancient shield cracks in two!"},
        {"Weapon Arm",     45, DEBUFF_STR_DOWN, 15, "Weapon arm severed from the socket!"},
        {"Sacred Plating", 40, DEBUFF_RES_DOWN, 12, "Sacred runes on plating smashed! Magic floods in."},
        {"Legs",           35, DEBUFF_SPD_DOWN, 5,  "Legs buckle under focused assault."}
    ),
    LT(21, 3,
        {"Helm",           35, DEBUFF_CRT_DOWN, 10, "Helm dented in! Spirit struggles to aim."},
        {"Breastplate",    40, DEBUFF_DEF_DOWN, 15, "Breastplate buckled! Armour useless."},
        {"Gauntlets",      28, DEBUFF_STR_DOWN, 10, "Gauntlets crushed! Grip strength fails."}
    ),
    LT(22, 3,
        {"Casting Hand",   18, DEBUFF_MAG_DOWN, 15, "Casting hand broken! Spell power collapses."},
        {"Focus Crystal",  14, DEBUFF_RES_DOWN, 12, "Focus crystal shattered! Resistance unravels."},
        {"Arcane Mantle",  22, DEBUFF_RES_DOWN, 8,  "Mantle torn! Arcane barrier gone."}
    ),
    LT(23, 2,
        {"Ethereal Form",  30, DEBUFF_DEF_DOWN, 12, "Ethereal shell torn! Wraith becomes tangible."},
        {"Spirit Core",    25, DEBUFF_MAG_DOWN, 10, "Spirit core destabilised! Holy power fades."}
    ),
    LT(24, 4,
        {"Lance Arm",      40, DEBUFF_STR_DOWN, 12, "Lance arm severed! Charge attack ruined."},
        {"Tower Shield",   45, DEBUFF_DEF_DOWN, 14, "Tower shield split in half!"},
        {"Dark Helm",      30, DEBUFF_CRT_DOWN, 10, "Dark helm crushed! Knight reels."},
        {"Armoured Legs",  35, DEBUFF_SPD_DOWN, 5,  "Armoured legs buckled! Charge speed lost."}
    ),
    LT(25, 3,
        {"Halberd Arm",    30, DEBUFF_STR_DOWN, 10, "Halberd arm snapped! Reach attack fails."},
        {"Plate Chest",    35, DEBUFF_DEF_DOWN, 10, "Chest plate caved in!"},
        {"Visor",          18, DEBUFF_CRT_DOWN, 8,  "Visor shattered! Guard fights half-blind."}
    ),
    LT(26, 3,
        {"Axe Arm",        55, DEBUFF_STR_DOWN, 18, "Axe arm broken! The giant cleaver drops."},
        {"Braced Shoulders",40, DEBUFF_DEF_DOWN,10, "Shoulder brace torn away!"},
        {"Mask",           25, DEBUFF_STUN,     1,  "Mask shattered! Executioner momentarily stunned."}
    ),
    LT(27, 3,
        {"Phylactery",     20, DEBUFF_MAG_DOWN, 18, "Phylactery cracked! Undead power crumbles."},
        {"Summoning Hand", 16, DEBUFF_STR_DOWN, 8,  "Summoning hand broken! No more minions."},
        {"Dark Cowl",      14, DEBUFF_RES_DOWN, 10, "Cowl destroyed! Necrotic wards gone."}
    ),
    LT(28, 3,
        {"Clawed Hand",    25, DEBUFF_STR_DOWN, 10, "Clawed hand severed! Rend attack lost."},
        {"Bat Wings",      20, DEBUFF_SPD_DOWN, 8,  "Wings shredded! Vampire loses aerial advantage."},
        {"Vampiric Fangs", 18, DEBUFF_BLEED,    5,  "Fangs knocked out! Life drain reversed - bleeds."}
    ),
    LT(29, 4,
        {"Skeletal Arm",   30, DEBUFF_MAG_DOWN, 14, "Skeletal arm reduced to dust! Magic output drops."},
        {"Rib Cage Core",  40, DEBUFF_DEF_DOWN, 12, "Rib cage destroyed! Soul anchor weakens."},
        {"Phylactery Skull",35, DEBUFF_RES_DOWN, 15,"Phylactery skull cracked! Magic resistance crumbles."},
        {"Spectral Robe",  25, DEBUFF_BLEED,    6,  "Spectral robe torn! Lich starts losing essence."}
    ),
    LT(30, 3,
        {"Hinged Lid",     28, DEBUFF_STR_DOWN, 8,  "Lid torn off! Bite attack loses force."},
        {"Treasure Lure",  18, DEBUFF_CRT_DOWN, 10, "Lure destroyed! Mimic's ambush precision drops."},
        {"Chest Body",     32, DEBUFF_DEF_DOWN, 10, "Chest body cracked open! Structural integrity fails."}
    ),
    LT(31, 2,
        {"Spectral Haze",  22, DEBUFF_DEF_DOWN, 10, "Haze dissipates! Spirit becomes tangible."},
        {"Will-o-core",    18, DEBUFF_MAG_DOWN, 12, "Will-o-core extinguished! Spirit power gutters."}
    ),
    LT(32, 4,
        {"Head",           50, DEBUFF_STR_DOWN, 15, "Head wound! Wyrm's bite becomes wild and weak."},
        {"Wings",          40, DEBUFF_SPD_DOWN, 8,  "Wings shredded! Wyrm crashes to the ground."},
        {"Tail",           35, DEBUFF_STUN,     1,  "Tail severed! Wyrm writhes in agony, stunned."},
        {"Scale Underbelly",45, DEBUFF_DEF_DOWN,15, "Underbelly exposed! Armour rating collapses."}
    ),
    LT(33, 4,
        {"Head",           80, DEBUFF_STR_DOWN, 20, "DRAGON HEAD WOUNDED! Breath weapon partially disabled!"},
        {"Wings",          70, DEBUFF_SPD_DOWN, 10, "WINGS TORN! The ancient dragon cannot take flight!"},
        {"Scale Underbelly",75, DEBUFF_DEF_DOWN,20, "UNDERBELLY EXPOSED! Critical vulnerability found!"},
        {"Tail",           60, DEBUFF_STUN,     2,  "TAIL SEVERED! The dragon collapses momentarily!"}
    )
};
#define LIMB_DB_COUNT (sizeof(LIMB_DB)/sizeof(LIMB_DB[0]))

LimbState build_limb_state(int monster_id) {
    LimbState ls;
    memset(&ls, 0, sizeof(ls));
    for (int i = 0; i < (int)LIMB_DB_COUNT; i++) {
        if (LIMB_DB[i].monster_id == monster_id) {
            ls.limb_count = LIMB_DB[i].limb_count;
            for (int j = 0; j < ls.limb_count; j++) {
                strncpy(ls.limbs[j].name, LIMB_DB[i].limb_defs[j].name, 23);
                ls.limbs[j].max_hp = LIMB_DB[i].limb_defs[j].max_hp;
                ls.limbs[j].hp     = LIMB_DB[i].limb_defs[j].max_hp;
                ls.limbs[j].broken = false;
                ls.limbs[j].debuff = LIMB_DB[i].limb_defs[j].debuff;
                ls.limbs[j].debuff_magnitude = LIMB_DB[i].limb_defs[j].debuff_magnitude;
                strncpy(ls.limbs[j].break_msg, LIMB_DB[i].limb_defs[j].break_msg, 79);
            }
            ls.broken_limb_count = 0;
            return ls;
        }
    }
    return ls;
}

Stat apply_debuffs_to_stat(Stat base, LimbState* ls) {
    Stat s = base;
    for (int i = 0; i < ls->debuff_count; i++) {
        ActiveDebuff* d = &ls->debuffs[i];
        switch (d->type) {
            case DEBUFF_STR_DOWN: s.STR -= d->magnitude; break;
            case DEBUFF_DEF_DOWN: s.DEF -= d->magnitude; break;
            case DEBUFF_SPD_DOWN: s.SPD -= d->magnitude; break;
            case DEBUFF_RES_DOWN: s.RES -= d->magnitude; break;
            case DEBUFF_CRT_DOWN: s.CRT -= d->magnitude; break;
            case DEBUFF_MAG_DOWN: s.MAG -= d->magnitude; break;
            default: break;
        }
    }
    if (s.STR < 0) s.STR = 0;
    if (s.DEF < 0) s.DEF = 0;
    if (s.SPD < 1) s.SPD = 1;
    if (s.RES < 0) s.RES = 0;
    if (s.CRT < 0) s.CRT = 0;
    if (s.MAG < 0) s.MAG = 0;
    return s;
}

void add_debuff(LimbState* ls, DebuffType type, int magnitude) {
    if (type == DEBUFF_NONE || type == DEBUFF_STUN || type == DEBUFF_IMMOBILIZE) {
        if (ls->debuff_count < MAX_ACTIVE_DEBUFFS)
            ls->debuffs[ls->debuff_count++] = (ActiveDebuff){type, magnitude, 2};
        return;
    }
    for (int i = 0; i < ls->debuff_count; i++) {
        if (ls->debuffs[i].type == type) {
            ls->debuffs[i].magnitude += magnitude;
            return;
        }
    }
    if (ls->debuff_count < MAX_ACTIVE_DEBUFFS)
        ls->debuffs[ls->debuff_count++] = (ActiveDebuff){type, magnitude, -1};
}

int tick_debuffs(LimbState* ls) {
    int bleed = 0;
    int write = 0;
    for (int i = 0; i < ls->debuff_count; i++) {
        if (ls->debuffs[i].type == DEBUFF_BLEED) bleed += ls->debuffs[i].magnitude;
        if (ls->debuffs[i].turns_remaining > 0)
            ls->debuffs[i].turns_remaining--;
        if (ls->debuffs[i].turns_remaining != 0)
            ls->debuffs[write++] = ls->debuffs[i];
    }
    ls->debuff_count = write;
    return bleed;
}

bool has_debuff(LimbState* ls, DebuffType type) {
    for (int i = 0; i < ls->debuff_count; i++)
        if (ls->debuffs[i].type == type) return true;
    return false;
}

// =====================================================
// EQUIPMENT SYSTEM
// =====================================================

typedef enum { SLOT_WEAPON, SLOT_ARMOR, SLOT_RELIC } SlotType;

typedef struct {
    char name[32];
    SlotType slot;
    Stat modifier;
    bool discovered;
} Gear;

static Gear EQUIPMENT_POOL[] = {
    {"Rusted Blade",         SLOT_WEAPON, {0,0,4,0,0,0,0,2},      true},
    {"Warped Staff",         SLOT_WEAPON, {0,0,0,6,0,0,-2,0},     true},
    {"Heavy Broadsword",     SLOT_WEAPON, {0,0,12,0,0,0,-4,5},    false},
    {"Bloodied Khopesh",     SLOT_WEAPON, {0,0,8,0,0,0,6,12},     false},
    {"Executioner Axe",      SLOT_WEAPON, {0,0,20,0,-5,-5,-2,8},  false},
    {"Sunfire Scepter",      SLOT_WEAPON, {15,15,0,18,0,5,2,4},   false},
    {"Voidcaster Athame",    SLOT_WEAPON, {0,30,2,25,0,0,4,10},   false},
    {"Oathkeeper Greatsword",SLOT_WEAPON, {40,0,28,0,10,5,-2,6},  false},
    {"Ashen Garb",           SLOT_ARMOR,  {10,0,0,0,3,3,2,0},     true},
    {"Gilded Aegis",         SLOT_ARMOR,  {30,0,0,0,15,8,-2,0},   false},
    {"Archmage Robe",        SLOT_ARMOR,  {0,40,0,14,2,12,1,3},   false},
    {"Steel Hauberk",        SLOT_ARMOR,  {50,0,-2,0,22,5,-4,0},  false},
    {"Obsidian Carapace",    SLOT_ARMOR,  {80,0,0,0,35,15,-6,0},  false},
    {"Windrunner Shroud",    SLOT_ARMOR,  {25,10,4,0,12,12,12,8}, false},
    {"Old Ring",             SLOT_RELIC,  {0,10,0,2,0,2,0,1},     true},
    {"Gargoyle Eye",         SLOT_RELIC,  {20,0,2,0,4,4,0,0},     false},
    {"Vortex Pendant",       SLOT_RELIC,  {0,20,0,5,0,0,8,5},     false},
    {"Chrono-Anchor",        SLOT_RELIC,  {5,5,0,0,0,0,12,4},     false},
    {"Bloodstone Scarab",    SLOT_RELIC,  {35,0,6,0,-2,-2,0,6},   false},
    {"Philosopher Ledger",   SLOT_RELIC,  {0,50,0,15,5,15,2,5},   false}
};
#define EQUIPMENT_COUNT (sizeof(EQUIPMENT_POOL)/sizeof(EQUIPMENT_POOL[0]))

// =====================================================
// SKILL SYSTEM
// =====================================================

typedef enum { CLASS_WARRIOR, CLASS_MAGE, CLASS_ROGUE } ClassType;

typedef struct {
    char name[32];
    int  pm_cost;
    int  rank;
    int  min_player_level;
    const char* description;
} Skill;

typedef struct {
    ClassType   type;
    char        class_name[16];
    Stat        base_stat;
    int         current_HP;
    int         current_PM;
    int         level;
    int         exp;
    int         exp_needed;
    Inventory   inventory;
    Gear*       slots[3];
    Skill       skills[MAX_SKILLS_PER_CLASS];
} Player;

static Skill WARRIOR_TREE[MAX_SKILLS_PER_CLASS] = {
    {"Shield Slam",          6,  1, 1, "DEF-weighted strike. Bonus damage per broken enemy limb."},
    {"Iron Will",            8,  0, 3, "Hardens defences. Recover HP scaled to rank."},
    {"Juggernaut Overdrive", 15, 0, 6, "Full STR execution strike. Bonus if enemy below 30% HP."}
};
static Skill MAGE_TREE[MAX_SKILLS_PER_CLASS] = {
    {"Pyroclast",       10, 1, 1, "MAG blast. Half-penetrates RES. Extra hit if limb broken."},
    {"Chrono Shift",    12, 0, 3, "Reduces enemy SPD. More actions for you next turn."},
    {"Singularity Core",22, 0, 6, "True MAG damage. Ignores all RES. Scales with broken limbs."}
};
static Skill ROGUE_TREE[MAX_SKILLS_PER_CLASS] = {
    {"Viper Strike",           8,  1, 1, "Dual hit. High crit chance. Applies bleed on crit."},
    {"Smoke Screen",           6,  0, 3, "Guaranteed escape. Next re-entry gives ambush bonus."},
    {"Shadow Assassination",   16, 0, 6, "Triple STR. Bypasses all DEF. Always crits."}
};

// =====================================================
// MONSTER SYSTEM
// =====================================================

typedef enum {
    DUNGEON_NORMAL, DUNGEON_CAVE, DUNGEON_FOREST,
    DUNGEON_TEMPLE, DUNGEON_CASTLE, DUNGEON_TYPE_COUNT
} DungeonType;

static const char* DUNGEON_TYPE_NAMES[] = {
    "Catacombs","Cavern","Abodes","Sanctuary","Keep"
};

typedef struct {
    int id;
    const char* name;
    Stat min_stat, max_stat;
    DungeonType allowed_dungeons[5];
    int dungeon_count;
} MonsterTemplate;

typedef struct {
    int  id;
    const char* name;
    Stat stat;
    int  current_HP;
    bool is_boss;
    char boss_title[MAX_TITLE_LEN];
} Monster;

static MonsterTemplate MONSTER_DB[MONSTER_COUNT] = {
    {0,  "Slime",          {20,0,3,0,2,1,2,0},     {40,10,8,5,5,4,4,5},       {DUNGEON_NORMAL,DUNGEON_CAVE},                   2},
    {1,  "Bandit",         {35,10,6,2,4,3,5,5},     {70,25,15,8,10,8,10,15},   {DUNGEON_NORMAL,DUNGEON_FOREST},                 2},
    {2,  "Goblin",         {25,5,5,0,3,2,6,5},      {55,15,12,4,8,6,14,15},    {DUNGEON_NORMAL,DUNGEON_CAVE},                   2},
    {3,  "Goblin Archer",  {20,5,7,0,2,2,8,10},     {50,15,15,3,6,5,18,20},    {DUNGEON_NORMAL,DUNGEON_FOREST},                 2},
    {4,  "Mercenary",      {45,10,10,0,8,5,6,5},    {90,25,20,5,18,12,14,15},  {DUNGEON_NORMAL,DUNGEON_CASTLE},                 2},
    {5,  "Wild Dog",       {18,0,6,0,2,1,10,5},     {40,5,14,0,6,4,20,15},     {DUNGEON_NORMAL,DUNGEON_FOREST},                 2},
    {6,  "Cave Bat",       {15,5,4,0,2,1,8,10},     {35,15,10,4,5,3,18,25},    {DUNGEON_CAVE},                                  1},
    {7,  "Rock Golem",     {60,0,10,0,20,10,1,0},   {120,10,25,5,40,30,5,5},   {DUNGEON_CAVE,DUNGEON_TEMPLE},                   2},
    {8,  "Spider",         {20,0,5,0,2,2,12,15},    {45,5,12,0,6,5,22,30},     {DUNGEON_CAVE,DUNGEON_FOREST},                   2},
    {9,  "Venom Spider",   {30,10,8,5,4,5,10,20},   {60,25,16,12,10,10,20,35}, {DUNGEON_CAVE},                                  1},
    {10, "Cave Troll",     {90,0,20,0,12,6,2,0},    {180,10,40,5,25,12,6,5},   {DUNGEON_CAVE},                                  1},
    {11, "Crystal Worm",   {50,20,12,15,10,15,4,5}, {100,50,24,35,20,30,10,15},{DUNGEON_CAVE},                                  1},
    {12, "Wolf",           {40,5,10,0,5,3,12,10},   {80,20,20,5,15,10,25,20},  {DUNGEON_FOREST},                                1},
    {13, "Ant",            {80,10,15,5,20,10,3,0},  {150,40,30,15,40,30,8,5},  {DUNGEON_FOREST,DUNGEON_TEMPLE},                 2},
    {14, "Boar",           {50,0,14,0,10,5,6,5},    {100,5,28,0,20,10,14,10},  {DUNGEON_FOREST},                                1},
    {15, "Forest Spirit",  {35,40,5,20,6,18,10,10}, {70,90,10,45,12,40,20,20}, {DUNGEON_FOREST,DUNGEON_TEMPLE},                 2},
    {16, "Dryad",          {40,50,6,25,8,20,12,15}, {85,120,12,50,15,40,22,30},{DUNGEON_FOREST},                                1},
    {17, "Bear",           {80,0,20,0,15,5,5,5},    {160,5,40,0,30,10,10,15},  {DUNGEON_FOREST},                                1},
    {18, "Cultist",        {50,30,10,20,8,15,10,10},{90,80,25,50,20,40,20,25}, {DUNGEON_TEMPLE},                                1},
    {19, "Dark Priest",    {60,60,8,35,10,25,8,10}, {110,140,15,70,20,50,16,20},{DUNGEON_TEMPLE},                               1},
    {20, "Temple Guardian",{120,20,25,10,30,20,4,5},{220,60,50,20,60,40,10,10},{DUNGEON_TEMPLE},                                1},
    {21, "Possessed Armor",{100,0,22,0,35,20,4,5},  {180,10,40,5,70,40,8,10}, {DUNGEON_TEMPLE,DUNGEON_CASTLE},                 2},
    {22, "Ancient Mage",   {50,100,5,45,10,30,10,15},{90,200,10,90,20,60,20,25},{DUNGEON_TEMPLE},                               1},
    {23, "Holy Wraith",    {45,80,10,40,8,25,14,20},{90,160,18,75,15,50,28,35},{DUNGEON_TEMPLE},                                1},
    {24, "Dark Knight",    {100,20,25,10,30,20,12,15},{180,60,45,25,60,40,25,30},{DUNGEON_CASTLE,DUNGEON_TEMPLE},               2},
    {25, "Royal Guard",    {80,10,20,5,20,10,8,10}, {140,25,38,10,40,20,18,20},{DUNGEON_CASTLE},                                1},
    {26, "Executioner",    {120,0,35,0,20,10,4,15}, {220,10,60,0,40,20,8,30},  {DUNGEON_CASTLE},                                1},
    {27, "Necromancer",    {50,120,5,55,8,30,10,10},{90,220,10,110,15,60,20,20},{DUNGEON_CASTLE,DUNGEON_TEMPLE},                2},
    {28, "Vampire",        {70,80,20,25,12,20,18,20},{140,160,40,50,25,40,35,40},{DUNGEON_CASTLE},                              1},
    {29, "Lich",           {90,200,10,70,15,40,8,10},{160,350,20,140,30,80,16,25},{DUNGEON_CASTLE,DUNGEON_TEMPLE},              2},
    {30, "Mimic",          {60,20,15,10,15,10,5,10},{120,50,30,20,30,20,12,20},{DUNGEON_NORMAL,DUNGEON_CAVE,DUNGEON_CASTLE},    3},
    {31, "Wandering Spirit",{40,60,8,30,5,20,15,15},{80,140,15,60,12,45,30,30},{DUNGEON_FOREST,DUNGEON_TEMPLE,DUNGEON_CASTLE},  3},
    {32, "Wyrm",           {120,40,30,20,25,20,12,10},{220,100,60,40,50,40,22,25},{DUNGEON_CAVE,DUNGEON_CASTLE},               2},
    {33, "Ancient Dragon", {300,150,60,60,50,50,15,20},{500,300,120,120,100,100,30,40},{DUNGEON_CASTLE,DUNGEON_CAVE},           2},
};

static const char* BOSS_TITLES[] = {
    "the Undying","the Corrupted","the Ancient","Lord",
    "the Defiler","the Eternal","Tyrant","the Cursed"
};
static const int BOSS_TITLE_COUNT = 8;

static const char* DESCRIPTORS[] = {
    "Haunted","Ancient","Grim","Forbidden","Forgotten","Dark","Ruined","Cursed"
};
static const int DESCRIPTOR_COUNT = 8;

static const char* BASE_NORMAL[] = {"Crypts","Abyss","Catacombs"};
static const char* BASE_CAVE[]   = {"Cavern","Grotto","Chasm"};
static const char* BASE_FOREST[] = {"Thicket","Woods","Glade"};
static const char* BASE_TEMPLE[] = {"Shrine","Temple","Sanctum"};
static const char* BASE_CASTLE[] = {"Citadel","Fortress","Keep"};

// =====================================================
// GLOBAL GAME STATE
// =====================================================

typedef enum { STATE_MENU, STATE_CAMP, STATE_EVENT, STATE_GAMEOVER } GameState;

typedef struct {
    GameState   state;
    Player      player;
    DungeonType dungeon_type;
    char        dungeon_name[DUNGEON_NAME_LEN];
    int         floor;
    Monster     current_enemy;
} Game;

static Game G;

// =====================================================
// UTILITIES
// =====================================================

int rand_range(int min, int max) {
    if (max <= min) return min;
    return min + rand() % (max - min + 1);
}

Stat generate_stat(Stat mn, Stat mx) {
    return (Stat){
        rand_range(mn.HP,mx.HP), rand_range(mn.PM,mx.PM),
        rand_range(mn.STR,mx.STR), rand_range(mn.MAG,mx.MAG),
        rand_range(mn.DEF,mx.DEF), rand_range(mn.RES,mx.RES),
        rand_range(mn.SPD,mx.SPD), rand_range(mn.CRT,mx.CRT),
    };
}

static void print_header(const char* title) {
    printf("\n+---------------------------------------------------+\n");
    printf("| %-49s |\n", title);
    printf("+---------------------------------------------------+\n");
}

static void print_divider(void) {
    printf("-----------------------------------------------------\n");
}

static void clear_screen(void) {
#if defined(_WIN32) || defined(_WIN64)
    system("cls");
#else
    printf("\033[H\033[2J");
    fflush(stdout);
#endif
}

static void press_enter(void) {
    printf("\n [Press ENTER to continue]");
    while (getchar() != '\n');
}

void generate_dungeon_name(DungeonType t, char* out) {
    int i = rand() % 3;
    const char* base = "Domain";
    switch (t) {
        case DUNGEON_NORMAL: base = BASE_NORMAL[i]; break;
        case DUNGEON_CAVE:   base = BASE_CAVE[i];   break;
        case DUNGEON_FOREST: base = BASE_FOREST[i]; break;
        case DUNGEON_TEMPLE: base = BASE_TEMPLE[i]; break;
        case DUNGEON_CASTLE: base = BASE_CASTLE[i]; break;
        default: break;
    }
    snprintf(out, DUNGEON_NAME_LEN, "The %s %s",
        DESCRIPTORS[rand() % DESCRIPTOR_COUNT], base);
}

// =====================================================
// STAT CALCULATION
// =====================================================

Stat calculate_total_stats(Player* p) {
    Stat total = p->base_stat;
    for (int i = 0; i < 3; i++) {
        if (p->slots[i]) {
            total.HP  += p->slots[i]->modifier.HP;
            total.PM  += p->slots[i]->modifier.PM;
            total.STR += p->slots[i]->modifier.STR;
            total.MAG += p->slots[i]->modifier.MAG;
            total.DEF += p->slots[i]->modifier.DEF;
            total.RES += p->slots[i]->modifier.RES;
            total.SPD += p->slots[i]->modifier.SPD;
            total.CRT += p->slots[i]->modifier.CRT;
        }
    }
    if (total.HP < 1) total.HP = 1;
    if (total.PM < 0) total.PM = 0;
    return total;
}

int actions_for(Stat* s) {
    int a = s->SPD / 10;
    if (a < 1) a = 1;
    if (a > MAX_ACTIONS) a = MAX_ACTIONS;
    return a;
}

// =====================================================
// HP / PM BAR RENDERING
// =====================================================

// Fills a bar with '#' for current and '.' for missing.
// Shows a '!' segment in the last 20% to signal danger.
// label: short string printed before the bar e.g. "HP"
static void print_bar(const char* label, int cur, int max, int width) {
    if (max <= 0) max = 1;
    if (cur < 0) cur = 0;
    int filled = cur * width / max;
    if (filled > width) filled = width;

    // danger threshold: last 20%
    int danger_start = width * 8 / 10;

    printf(" %s [", label);
    for (int i = 0; i < width; i++) {
        if (i < filled) {
            // Use '!' for the danger zone when low
            if (cur * 100 / max <= 20 && i >= danger_start)
                putchar('!');
            else
                putchar('#');
        } else {
            putchar('.');
        }
    }
    printf("] %d/%d\n", cur, max);
}

// Combined status block used in combat — both bars on adjacent lines
static void print_combat_bars(Player* p, Monster* m) {
    Stat total = calculate_total_stats(p);
    printf(" YOU  ");
    print_bar("HP", p->current_HP, total.HP, HP_BAR_WIDTH);
    printf("      ");
    print_bar("PM", p->current_PM, total.PM, HP_BAR_WIDTH);
    printf("\n");
    if (m->is_boss)
        printf(" %s %s\n", m->boss_title, m->name);
    else
        printf(" %s\n", m->name);
    print_bar("HP", m->current_HP, m->stat.HP, HP_BAR_WIDTH);
}

// =====================================================
// INVENTORY
// =====================================================

void inventory_add(Inventory* inv, ItemType type, int qty) {
    for (int i = 0; i < inv->count; i++) {
        if (inv->items[i].type == type) { inv->items[i].quantity += qty; return; }
    }
    if (inv->count < INVENTORY_SIZE)
        inv->items[inv->count++] = (ItemStack){type, qty};
}

int inventory_count(Inventory* inv, ItemType type) {
    for (int i = 0; i < inv->count; i++)
        if (inv->items[i].type == type) return inv->items[i].quantity;
    return 0;
}

bool inventory_use(Inventory* inv, ItemType type) {
    for (int i = 0; i < inv->count; i++) {
        if (inv->items[i].type == type && inv->items[i].quantity > 0) {
            inv->items[i].quantity--; return true;
        }
    }
    return false;
}

// =====================================================
// STATUS DISPLAY
// =====================================================

static void print_player_status(Player* p) {
    Stat total = calculate_total_stats(p);
    printf(" [%s] Lv.%d | EXP: %d/%d\n",
        p->class_name, p->level, p->exp, p->exp_needed);
    print_bar("HP", p->current_HP, total.HP, HP_BAR_WIDTH);
    print_bar("PM", p->current_PM, total.PM, HP_BAR_WIDTH);
    printf(" Potions: [%d]  Ethers: [%d]\n",
        inventory_count(&p->inventory, ITEM_POTION),
        inventory_count(&p->inventory, ITEM_ETHER));
}

static void print_limb_state(LimbState* ls) {
    if (ls->limb_count == 0) return;
    printf(" Limbs: ");
    for (int i = 0; i < ls->limb_count; i++) {
        Limb* lb = &ls->limbs[i];
        if (lb->broken)
            printf("[%s:BROKEN] ", lb->name);
        else
            printf("[%s:%d/%d] ", lb->name, lb->hp, lb->max_hp);
    }
    printf("\n");
    bool any = false;
    for (int i = 0; i < ls->debuff_count; i++) {
        ActiveDebuff* d = &ls->debuffs[i];
        if (d->type != DEBUFF_NONE) {
            if (!any) { printf(" Debuffs: "); any = true; }
            if (d->turns_remaining == -1)
                printf("[%s-%d] ", DEBUFF_NAMES[d->type], d->magnitude);
            else
                printf("[%s(%dT)] ", DEBUFF_NAMES[d->type], d->turns_remaining);
        }
    }
    if (any) printf("\n");
}

static void print_monster_status(Monster* m, LimbState* ls) {
    print_bar("HP", m->current_HP, m->stat.HP, HP_BAR_WIDTH);
    print_limb_state(ls);
}

// =====================================================
// LIMB TARGETING — REWORKED DAMAGE MODEL
//
// A limb attack splits damage into TWO portions:
//   - LIMB portion  (80% of raw damage) applied to the limb HP.
//   - BODY bleed-through (50% of raw damage) applied to enemy HP
//     directly — always, even if the limb doesn't break.
//
// When a limb BREAKS it also applies:
//   - An instant body bonus hit: 30% of raw damage extra.
//   - Its debuff (permanent stat penalty or bleed/stun).
//
// Every already-broken limb on the monster increases ALL future
// body damage by +15% (stacks additively). This makes early limb
// investment snowball into huge body damage.
// =====================================================

// Calculate the body damage multiplier from broken limbs
static float broken_limb_body_bonus(LimbState* ls) {
    // +15% per broken limb, up to 60% bonus
    float bonus = 1.0f + (ls->broken_limb_count * 0.15f);
    if (bonus > 1.60f) bonus = 1.60f;
    return bonus;
}

// Core limb-targeting attack. Returns total body damage dealt.
int do_limb_attack(Player* p, Monster* enemy, LimbState* ls, bool is_magic) {
    if (ls->limb_count == 0) {
        printf(" This enemy has no targetable limbs.\n");
        return 0;
    }

    // Show available limbs with HP bars and break-debuff preview
    printf("\n Target limb  (broken limbs grant +15%% body dmg each):\n");
    int available[MAX_LIMBS];
    int avail_count = 0;
    for (int i = 0; i < ls->limb_count; i++) {
        Limb* lb = &ls->limbs[i];
        if (!lb->broken) {
            // Mini HP bar for the limb
            int bw = 12;
            int filled = lb->hp * bw / (lb->max_hp > 0 ? lb->max_hp : 1);
            char bar[16];
            for (int b = 0; b < bw; b++) bar[b] = (b < filled) ? '#' : '.';
            bar[bw] = '\0';
            printf("  %d. %-20s [%s] %2d/%-2d  on break: %s\n",
                avail_count + 1,
                lb->name,
                bar,
                lb->hp, lb->max_hp,
                DEBUFF_NAMES[lb->debuff]);
            available[avail_count++] = i;
        }
    }
    if (avail_count == 0) {
        // All limbs broken — still let the player attack body
        printf(" All limbs broken! Body is fully exposed.\n");
        // Bonus body attack instead
        Stat total_p = calculate_total_stats(p);
        int raw;
        if (is_magic) {
            if (p->current_PM < 4) { printf(" Not enough PM.\n"); return 0; }
            p->current_PM -= 4;
            raw = total_p.MAG * 100 / (100 + enemy->stat.RES);
        } else {
            raw = total_p.STR * 100 / (100 + enemy->stat.DEF);
        }
        if (rand() % 100 < total_p.CRT) { raw *= 2; printf(" CRITICAL!\n"); }
        int body = (int)(raw * broken_limb_body_bonus(ls));
        if (body < 1) body = 1;
        printf(" Exposed body hit for %d damage!\n", body);
        return body;
    }

    // Print broken count bonus info
    if (ls->broken_limb_count > 0)
        printf(" [%d limb(s) broken: +%d%% body dmg bonus active]\n",
            ls->broken_limb_count, ls->broken_limb_count * 15);

    printf("  0. Cancel\n> ");

    int choice = 0;
    if (scanf("%d", &choice) != 1) choice = 0;
    while (getchar() != '\n');
    if (choice < 1 || choice > avail_count) return 0;

    int limb_idx = available[choice - 1];
    Limb* lb = &ls->limbs[limb_idx];

    Stat total_p = calculate_total_stats(p);
    int raw;
    if (is_magic) {
        if (p->current_PM < 4) { printf("\n Not enough PM.\n"); return 0; }
        p->current_PM -= 4;
        raw = total_p.MAG * 100 / (100 + enemy->stat.RES);
    } else {
        raw = total_p.STR * 100 / (100 + enemy->stat.DEF);
    }
    bool crit = (rand() % 100 < total_p.CRT);
    if (crit) { raw *= 2; printf(" CRITICAL STRIKE!\n"); }
    if (raw < 1) raw = 1;

    // Split the hit
    int limb_dmg = raw * 8 / 10;   // 80% to limb
    int body_dmg = raw * 5 / 10;   // 50% bleed-through to body (always)

    // Apply broken limb bonus to the bleed-through
    body_dmg = (int)(body_dmg * broken_limb_body_bonus(ls));
    if (limb_dmg < 1) limb_dmg = 1;
    if (body_dmg < 1) body_dmg = 1;

    lb->hp -= limb_dmg;
    printf(" %s struck for %d limb dmg  |  %d bleed-through to body\n",
        lb->name, limb_dmg, body_dmg);

    if (lb->hp <= 0) {
        lb->hp = 0;
        lb->broken = true;
        ls->broken_limb_count++;

        printf("\n *** %s ***\n", lb->break_msg);

        // Instant bonus body hit on break
        int break_bonus = raw * 3 / 10;
        break_bonus = (int)(break_bonus * broken_limb_body_bonus(ls));
        if (break_bonus > 0) {
            body_dmg += break_bonus;
            printf(" Break bonus! +%d extra body damage!\n", break_bonus);
        }

        // Apply debuff
        if (lb->debuff != DEBUFF_NONE) {
            add_debuff(ls, lb->debuff, lb->debuff_magnitude);
            switch (lb->debuff) {
                case DEBUFF_BLEED:
                    printf(" BLEED applied: %d dmg/turn!\n", lb->debuff_magnitude); break;
                case DEBUFF_STUN:
                    printf(" STUN: enemy loses %d action(s) next turn!\n", lb->debuff_magnitude); break;
                case DEBUFF_IMMOBILIZE:
                    printf(" IMMOBILIZED: enemy cannot retreat!\n"); break;
                default:
                    printf(" %s: -%d to enemy stat!\n",
                        DEBUFF_NAMES[lb->debuff], lb->debuff_magnitude); break;
            }
        }

        if (ls->broken_limb_count > 1)
            printf(" [All body attacks now deal +%d%% bonus dmg!]\n",
                ls->broken_limb_count * 15);
    }

    return body_dmg;
}

// =====================================================
// SKILL RESOLUTION (updated for limb bonus)
// =====================================================

void resolve_capacity_strike(Player* p, int skill_idx, Monster* enemy, LimbState* ls) {
    Skill* sk = &p->skills[skill_idx];
    Stat total = calculate_total_stats(p);

    if (sk->rank == 0)    { printf("\n[-] Ability sealed!\n"); return; }
    if (p->current_PM < sk->pm_cost) { printf("\n[-] Not enough PM! (%d/%d)\n", p->current_PM, sk->pm_cost); return; }

    p->current_PM -= sk->pm_cost;
    float lb_bonus = broken_limb_body_bonus(ls);
    printf("\n [SKILL] %s (Rank %d)!\n", sk->name, sk->rank);

    int out_dmg = 0;
    if (p->type == CLASS_WARRIOR) {
        if (skill_idx == 0) { // Shield Slam
            // DEF-weighted; +10% per broken limb on top of global bonus
            out_dmg = (int)((total.STR + (total.DEF * sk->rank)) * 100 / (100 + enemy->stat.DEF) * lb_bonus);
            printf("  Shield Slam hits for %d armour-weighted damage.\n", out_dmg);
        } else if (skill_idx == 1) { // Iron Will
            int recovery = 30 * sk->rank;
            p->current_HP += recovery;
            Stat tot = calculate_total_stats(p);
            if (p->current_HP > tot.HP) p->current_HP = tot.HP;
            printf("  Iron Will: recovered %d HP.\n", recovery);
            return;
        } else { // Juggernaut Overdrive
            out_dmg = (int)(total.STR * 4 * sk->rank * 100 / (100 + enemy->stat.DEF) * lb_bonus);
            // Bonus if enemy below 30% HP
            if (enemy->current_HP * 100 / (enemy->stat.HP > 0 ? enemy->stat.HP : 1) < 30) {
                out_dmg = out_dmg * 3 / 2;
                printf("  EXECUTE! Enemy below 30%% HP — bonus damage!\n");
            }
            printf("  JUGGERNAUT OVERDRIVE! %d demolition damage!\n", out_dmg);
        }
    } else if (p->type == CLASS_MAGE) {
        if (skill_idx == 0) { // Pyroclast
            out_dmg = (int)(total.MAG * (1.8f + sk->rank) * 100 / (100 + enemy->stat.RES / 2) * lb_bonus);
            printf("  Pyroclast burns for %d damage.\n", out_dmg);
        } else if (skill_idx == 1) { // Chrono Shift
            int spd_penalty = 4 * sk->rank;
            enemy->stat.SPD -= spd_penalty;
            if (enemy->stat.SPD < 1) enemy->stat.SPD = 1;
            printf("  Chrono Shift! Enemy SPD reduced by %d.\n", spd_penalty);
            return;
        } else { // Singularity Core
            out_dmg = (int)(total.MAG * 5 * sk->rank * lb_bonus);
            printf("  SINGULARITY CORE! %d true damage (ignores RES)!\n", out_dmg);
        }
    } else if (p->type == CLASS_ROGUE) {
        if (skill_idx == 0) { // Viper Strike — two hits
            int hit1 = total.STR * sk->rank * 100 / (100 + enemy->stat.DEF);
            int hit2 = total.STR * sk->rank * 100 / (100 + enemy->stat.DEF);
            bool crit1 = (rand() % 100 < (total.CRT + 20));
            bool crit2 = (rand() % 100 < (total.CRT + 20));
            if (crit1) hit1 *= 2;
            if (crit2) hit2 *= 2;
            hit1 = (int)(hit1 * lb_bonus);
            hit2 = (int)(hit2 * lb_bonus);
            printf("  Viper Strike: hit 1 = %d%s, hit 2 = %d%s\n",
                hit1, crit1 ? "(CRIT)" : "",
                hit2, crit2 ? "(CRIT)" : "");
            // Apply bleed on any crit
            if (crit1 || crit2) {
                add_debuff(ls, DEBUFF_BLEED, 3 * sk->rank);
                printf("  Crit triggers BLEED: %d/turn!\n", 3 * sk->rank);
            }
            out_dmg = hit1 + hit2;
        } else if (skill_idx == 1) { // Smoke Screen
            printf("  Smoke Screen! Guaranteed escape.\n");
            enemy->current_HP = -999;
            return;
        } else { // Shadow Assassination — ignores all DEF, always crits
            out_dmg = (int)(total.STR * 3 * sk->rank * 2 * lb_bonus);
            printf("  SHADOW ASSASSINATION! %d execution damage (ignores DEF, always crits)!\n", out_dmg);
        }
    }

    if (out_dmg < 1) out_dmg = 1;
    enemy->current_HP -= out_dmg;
}

// =====================================================
// PROGRESSION
// =====================================================

// Compute what the next rank would cost (same formula as upgrade)
static int next_rank_cost(Skill* sk) {
    return (int)(sk->pm_cost * 1.2f);
}

void manage_skill_tree(Player* p) {
    while (1) {
        clear_screen();
        print_header("SKILL TREE");
        printf(" Class: %s   Level: %d\n", p->class_name, p->level);
        print_divider();

        for (int i = 0; i < MAX_SKILLS_PER_CLASS; i++) {
            Skill* sk = &p->skills[i];
            printf("  %d. %-24s\n", i + 1, sk->name);

            if (sk->rank == 0) {
                printf("     Status  : LOCKED  (requires Level %d)\n", sk->min_player_level);
                printf("     Unlock  : %d PM cost at Rank 1\n", sk->pm_cost);
            } else {
                // Current rank info
                printf("     Current : Rank %d  |  Cost: %d PM\n", sk->rank, sk->pm_cost);
                // Next rank preview
                if (p->level >= sk->min_player_level)
                    printf("     Upgrade : Rank %d  |  Cost: %d PM  [AVAILABLE]\n",
                        sk->rank + 1, next_rank_cost(sk));
                else
                    printf("     Upgrade : Rank %d  |  Cost: %d PM  (Req. Lv.%d)\n",
                        sk->rank + 1, next_rank_cost(sk), sk->min_player_level);
            }
            printf("     %s\n\n", sk->description);
        }

        printf("  0. Back\n> ");
        int c = 0;
        if (scanf("%d", &c) != 1) c = 0;
        while (getchar() != '\n');
        if (c < 1 || c > MAX_SKILLS_PER_CLASS) break;

        int idx = c - 1;
        if (p->level < p->skills[idx].min_player_level) {
            printf("\n Level too low (need %d, have %d).\n",
                p->skills[idx].min_player_level, p->level);
            press_enter();
            continue;
        }
        p->skills[idx].pm_cost = next_rank_cost(&p->skills[idx]);
        p->skills[idx].rank++;
        printf("\n %s upgraded to Rank %d! New cost: %d PM\n",
            p->skills[idx].name, p->skills[idx].rank, p->skills[idx].pm_cost);
        press_enter();
    }
}

void check_level_up(Player* p) {
    while (p->exp >= p->exp_needed) {
        p->exp -= p->exp_needed;
        p->level++;
        p->exp_needed = (int)(p->exp_needed * 1.5f);

        clear_screen();
        print_header("LEVEL UP!");
        draw_class(p->type);
        printf(" You are now Level %d!\n", p->level);
        print_divider();

        int points = 3;
        while (points > 0) {
            printf(" Stat points left: %d\n", points);
            printf("  1. HP  (+15)  [now: %d]\n", p->base_stat.HP);
            printf("  2. PM  (+10)  [now: %d]\n", p->base_stat.PM);
            printf("  3. STR (+3)   [now: %d]\n", p->base_stat.STR);
            printf("  4. MAG (+3)   [now: %d]\n", p->base_stat.MAG);
            printf("  5. DEF (+2)   [now: %d]\n", p->base_stat.DEF);
            printf("  6. SPD (+2)   [now: %d]\n", p->base_stat.SPD);
            printf("> ");
            int ch = 0;
            if (scanf("%d", &ch) != 1) ch = 1;
            while (getchar() != '\n');
            switch (ch) {
                case 1: p->base_stat.HP  += 15; points--; break;
                case 2: p->base_stat.PM  += 10; points--; break;
                case 3: p->base_stat.STR += 3;  points--; break;
                case 4: p->base_stat.MAG += 3;  points--; break;
                case 5: p->base_stat.DEF += 2;  points--; break;
                case 6: p->base_stat.SPD += 2;  points--; break;
                default: break;
            }
        }
        Stat tot = calculate_total_stats(p);
        p->current_HP = tot.HP;
        p->current_PM = tot.PM;
        printf("\n Stats updated.\n");
        press_enter();
        manage_skill_tree(p);
    }
}

// =====================================================
// EQUIPMENT — SINGLE-SCREEN OVERVIEW + CHANGE
// Shows all three slots AND all discovered gear at once.
// =====================================================

static void print_gear_stat_line(Gear* g) {
    // Print only non-zero modifiers in a compact format
    printf("     ");
    if (g->modifier.HP)  printf("HP:%+d ", g->modifier.HP);
    if (g->modifier.PM)  printf("PM:%+d ", g->modifier.PM);
    if (g->modifier.STR) printf("STR:%+d ", g->modifier.STR);
    if (g->modifier.MAG) printf("MAG:%+d ", g->modifier.MAG);
    if (g->modifier.DEF) printf("DEF:%+d ", g->modifier.DEF);
    if (g->modifier.RES) printf("RES:%+d ", g->modifier.RES);
    if (g->modifier.SPD) printf("SPD:%+d ", g->modifier.SPD);
    if (g->modifier.CRT) printf("CRT:%+d ", g->modifier.CRT);
    printf("\n");
}

static const char* slot_names[] = {"Weapon","Armor","Relic"};

static void manage_equipment(void) {
    Player* p = &G.player;

    while (1) {
        clear_screen();
        print_header("EQUIPMENT");

        // ---- Current loadout overview ----
        printf(" EQUIPPED\n");
        print_divider();
        for (int s = 0; s < 3; s++) {
            if (p->slots[s]) {
                printf("  %-7s: %s\n", slot_names[s], p->slots[s]->name);
                print_gear_stat_line(p->slots[s]);
            } else {
                printf("  %-7s: (empty)\n", slot_names[s]);
            }
        }

        // Combined stat preview with current gear
        Stat tot = calculate_total_stats(p);
        printf("\n TOTAL STATS WITH GEAR\n");
        print_divider();
        printf("  HP:%-4d  PM:%-4d  STR:%-3d  MAG:%-3d\n",
            tot.HP, tot.PM, tot.STR, tot.MAG);
        printf("  DEF:%-3d  RES:%-3d  SPD:%-3d  CRT:%-3d%%\n",
            tot.DEF, tot.RES, tot.SPD, tot.CRT);

        // ---- Available discovered gear ----
        printf("\n DISCOVERED GEAR\n");
        print_divider();
        int item_keys[EQUIPMENT_COUNT];
        int di = 1;
        for (int i = 0; i < (int)EQUIPMENT_COUNT; i++) {
            if (EQUIPMENT_POOL[i].discovered) {
                // Mark if currently equipped
                bool equipped = false;
                for (int s = 0; s < 3; s++)
                    if (p->slots[s] == &EQUIPMENT_POOL[i]) equipped = true;

                printf("  %2d. [%-6s] %-24s%s\n",
                    di,
                    slot_names[EQUIPMENT_POOL[i].slot],
                    EQUIPMENT_POOL[i].name,
                    equipped ? " <EQUIPPED>" : "");
                print_gear_stat_line(&EQUIPMENT_POOL[i]);
                item_keys[di++] = i;
            }
        }

        printf("\n  0. Back\n");
        print_divider();
        printf(" Enter item number to equip/unequip, or 0 to back: ");

        int ic = 0;
        if (scanf("%d", &ic) != 1) ic = 0;
        while (getchar() != '\n');
        if (ic <= 0 || ic >= di) break;

        int gear_idx = item_keys[ic];
        Gear* g = &EQUIPMENT_POOL[gear_idx];
        SlotType target_slot = g->slot;

        // If already equipped in its slot, unequip; otherwise equip
        if (p->slots[target_slot] == g) {
            p->slots[target_slot] = NULL;
            printf("\n %s unequipped.\n", g->name);
        } else {
            p->slots[target_slot] = g;
            printf("\n %s equipped to %s slot.\n", g->name, slot_names[target_slot]);
        }
        press_enter();
    }
}

// =====================================================
// CLASS SELECTION
// =====================================================

static void choose_class(void) {
    clear_screen();
    print_header("CHOOSE YOUR CLASS");
    printf("  1. Warrior  - High HP & DEF, melee powerhouse\n");
    printf("  2. Mage     - High PM & MAG, arcane destroyer\n");
    printf("  3. Rogue    - High SPD & CRT, burst assassin\n\n> ");

    int c = 0;
    if (scanf("%d", &c) != 1) c = 1;
    while (getchar() != '\n');

    Player* p = &G.player;
    memset(p, 0, sizeof(Player));
    p->level = 1; p->exp = 0; p->exp_needed = 100;

    switch (c) {
        case 2:
            p->type = CLASS_MAGE; strcpy(p->class_name, "Mage");
            p->base_stat = (Stat){60,120,5,30,5,20,12,10};
            memcpy(p->skills, MAGE_TREE, sizeof(MAGE_TREE)); break;
        case 3:
            p->type = CLASS_ROGUE; strcpy(p->class_name, "Rogue");
            p->base_stat = (Stat){80,40,18,10,10,10,30,25};
            memcpy(p->skills, ROGUE_TREE, sizeof(ROGUE_TREE)); break;
        default:
            p->type = CLASS_WARRIOR; strcpy(p->class_name, "Warrior");
            p->base_stat = (Stat){150,20,25,5,30,15,10,5};
            memcpy(p->skills, WARRIOR_TREE, sizeof(WARRIOR_TREE)); break;
    }

    p->current_HP = calculate_total_stats(p).HP;
    p->current_PM = calculate_total_stats(p).PM;
    inventory_add(&p->inventory, ITEM_POTION, 2);
    inventory_add(&p->inventory, ITEM_ETHER,  1);

    clear_screen();
    draw_class(p->type);
    printf(" You chose %s!\n", p->class_name);
    press_enter();
}

// =====================================================
// COMBAT
// =====================================================

static FightResult run_fight(Monster* enemy) {
    Player* p = &G.player;

    LimbState ls = build_limb_state(enemy->id);
    if (enemy->is_boss) {
        for (int i = 0; i < ls.limb_count; i++) {
            ls.limbs[i].max_hp = (int)(ls.limbs[i].max_hp * 1.5f);
            ls.limbs[i].hp = ls.limbs[i].max_hp;
        }
    }

    clear_screen();
    draw_monster(enemy->id, enemy->is_boss);
    print_header(enemy->is_boss ? "BOSS ENCOUNTER" : "ENCOUNTER");

    while (1) {
        // ------ PLAYER TURN ------
        Stat total_p = calculate_total_stats(p);
        int player_actions = actions_for(&total_p);

        for (int action = 0; action < player_actions; action++) {
            if (p->current_HP <= 0 || enemy->current_HP <= 0) break;

            printf("\n");
            print_divider();
            print_combat_bars(p, enemy);
            print_limb_state(&ls);
            print_divider();
            printf(" Action (%d/%d):\n", action + 1, player_actions);
            printf("  1. Physical Attack\n");
            printf("  2. Magic Attack   (4 PM)\n");
            printf("  3. Target Limb - Physical\n");
            printf("  4. Target Limb - Magic    (4 PM)\n");
            printf("  5. Use Skill\n");
            printf("  6. Potion [%d]   7. Ether [%d]\n",
                inventory_count(&p->inventory, ITEM_POTION),
                inventory_count(&p->inventory, ITEM_ETHER));
            printf("  8. Flee\n> ");

            int ch = 0;
            if (scanf("%d", &ch) != 1) ch = 1;
            while (getchar() != '\n');

            int dmg = 0;
            switch (ch) {
                case 1: { // Physical
                    dmg = total_p.STR * 100 / (100 + enemy->stat.DEF);
                    // Apply broken limb bonus to plain attacks too
                    dmg = (int)(dmg * broken_limb_body_bonus(&ls));
                    if (rand() % 100 < total_p.CRT) { dmg *= 2; printf(" CRITICAL HIT!\n"); }
                    if (dmg < 1) dmg = 1;
                    enemy->current_HP -= dmg;
                    printf(" Physical attack: %d damage.\n", dmg);
                    break;
                }
                case 2: { // Magic
                    if (p->current_PM < 4) { printf("\n Not enough PM.\n"); action--; continue; }
                    p->current_PM -= 4;
                    dmg = total_p.MAG * 100 / (100 + enemy->stat.RES);
                    dmg = (int)(dmg * broken_limb_body_bonus(&ls));
                    if (dmg < 1) dmg = 1;
                    enemy->current_HP -= dmg;
                    printf(" Magic attack: %d damage.\n", dmg);
                    break;
                }
                case 3: { // Limb physical
                    dmg = do_limb_attack(p, enemy, &ls, false);
                    if (dmg > 0) enemy->current_HP -= dmg;
                    else { action--; continue; }
                    break;
                }
                case 4: { // Limb magic
                    if (p->current_PM < 4) { printf("\n Not enough PM.\n"); action--; continue; }
                    dmg = do_limb_attack(p, enemy, &ls, true);
                    if (dmg > 0) enemy->current_HP -= dmg;
                    else { action--; continue; }
                    break;
                }
                case 5: { // Skill
                    clear_screen();
                    print_header("SKILLS");
                    for (int i = 0; i < MAX_SKILLS_PER_CLASS; i++) {
                        Skill* sk = &p->skills[i];
                        if (sk->rank > 0)
                            printf("  %d. %-22s Rank %d  (%d PM)\n",
                                i+1, sk->name, sk->rank, sk->pm_cost);
                        else
                            printf("  %d. [LOCKED] %s (Req. Lv.%d)\n",
                                i+1, sk->name, sk->min_player_level);
                    }
                    printf("  0. Back\n> ");
                    int sk = 0;
                    if (scanf("%d", &sk) != 1) sk = 0;
                    while (getchar() != '\n');
                    if (sk < 1 || sk > 3 || p->skills[sk-1].rank == 0) { action--; continue; }
                    resolve_capacity_strike(p, sk-1, enemy, &ls);
                    if (enemy->current_HP == -999) return FIGHT_FLED;
                    break;
                }
                case 6: { // Potion
                    if (!inventory_use(&p->inventory, ITEM_POTION)) {
                        printf("\n No potions!\n"); action--; continue;
                    }
                    int heal = rand_range(35, 55);
                    p->current_HP += heal;
                    if (p->current_HP > total_p.HP) p->current_HP = total_p.HP;
                    printf(" Potion: recovered %d HP.\n", heal);
                    break;
                }
                case 7: { // Ether
                    if (!inventory_use(&p->inventory, ITEM_ETHER)) {
                        printf("\n No ethers!\n"); action--; continue;
                    }
                    int restore = rand_range(20, 35);
                    p->current_PM += restore;
                    if (p->current_PM > total_p.PM) p->current_PM = total_p.PM;
                    printf(" Ether: recovered %d PM.\n", restore);
                    break;
                }
                case 8: { // Flee
                    if (enemy->is_boss) { printf("\n Can't flee a boss!\n"); action--; continue; }
                    if (rand() % 100 < 40) { printf("\n Fled!\n"); return FIGHT_FLED; }
                    else { printf("\n Failed to flee!\n"); }
                    break;
                }
                default: action--; continue;
            }
            press_enter();
            if (enemy->current_HP <= 0) break;
        }

        if (enemy->current_HP <= 0) {
            printf("\n %s defeated!\n", enemy->name);
            return FIGHT_WIN;
        }
        if (p->current_HP <= 0) {
            printf("\n You died.\n");
            return FIGHT_LOSE;
        }

        // ------ BLEED TICK ------
        int bleed = tick_debuffs(&ls);
        if (bleed > 0) {
            enemy->current_HP -= bleed;
            printf("\n [BLEED] %s bleeds for %d damage!\n", enemy->name, bleed);
            if (enemy->current_HP <= 0) {
                printf(" %s bled out!\n", enemy->name);
                return FIGHT_WIN;
            }
        }

        // ------ ENEMY TURN ------
        Stat eff_enemy = apply_debuffs_to_stat(enemy->stat, &ls);
        int enemy_actions = actions_for(&eff_enemy);

        if (has_debuff(&ls, DEBUFF_STUN)) {
            int stun_lost = 1;
            enemy_actions -= stun_lost;
            if (enemy_actions < 0) enemy_actions = 0;
            printf("\n [STUN] %s loses 1 action this turn!\n", enemy->name);
        }

        if (enemy_actions > 0) {
            printf("\n [ENEMY] %s attacks %d time(s)!\n", enemy->name, enemy_actions);
            for (int ea = 0; ea < enemy_actions; ea++) {
                if (p->current_HP <= 0) break;
                Stat total_p2 = calculate_total_stats(p);
                int edm;
                if (eff_enemy.MAG > 5 && rand() % 10 < 3) {
                    edm = eff_enemy.MAG * 100 / (100 + total_p2.RES);
                    printf("  %s casts for %d magic damage.\n", enemy->name, edm);
                } else {
                    edm = eff_enemy.STR * 100 / (100 + total_p2.DEF);
                    if (rand() % 100 < eff_enemy.CRT) {
                        edm *= 2;
                        printf("  ENEMY CRITICAL!\n");
                    }
                    printf("  %s strikes for %d damage.\n", enemy->name, edm);
                }
                if (edm < 1) edm = 1;
                p->current_HP -= edm;
            }
        }
        press_enter();

        if (p->current_HP <= 0) { printf("\n You were defeated.\n"); return FIGHT_LOSE; }
    }
}

// =====================================================
// ENCOUNTER SEEDING
// =====================================================

void seed_encounter_node(void) {
    int pool[MONSTER_COUNT], count = 0;
    for (int i = 0; i < MONSTER_COUNT; i++) {
        for (int d = 0; d < MONSTER_DB[i].dungeon_count; d++) {
            if (MONSTER_DB[i].allowed_dungeons[d] == G.dungeon_type) {
                pool[count++] = i; break;
            }
        }
    }
    int id = (count > 0) ? pool[rand() % count] : 0;
    MonsterTemplate t = MONSTER_DB[id];

    G.current_enemy.id   = t.id;
    G.current_enemy.name = t.name;
    G.current_enemy.stat = generate_stat(t.min_stat, t.max_stat);

    float scale = 1.0f + (G.floor * 0.12f);
    G.current_enemy.stat.HP  = (int)(G.current_enemy.stat.HP  * scale);
    G.current_enemy.stat.STR = (int)(G.current_enemy.stat.STR * scale);
    G.current_enemy.stat.DEF = (int)(G.current_enemy.stat.DEF * scale);
    G.current_enemy.stat.RES = (int)(G.current_enemy.stat.RES * scale);

    G.current_enemy.is_boss = (G.floor % BOSS_EVERY == 0);
    if (G.current_enemy.is_boss) {
        G.current_enemy.stat.HP *= 2;
        strcpy(G.current_enemy.boss_title, BOSS_TITLES[rand() % BOSS_TITLE_COUNT]);
    } else {
        strcpy(G.current_enemy.boss_title, "");
    }
    G.current_enemy.current_HP = G.current_enemy.stat.HP;
}

void trigger_loot_discovery(void) {
    int roll = rand() % 100;
    if (roll < 40) {
        int loot_idx = rand() % EQUIPMENT_COUNT;
        if (!EQUIPMENT_POOL[loot_idx].discovered) {
            EQUIPMENT_POOL[loot_idx].discovered = true;
            printf("\n [GEAR FOUND] \"%s\" added to armory!\n", EQUIPMENT_POOL[loot_idx].name);
            return;
        }
    }
    ItemType drop = (rand() % 2 == 0) ? ITEM_POTION : ITEM_ETHER;
    inventory_add(&G.player.inventory, drop, 1);
    printf("\n [LOOT] Found a %s!\n", drop == ITEM_POTION ? "Potion" : "Ether");
}

// =====================================================
// EVENTS
// =====================================================

void process_event(void) {
    int roll = rand() % 100;
    bool is_boss_floor = (G.floor % BOSS_EVERY == 0);

    if (roll < 60 || is_boss_floor) {
        seed_encounter_node();
        FightResult res = run_fight(&G.current_enemy);
        if (res == FIGHT_WIN) {
            clear_screen();
            print_header("VICTORY");
            int exp_gain = G.current_enemy.is_boss
                ? 100 + (G.floor * 20)
                : 25  + (G.floor * 5);
            G.player.exp += exp_gain;
            printf(" EXP gained: +%d\n", exp_gain);
            trigger_loot_discovery();
            press_enter();
            check_level_up(&G.player);
            G.floor++;
        } else if (res == FIGHT_LOSE) {
            G.state = STATE_GAMEOVER;
        } else {
            printf("\n Retreated. Floor unchanged.\n");
            press_enter();
        }
    } else if (roll < 75) {
        clear_screen();
        print_header("CAMPFIRE");
        printf("  1. Rest        - recover 50%% HP\n");
        printf("  2. Meditate    - gain 2 Ethers\n> ");
        int c = 1; if (scanf("%d",&c)!=1) c=1; while(getchar()!='\n');
        Stat tot = calculate_total_stats(&G.player);
        if (c == 1) {
            G.player.current_HP += tot.HP / 2;
            if (G.player.current_HP > tot.HP) G.player.current_HP = tot.HP;
            printf("\n HP recovered.\n");
        } else {
            inventory_add(&G.player.inventory, ITEM_ETHER, 2);
            printf("\n Gained 2 Ethers.\n");
        }
        press_enter(); G.floor++;
    } else if (roll < 90) {
        clear_screen();
        print_header("WANDERING ALCHEMIST");
        printf("  1. Trade 15 max HP  for +3 STR\n");
        printf("  2. Trade 15 max PM  for +3 DEF\n");
        printf("  3. Decline\n> ");
        int c = 3; if (scanf("%d",&c)!=1) c=3; while(getchar()!='\n');
        if (c == 1 && G.player.base_stat.HP > 20) {
            G.player.base_stat.HP -= 15; G.player.base_stat.STR += 3;
            printf("\n STR +3 (HP max reduced by 15).\n");
        } else if (c == 2 && G.player.base_stat.PM > 20) {
            G.player.base_stat.PM -= 15; G.player.base_stat.DEF += 3;
            printf("\n DEF +3 (PM max reduced by 15).\n");
        } else { printf("\n Declined.\n"); }
        Stat tot = calculate_total_stats(&G.player);
        if (G.player.current_HP > tot.HP) G.player.current_HP = tot.HP;
        if (G.player.current_PM > tot.PM) G.player.current_PM = tot.PM;
        press_enter(); G.floor++;
    } else {
        clear_screen();
        print_header("TRAINING ROOM");
        G.player.exp += 40;
        printf("\n Old training logs found. +40 EXP.\n");
        press_enter();
        check_level_up(&G.player);
        G.floor++;
    }
}

// =====================================================
// CAMP INTERMISSION
// Now includes: potion/ether use, full status bar display
// =====================================================

void process_camp_intermission(void) {
    Player* p = &G.player;

    while (1) {
        clear_screen();
        print_header("CAMP");
        printf(" Zone: %s | Floor: %d\n", G.dungeon_name, G.floor);
        print_divider();
        print_player_status(p);
        print_divider();

        printf("  1. Advance into dungeon\n");
        printf("  2. View full stats\n");
        printf("  3. Equipment\n");
        printf("  4. Skill tree\n");
        printf("  5. Drink Potion  [%d available]  (+35~55 HP)\n",
            inventory_count(&p->inventory, ITEM_POTION));
        printf("  6. Drink Ether   [%d available]  (+20~35 PM)\n",
            inventory_count(&p->inventory, ITEM_ETHER));
        printf("  0. Quit to menu\n> ");

        int c = 1;
        if (scanf("%d",&c)!=1) c=1;
        while (getchar() != '\n');

        Stat tot;
        switch (c) {
            case 1:
                process_event();
                // After an event the camp loop continues (unless game over)
                if (G.state != STATE_CAMP) return;
                break;
            case 2: {
                clear_screen();
                print_header("FULL STATS");
                tot = calculate_total_stats(p);
                draw_class(p->type);
                printf("  Class  : %s   Level : %d   EXP: %d/%d\n",
                    p->class_name, p->level, p->exp, p->exp_needed);
                print_bar("HP", p->current_HP, tot.HP, HP_BAR_WIDTH);
                print_bar("PM", p->current_PM, tot.PM, HP_BAR_WIDTH);
                print_divider();
                printf("  STR:%-4d  MAG:%-4d  DEF:%-4d\n", tot.STR, tot.MAG, tot.DEF);
                printf("  RES:%-4d  SPD:%-4d  CRT:%-3d%%  Actions/turn: %d\n",
                    tot.RES, tot.SPD, tot.CRT, actions_for(&tot));
                print_divider();
                printf("  Weapon : %s\n", p->slots[SLOT_WEAPON] ? p->slots[SLOT_WEAPON]->name : "None");
                printf("  Armor  : %s\n", p->slots[SLOT_ARMOR]  ? p->slots[SLOT_ARMOR]->name  : "None");
                printf("  Relic  : %s\n", p->slots[SLOT_RELIC]  ? p->slots[SLOT_RELIC]->name  : "None");
                press_enter();
                break;
            }
            case 3: manage_equipment(); break;
            case 4: manage_skill_tree(p); break;
            case 5: { // Potion at camp
                if (!inventory_use(&p->inventory, ITEM_POTION)) {
                    printf("\n No potions!\n"); press_enter(); break;
                }
                tot = calculate_total_stats(p);
                int heal = rand_range(35, 55);
                p->current_HP += heal;
                if (p->current_HP > tot.HP) p->current_HP = tot.HP;
                printf("\n Drank a Potion. Recovered %d HP.\n", heal);
                press_enter();
                break;
            }
            case 6: { // Ether at camp
                if (!inventory_use(&p->inventory, ITEM_ETHER)) {
                    printf("\n No ethers!\n"); press_enter(); break;
                }
                tot = calculate_total_stats(p);
                int restore = rand_range(20, 35);
                p->current_PM += restore;
                if (p->current_PM > tot.PM) p->current_PM = tot.PM;
                printf("\n Drank an Ether. Recovered %d PM.\n", restore);
                press_enter();
                break;
            }
            case 0:
                G.state = STATE_MENU;
                return;
            default: break;
        }
    }
}

// =====================================================
// MAIN
// =====================================================

int main(void) {
    srand((unsigned int)time(NULL));
    G.state = STATE_MENU;

    while (1) {
        if (G.state == STATE_MENU) {
            clear_screen();
            print_header("ROGUELITE");
            printf("  1. New Game\n  0. Quit\n> ");
            int c = 0;
            if (scanf("%d",&c)!=1) c=0;
            while (getchar()!='\n');
            if (c != 1) { printf("Goodbye.\n"); break; }
            choose_class();
            G.dungeon_type = (DungeonType)rand_range(0, DUNGEON_TYPE_COUNT - 1);
            generate_dungeon_name(G.dungeon_type, G.dungeon_name);
            G.floor = 1;
            G.state = STATE_CAMP;
        } else if (G.state == STATE_CAMP) {
            process_camp_intermission();
        } else if (G.state == STATE_GAMEOVER) {
            clear_screen();
            print_header("GAME OVER");
            printf(" Defeated on floor %d in %s.\n", G.floor, G.dungeon_name);
            press_enter();
            G.state = STATE_MENU;
        }
    }
    return 0;
}