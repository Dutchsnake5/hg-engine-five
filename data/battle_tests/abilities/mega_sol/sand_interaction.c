// Test: Mega Sol - Ignores Sand Veil and Rock type special defense boost
#include "../../battle_tests.h"
BEGIN_TEST
{
    .battleType = BATTLE_TYPE_TRAINER,
    .weather = FIELD_CONDITION_NONE,
    .fieldCondition = 0,
    .terrain = TERRAIN_NONE,
    .playerParty = {
        {
            .species = SPECIES_GOLEM,
            // level 55, not 50: this hack's stronger Absorb (30) knocked out a level 50 Golem on the top rolls, and
            // losing the battle left the next test unable to start
            .level = 55,
            .form = 0,
            .ability = ABILITY_SAND_VEIL,
            .item = ITEM_NONE,
            .moves = { MOVE_SANDSTORM, MOVE_NONE, MOVE_NONE, MOVE_NONE },
            .hp = FULL_HP,
            .status = 0,
            .condition2 = 0,
            .moveEffectFlags = 0,
        },
        { .species = SPECIES_NONE },
        { .species = SPECIES_NONE },
        { .species = SPECIES_NONE },
        { .species = SPECIES_NONE },
        { .species = SPECIES_NONE }
    },
    .enemyParty = { {
                        .species = SPECIES_MEGANIUM,
                        .level = 50,
                        .form = 0,
                        .ability = ABILITY_OVERGROW,
                        .item = ITEM_MEGANIUMITE,
                        .moves = { MOVE_ABSORB, MOVE_SLEEP_TALK, MOVE_NONE, MOVE_NONE },
                        .hp = 76,
                        .status = 0,
                        .condition2 = 0,
                        .moveEffectFlags = 0,
                    },
        { .species = SPECIES_NONE },
        { .species = SPECIES_NONE },
        { .species = SPECIES_NONE },
        { .species = SPECIES_NONE },
        { .species = SPECIES_NONE } },
    .playerScript = { {
                          { ACTION_MOVE_SLOT_1, BATTLER_ENEMY_FIRST },
                          { ACTION_MOVE_SLOT_1, BATTLER_ENEMY_FIRST },
                          { ACTION_NONE, 0 },
                          { ACTION_NONE, 0 },
                          { ACTION_NONE, 0 },
                          { ACTION_NONE, 0 },
                          { ACTION_NONE, 0 },
                          { ACTION_NONE, 0 },
                      },
        {
            { ACTION_NONE, 0 },
            { ACTION_NONE, 0 },
            { ACTION_NONE, 0 },
            { ACTION_NONE, 0 },
            { ACTION_NONE, 0 },
            { ACTION_NONE, 0 },
            { ACTION_NONE, 0 },
            { ACTION_NONE, 0 },
        } },
    .enemyScript = { {
                         { ACTION_MOVE_SLOT_2, BATTLER_ENEMY_FIRST },
                         { ACTION_MOVE_SLOT_1, BATTLER_ENEMY_FIRST },
                         { ACTION_NONE, 0 },
                         { ACTION_NONE, 0 },
                         { ACTION_NONE, 0 },
                         { ACTION_NONE, 0 },
                         { ACTION_NONE, 0 },
                         { ACTION_NONE, 0 },
                     },
        {
            { ACTION_NONE, 0 },
            { ACTION_NONE, 0 },
            { ACTION_NONE, 0 },
            { ACTION_NONE, 0 },
            { ACTION_NONE, 0 },
            { ACTION_NONE, 0 },
            { ACTION_NONE, 0 },
            { ACTION_NONE, 0 },
        } },
    .expectations = {
        { .expectationType = EXPECTATION_TYPE_MESSAGE, .expectationValue.message = "Golem used Sandstorm!" },
        { .expectationType = EXPECTATION_TYPE_MESSAGE, .expectationValue.message = "The opposing Meganium used Absorb!" },
        { .expectationType = EXPECTATION_TYPE_HP_BAR, .expectationValue.hpTaken = { 112, 112, 120, 120, 120, 120, 120, 124, 124, 124, 124, 132, 132, 132, 132, 136 } },
    }
}
END_TEST
