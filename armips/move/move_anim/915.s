.nds
.thumb

.include "armips/include/animscriptcmd.s"

.create "build/move/move_anim/0_915", 0

// Hard Press: reuses the Crush Grip animation (target tint)

a010_915:
    initspriteresource
    loadspriteresource 0
    loadspriteresource 1
    loadspriteresource 2
    loadspriteresource 3
    loadspritemaybe 4, 0, 0, 0
    loadspritemaybe 5, 0, 1, 1
    loadspritemaybe 6, 0, 2, 2
    loadspritemaybe 7, 0, 3, 3
    callfunction 78, 1, 0, "NaN", "NaN", "NaN", "NaN", "NaN", "NaN", "NaN", "NaN", "NaN"
    loadparticle 0, 480
    waitstate
    unloadspriteresource
    resetsprite 0
    resetsprite 1
    resetsprite 2
    resetsprite 3
    playsepanmod 2103, -117, 117, 4, 2
    jumpifcontest _s0_00EC
    jumpifside 0, _s0_01E4, _s0_02DC
    end
_s0_00EC:
    addparticle 0, 1, 3
    callfunction 65, 6, 0, 0, 0, 0, 10, 64, "NaN", "NaN", "NaN", "NaN"
    wait 5
    wait 6
    playsepan 2014, 117
    shadetargetmon 22, 22, 26
    callfunction 42, 8, 264, 100, 100, 100, 70, 100, 1310721, 262148, "NaN", "NaN"
    wait 5
    addparticle 0, 3, 4
    addparticle 0, 3, 4
    addparticle 0, 2, 4
    waitse 1827, 117, 7
    waitse 1827, 117, 8
    waitse 1827, 117, 10
    waitstate
    waitparticle
    unloadparticle 0
    waitstate
    end
_s0_01E4:
    addparticle 0, 0, 3
    callfunction 65, 6, 0, 0, 0, 0, 10, 64, "NaN", "NaN", "NaN", "NaN"
    wait 5
    wait 6
    playsepan 2014, 117
    shadetargetmon 22, 22, 26
    callfunction 42, 8, 264, 100, 100, 100, 70, 100, 1310721, 262148, "NaN", "NaN"
    wait 5
    addparticle 0, 3, 4
    addparticle 0, 3, 4
    addparticle 0, 2, 4
    waitse 1827, 117, 7
    waitse 1827, 117, 8
    waitse 1827, 117, 10
    waitstate
    waitparticle
    unloadparticle 0
    waitstate
    end
_s0_02DC:
    addparticle 0, 1, 3
    callfunction 65, 6, 0, 0, 0, 0, 10, 64, "NaN", "NaN", "NaN", "NaN"
    wait 5
    wait 6
    playsepan 2014, 117
    shadetargetmon 22, 22, 26
    callfunction 42, 8, 264, 100, 100, 100, 70, 100, 1310721, 262148, "NaN", "NaN"
    wait 5
    addparticle 0, 3, 4
    addparticle 0, 3, 4
    addparticle 0, 2, 4
    waitse 1827, 117, 7
    waitse 1827, 117, 8
    waitse 1827, 117, 10
    waitstate
    waitparticle
    unloadparticle 0
    waitstate
    end

.close
