.nds
.thumb

.include "armips/include/animscriptcmd.s"

.create "build/move/move_anim/0_751", 0

// No Retreat: reuses the Bulk Up animation (user tint)

a010_751:
    playsepan 2026, -117
    waitse 2029, -117, 20
    callfunction 6, 1, 0, "NaN", "NaN", "NaN", "NaN", "NaN", "NaN", "NaN", "NaN", "NaN"
    waitstate
    shadeattackingmon 31, 4, 4
    waitstate
    end

.close
