//
// Created by Александр Дремов on 23.04.2021.
//

#ifndef NGGC_REGLIST_H
#define NGGC_REGLIST_H

namespace NGGC {
    struct RegLists {
        static constexpr unsigned allRegs[] = {
                REG_RBX,
                REG_R12,
                REG_R13,
                REG_R14,
                REG_R15,
                REG_R11,
                REG_RCX,
                REG_R10,
                REG_R9,
                REG_R8,
                REG_RDX,
                REG_RSI,
                REG_RDI,
        };

        static constexpr unsigned callRegs[] = {
                REG_RDI,
                REG_RSI,
                REG_RDX,
                REG_RCX,
                REG_R8,
                REG_R9,
        };

        static constexpr unsigned regsPreserve[] = {
                REG_RBX,
                REG_R12,
                REG_R13,
                REG_R14,
                REG_R15,
        };

        static constexpr unsigned callNotPreserved[] = {
                REG_RCX,
                REG_R11,
                REG_R10,
                REG_R9,
                REG_R8,
                REG_RDX,
                REG_RSI,
                REG_RDI,
        };

        static constexpr unsigned allRegsN = sizeof(allRegs) / sizeof(allRegs[0]);
        static constexpr unsigned callRegsN = sizeof(callRegs) / sizeof(callRegs[0]);
        static constexpr unsigned regsPreserveN = sizeof(regsPreserve) / sizeof(regsPreserve[0]);
        static constexpr unsigned callNotPreservedN = sizeof(callNotPreserved) / sizeof(callNotPreserved[0]);
    };
}

#endif //NGGC_REGLIST_H
