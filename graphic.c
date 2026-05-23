// =====================================================
// ASCII ART SYSTEM
// =====================================================
// HOW TO ADD ART:
//   1. In draw_monster(): find the case for the monster ID and replace the
//      placeholder printf lines with your actual ASCII art.
//   2. In draw_class(): find the case for the class and do the same.
//   3. Art is printed to the left of the combat status block.
//   Keep art to roughly 12 lines x 28 characters for clean formatting.
// =====================================================
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "graphic.h"


void draw_monster(int monster_id, bool is_boss) {
    // Each case: replace the placeholder block with real ASCII art.
    // Prefix each line with "  " (two spaces) for alignment.
    printf("\n");
    switch (monster_id) {
        case 0: // Slime
            /* ---- PLACEHOLDER: SLIME ASCII ART ----
               Example shape ~5 lines, replace below  */
            printf("  .  ~ SLIME ~ .\n");
            printf("  ( placeholder )\n");
            printf("  '~~~~~~~~~~~~~'\n");
            break;
        case 1: // Bandit
            printf("  .  ~ BANDIT ~ .\n");
            printf("  ( placeholder )\n");
            printf("  '~~~~~~~~~~~~~'\n");
            break;
        case 2: // Goblin
            printf("  .  ~ GOBLIN ~ .\n");
            printf("  ( placeholder )\n");
            printf("  '~~~~~~~~~~~~~'\n");
            break;
        case 3: // Goblin Archer
            printf("  . ~GOB ARCHER~.\n");
            printf("  ( placeholder )\n");
            printf("  '~~~~~~~~~~~~~'\n");
            break;
        case 4: // Mercenary
            printf("  . ~MERCENARY~. \n");
            printf("  ( placeholder )\n");
            printf("  '~~~~~~~~~~~~~'\n");
            break;
        case 5: // Wild Dog
            printf("  . ~ WILD DOG ~.\n");
            printf("  ( placeholder )\n");
            printf("  '~~~~~~~~~~~~~'\n");
            break;
        case 6: // Cave Bat
            printf("  . ~ CAVE BAT ~.\n");
            printf("  ( placeholder )\n");
            printf("  '~~~~~~~~~~~~~'\n");
            break;
        case 7: // Rock Golem
            printf("  . ~ROCK GOLEM~.\n");
            printf("  ( placeholder )\n");
            printf("  '~~~~~~~~~~~~~'\n");
            break;
        case 8: // Spider
            printf("  . ~ SPIDER ~. \n");
            printf("  ( placeholder )\n");
            printf("  '~~~~~~~~~~~~~'\n");
            break;
        case 9: // Venom Spider
            printf("  . ~VNM SPIDER~.\n");
            printf("  ( placeholder )\n");
            printf("  '~~~~~~~~~~~~~'\n");
            break;
        case 10: // Cave Troll
            printf("  . ~CAVE TROLL~.\n");
            printf("  ( placeholder )\n");
            printf("  '~~~~~~~~~~~~~'\n");
            break;
        case 11: // Crystal Worm
            printf("  . ~CRYST WORM~.\n");
            printf("  ( placeholder )\n");
            printf("  '~~~~~~~~~~~~~'\n");
            break;
        case 12: // Wolf
            printf("  . ~ WOLF  ~ . \n");
            printf("  ( placeholder )\n");
            printf("  '~~~~~~~~~~~~~'\n");
            break;
        case 13: // Giant Ant
            printf("  . ~GIANT ANT~. \n");
            printf("  ( placeholder )\n");
            printf("  '~~~~~~~~~~~~~'\n");
            break;
        case 14: // Boar
            printf("  . ~  BOAR  ~ . \n");
            printf("  ( placeholder )\n");
            printf("  '~~~~~~~~~~~~~'\n");
            break;
        case 15: // Forest Spirit
            printf("  . ~FRST SPIRT~.\n");
            printf("  ( placeholder )\n");
            printf("  '~~~~~~~~~~~~~'\n");
            break;
        case 16: // Dryad
            printf("  . ~  DRYAD ~ . \n");
            printf("  ( placeholder )\n");
            printf("  '~~~~~~~~~~~~~'\n");
            break;
        case 17: // Bear
            printf("  . ~  BEAR  ~ . \n");
            printf("  ( placeholder )\n");
            printf("  '~~~~~~~~~~~~~'\n");
            break;
        case 18: // Cultist
            printf("  . ~ CULTIST~. \n");
            printf("  ( placeholder )\n");
            printf("  '~~~~~~~~~~~~~'\n");
            break;
        case 19: // Dark Priest
            printf("  . ~DRK PRIEST~.\n");
            printf("  ( placeholder )\n");
            printf("  '~~~~~~~~~~~~~'\n");
            break;
        case 20: // Temple Guardian
            printf("  . ~TMPL GRDN~. \n");
            printf("  ( placeholder )\n");
            printf("  '~~~~~~~~~~~~~'\n");
            break;
        case 21: // Possessed Armor
            printf("  . ~POSS ARMOR~.\n");
            printf("  ( placeholder )\n");
            printf("  '~~~~~~~~~~~~~'\n");
            break;
        case 22: // Ancient Mage
            printf("  . ~ANC. MAGE~. \n");
            printf("  ( placeholder )\n");
            printf("  '~~~~~~~~~~~~~'\n");
            break;
        case 23: // Holy Wraith
            printf("  . ~HLY WRAITH~.\n");
            printf("  ( placeholder )\n");
            printf("  '~~~~~~~~~~~~~'\n");
            break;
        case 24: // Dark Knight
            printf("  . ~DRK KNIGHT~.\n");
            printf("  ( placeholder )\n");
            printf("  '~~~~~~~~~~~~~'\n");
            break;
        case 25: // Royal Guard
            printf("  . ~ROYAL GARD~.\n");
            printf("  ( placeholder )\n");
            printf("  '~~~~~~~~~~~~~'\n");
            break;
        case 26: // Executioner
            printf("  . ~EXECUTNR~.  \n");
            printf("  ( placeholder )\n");
            printf("  '~~~~~~~~~~~~~'\n");
            break;
        case 27: // Necromancer
            printf("  . ~NECROMNCR~. \n");
            printf("  ( placeholder )\n");
            printf("  '~~~~~~~~~~~~~'\n");
            break;
        case 28: // Vampire
            printf("  . ~ VAMPIRE~.  \n");
            printf("  ( placeholder )\n");
            printf("  '~~~~~~~~~~~~~'\n");
            break;
        case 29: // Lich
            printf("  . ~  LICH  ~.  \n");
            printf("  ( placeholder )\n");
            printf("  '~~~~~~~~~~~~~'\n");
            break;
        case 30: // Mimic
            printf("  . ~  MIMIC ~.  \n");
            printf("  ( placeholder )\n");
            printf("  '~~~~~~~~~~~~~'\n");
            break;
        case 31: // Wandering Spirit
            printf("  . ~WNDG SPIRT~.\n");
            printf("  ( placeholder )\n");
            printf("  '~~~~~~~~~~~~~'\n");
            break;
        case 32: // Wyrm
            printf("  . ~  WYRM  ~.  \n");
            printf("  ( placeholder )\n");
            printf("  '~~~~~~~~~~~~~'\n");
            break;
        case 33: // Ancient Dragon
            printf("  . ~ANC DRAGON~.\n");
            printf("  ( placeholder )\n");
            printf("  '~~~~~~~~~~~~~'\n");
            break;
        default:
            printf("  . ~ UNKNOWN ~. \n");
            printf("  ( placeholder )\n");
            printf("  '~~~~~~~~~~~~~'\n");
            break;
    }
    if (is_boss) printf("  [!!! BOSS !!!]\n");
    printf("\n");
}

// Draw player class art (shown at camp / level up)
void draw_class(int class_type) {
    // class_type: 0=Warrior, 1=Mage, 2=Rogue
    // Replace placeholder prints with real ASCII art per class.
    printf("\n");
    switch (class_type) {
        case 0: // Warrior
            /* ---- PLACEHOLDER: WARRIOR ASCII ART ---- */
            printf("  . ~ WARRIOR ~. \n");
            printf("  ( placeholder )\n");
            printf("  '~~~~~~~~~~~~~'\n");
            break;
        case 1: // Mage
            /* ---- PLACEHOLDER: MAGE ASCII ART ---- */
            printf("  . ~  MAGE  ~.  \n");
            printf("  ( placeholder )\n");
            printf("  '~~~~~~~~~~~~~'\n");
            break;
        case 2: // Rogue
            /* ---- PLACEHOLDER: ROGUE ASCII ART ---- */
            printf("  . ~  ROGUE ~.  \n");
            printf("  ( placeholder )\n");
            printf("  '~~~~~~~~~~~~~'\n");
            break;
        default:
            printf("  . ~ UNKNOWN~.  \n");
            printf("  ( placeholder )\n");
            printf("  '~~~~~~~~~~~~~'\n");
            break;
    }
    printf("\n");
}