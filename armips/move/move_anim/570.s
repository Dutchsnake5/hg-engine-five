.nds
.thumb

.include "armips/include/animscriptcmd.s"

.create "build/move/move_anim/0_570", 0

// Trick Or Treat: reuses the Nightmare animation (target tint)

a010_570:
    initspriteresource
    loadspriteresource 0
    loadspritemaybe 1, 0, 0, 0
    changebg 24, 0x1
    waitforchangebg2
    callfunction 76, 1, 130, "NaN", "NaN", "NaN", "NaN", "NaN", "NaN", "NaN", "NaN", "NaN"
    waitforchangebg
    wait 45
    callfunction 26, 1, 0, "NaN", "NaN", "NaN", "NaN", "NaN", "NaN", "NaN", "NaN", "NaN"
    repeatse 2009, 117, 2, 4
    wait 15
    shadetargetmon 31, 16, 0
    callfunction 36, 5, 2, 0, 1, 6, 264, "NaN", "NaN", "NaN", "NaN", "NaN"
    wait 15
    enablemonsprite 0, 0x0
    wait 30
    resetbg 24, 0x1
    waitstate
    waitforchangebg
    resetsprite 0
    unloadspriteresource
    waitstate
    end

.close
