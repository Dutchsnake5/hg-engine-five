// Test: Cud Chew - does not eat a berry stolen with Bug Bite again
#include "../../battle_tests.h"
BEGIN_TEST
{
    .battleType = BATTLE_TYPE_TRAINER,
    .weather = FIELD_CONDITION_NONE,
    .fieldCondition = 0,
    .terrain = TERRAIN_NONE,
    .playerParty = {
        {
            .species = SPECIES_TAUROS,
            .level = 50,
            .form = 0,
            .ability = ABILITY_CUD_CHEW,
            .item = ITEM_NONE,
            .moves = { MOVE_BUG_BITE, MOVE_NONE, MOVE_NONE, MOVE_NONE },
            .hp = FULL_HP,
            .status = 0,
            .condition2 = 0,
            .moveEffectFlags = 0,
        },
        { .species = SPECIES_NONE },
        { .species = SPECIES_NONE },
        { .species = SPECIES_NONE },
        { .species = SPECIES_NONE },
        { .species = SPECIES_NONE },
    },
    .enemyParty = {
        {
            .species = SPECIES_TOXICROAK,
            .level = 50,
            .form = 0,
            .ability = ABILITY_DRY_SKIN,
            .item = ITEM_LUM_BERRY,
            .moves = { MOVE_TOXIC, MOVE_NONE, MOVE_NONE, MOVE_NONE },
            .hp = FULL_HP,
            .status = 0,
            .condition2 = 0,
            .moveEffectFlags = 0,
        },
        { .species = SPECIES_NONE },
        { .species = SPECIES_NONE },
        { .species = SPECIES_NONE },
        { .species = SPECIES_NONE },
        { .species = SPECIES_NONE },
    },
    .playerScript = {
        {
            { ACTION_MOVE_SLOT_1, BATTLER_ENEMY_FIRST },
            { ACTION_MOVE_SLOT_1, BATTLER_ENEMY_FIRST },
            { ACTION_MOVE_SLOT_1, BATTLER_ENEMY_FIRST },
            { ACTION_NONE, 0 },
            { ACTION_NONE, 0 },
            { ACTION_NONE, 0 },
            { ACTION_NONE, 0 },
            { ACTION_NONE, 0 },
        },
    },
    .enemyScript = {
        {
            { ACTION_MOVE_SLOT_1, BATTLER_PLAYER_FIRST },
            { ACTION_MOVE_SLOT_1, BATTLER_PLAYER_FIRST },
            { ACTION_MOVE_SLOT_1, BATTLER_PLAYER_FIRST },
            { ACTION_NONE, 0 },
            { ACTION_NONE, 0 },
            { ACTION_NONE, 0 },
            { ACTION_NONE, 0 },
            { ACTION_NONE, 0 },
        },
    },
    .expectations = {
        { .expectationType = EXPECTATION_TYPE_MESSAGE, .expectationValue.message = "Tauros stole and ate its target's Lum Berry!" },
        { .expectationType = EXPECTATION_TYPE_MESSAGE, .expectationValue.message = "Tauros was badly poisoned!" },
        { .expectationType = EXPECTATION_TYPE_MESSAGE, .expectationValue.message = "Tauros was hurt by its poisoning!" },
        { .expectationType = EXPECTATION_TYPE_MESSAGE, .expectationValue.message = "Tauros was hurt by its poisoning!" },
        { .expectationType = EXPECTATION_TYPE_MESSAGE, .expectationValue.message = "Tauros was hurt by its poisoning!" },
    },
}
END_TEST
