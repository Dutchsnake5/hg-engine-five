.nds
.thumb

.include "armips/include/animscriptcmd.s"

.create "build/move/move_anim/0_924", 0

a010_924:
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
    loadparticle 0, 545 // Will-O-Wisp's particles (279) recoloured light blue (rawdata/move_spa/9_545: scripts/spa_tool.py recolor 279 16,26,31 0.8)
    loadparticle 1, 39  // Ice Punch's particles, for the ice crystals on the target
    waitstate
    unloadspriteresource
    resetsprite 0
    resetsprite 1
    resetsprite 2
    resetsprite 3
    playsepanmod 1937, -117, 117, 4, 2
    repeatse 2052, -117, 6, 4 // a soft icy chime as the wisps float over
    addparticle 0, 0, 17
    cmd37 6, 0, 2, 6, 1, 16, 0, "NaN", "NaN"
    cmd37 5, 3, 0, 0, 0, 0, "NaN", "NaN", "NaN"
    wait 30
    // the wisp hits: ice crystals on the target instead of Will-O-Wisp's flame burst
    addparticle 1, 0, 4 // the burst of ice crystals
    addparticle 1, 1, 4 // the icy puff that follows (Ice Punch's emitter 2, the fist, is left out)
    // freezing sounds instead of Ice Punch's punch impact (2116): Ice Punch's freeze, Ice Shard's crystals, the icy chime
    playsepan 2055, 117
    repeatse 1827, 117, 2, 3
    waitse 2052, 117, 7
    callfunction 36, 5, 2, 0, 1, 2, 264, "NaN", "NaN", "NaN", "NaN", "NaN"
    callfunction 34, 6, 8, 0, 2, 32631, 14, 0, "NaN", "NaN", "NaN", "NaN"
    waitparticle
    unloadparticle 0
    unloadparticle 1
    end

.close
