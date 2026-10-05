.nds
.thumb

.include "armips/include/animscriptcmd.s"

.create "build/move/move_anim/0_549", 0

// Techno Blast: reuses the Signal Beam animation (screen flash)

a010_549:
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
    loadparticle 0, 342
    waitstate
    unloadspriteresource
    resetsprite 0
    resetsprite 1
    resetsprite 2
    resetsprite 3
    jumpifplayerattack _s0_01AC
    jumpifcontest _s0_0288
    addsequentialparticle 0, 0, 1, 2, 3, 4, 5, 0x12
    callfunction 36, 5, 1, 0, 0, 15, 258, "NaN", "NaN", "NaN", "NaN", "NaN"
    callfunction 36, 5, 1, 0, 0, 15, 264, "NaN", "NaN", "NaN", "NaN", "NaN"
    repeatse 1530, 0, 6, 2
    loop 3
    callfunction 34, 5, 8, 0, 1, 31, 8, "NaN", "NaN", "NaN", "NaN", "NaN"
    wait 8
    callfunction 34, 5, 8, 0, 1, 11252, 8, "NaN", "NaN", "NaN", "NaN", "NaN"
    wait 8
    doloop
    waitparticle
    unloadparticle 0
    flashscreencolor 31, 31, 31
    waitstate
    end
_s0_01AC:
    addparticlebasedonbattler 0, 7, 8, 7, 8, 0x3
    callfunction 36, 5, 1, 0, 0, 15, 258, "NaN", "NaN", "NaN", "NaN", "NaN"
    callfunction 36, 5, 1, 0, 0, 15, 264, "NaN", "NaN", "NaN", "NaN", "NaN"
    repeatse 1530, 0, 6, 2
    loop 3
    callfunction 34, 5, 8, 0, 1, 31, 8, "NaN", "NaN", "NaN", "NaN", "NaN"
    wait 8
    callfunction 34, 5, 8, 0, 1, 11252, 8, "NaN", "NaN", "NaN", "NaN", "NaN"
    wait 8
    doloop
    waitparticle
    unloadparticle 0
    flashscreencolor 31, 31, 31
    waitstate
    end
_s0_0288:
    addparticle 0, 6, 3
    callfunction 36, 5, 1, 0, 0, 15, 258, "NaN", "NaN", "NaN", "NaN", "NaN"
    callfunction 36, 5, 1, 0, 0, 15, 264, "NaN", "NaN", "NaN", "NaN", "NaN"
    repeatse 1530, 0, 6, 2
    loop 3
    callfunction 34, 5, 8, 0, 1, 31, 8, "NaN", "NaN", "NaN", "NaN", "NaN"
    wait 8
    callfunction 34, 5, 8, 0, 1, 11252, 8, "NaN", "NaN", "NaN", "NaN", "NaN"
    wait 8
    doloop
    waitparticle
    unloadparticle 0
    flashscreencolor 31, 31, 31
    waitstate
    end

.close
