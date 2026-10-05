.nds
.thumb

.include "armips/include/animscriptcmd.s"

.create "build/move/move_anim/0_677", 0

// Gear Up: reuses the Shift Gear animation (user tint)

a010_677:
    loadparticlefromspa 0, 520
    waitparticle

    addparticle 0, 0, 3
    addparticle 0, 1, 3
    addparticle 0, 2, 3
    addparticle 0, 3, 3
    addparticle 0, 4, 3
    addparticle 0, 5, 3

    playsepan 2038, 117 // metal claw sound

    wait 30
    shadeattackingmon 22, 22, 26
    callfunction 36, 5, 4, 0, 1, 14, 264, "NaN", "NaN", "NaN", "NaN", "NaN"
    waitse 1850, 117, 3
    addparticle 0, 6, 3
    addparticle 0, 7, 3
    waitparticle

    unloadparticle 0
    waitstate

    waitstate
    end

.close
