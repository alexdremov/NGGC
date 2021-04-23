//
// Created by Александр Дремов on 23.04.2021.
//

#ifndef NGGC_REGVARUSAGE_H
#define NGGC_REGVARUSAGE_H

#include <cassert>

namespace NGGC {
    enum RegisterUsageType {
        LOC_REG,
        GLO_REG,
        TMP_REG
    };

    struct RegVarUsage {
        RegisterUsageType type;
        size_t offset;
        bool active;
        size_t stackIndex;
        size_t id;
        const char *name;

        void initGlobalVar(size_t varOffset, const char *varName) {
            name = varName;
            offset = varOffset;
            type = GLO_REG;
            active = false;
            stackIndex = 0;
            id = 0;
        }

        void initLocalVar(size_t varOffset, const char *varName) {
            name = varName;
            offset = varOffset;
            type = LOC_REG;
            active = false;
            stackIndex = 0;
            id = 0;
        }

        void initTmpReg(size_t tmpId) {
            name = nullptr;
            offset = 0;
            type = TMP_REG;
            active = false;
            stackIndex = -1;
            id = tmpId;
        }

        bool operator==(const RegVarUsage& other) const{
            if (other.type != type)
                return false;
            switch (type) {
                case LOC_REG:
                case GLO_REG:
                    return other.offset == offset;
                case TMP_REG:
                    return other.id == id;
            }
            assert(false);
        }
    };
}
#endif //NGGC_REGVARUSAGE_H
