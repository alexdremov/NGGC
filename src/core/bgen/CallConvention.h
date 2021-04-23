//
// Created by Александр Дремов on 23.04.2021.
//

#ifndef CallConvention_GUARD
#define CallConvention_GUARD

#include <MachOBuilder.h>
#include <FastList.h>
#include "ByteContainer.h"
#include "ElCommand.h"
#include "../../compiler/VarTable.h"

namespace NGGC {
    struct RegisterVariable {
        const char* id;
        unsigned scopeLevel;
        unsigned reg;
        size_t rbpOffset;

        bool operator==(const RegisterVariable& other) const{
            return (scopeLevel == other.scopeLevel && strcmp(id, other.id) == 0);
        }
    };

    struct RegsMapper {
        FastList<RegisterVariable> map;

        void init() {
            map.init();
        }

        void dest() {
            map.dest();
        }

        void set(const char *id, unsigned scope, size_t rbpOffset, unsigned reg) {
            RegisterVariable needed = {id, scope, reg, rbpOffset};
            RegisterVariable* found = nullptr;
            for(auto i = map.begin(); i != map.end(); map.nextIterator(&i)){
                RegisterVariable* tmp = nullptr;
                map.get(i, &tmp);
                if (*tmp == needed) {
                    found = tmp;
                    break;
                }
            }
            if (found != nullptr) {
                found->rbpOffset = rbpOffset;
                return;
            }
            map.pushBack(needed);
            return;
        }

        unsigned get(const char *id, unsigned scope) {
            RegisterVariable needed = {id, scope, 0, 0};
            RegisterVariable* found = nullptr;
            for(auto i = map.begin(); i != map.end(); map.nextIterator(&i)){
                RegisterVariable* tmp = nullptr;
                map.get(i, &tmp);
                if (*tmp == needed) {
                    found = tmp;
                    break;
                }
            }
            return found == nullptr ? -1 : found->reg;
        }

        void clear() {
            map.clear();
        }
    };

    struct RegsMaster {
        RegsMapper mapper;
        static constexpr unsigned allRegs[] = {
                REG_RCX,
                REG_RSI,
                REG_RDI,
                REG_R8,
                REG_R9,
                REG_R10,
                REG_R11,
                REG_R12,
                REG_R13,
                REG_R14,
                REG_R15,
                REG_RBX,
                REG_RDX,
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
        static constexpr unsigned useOrder[] = {
                REG_RCX,
                REG_RSI,
                REG_RDI,
                REG_R8,
                REG_R9,
                REG_R10,
                REG_R11,
                REG_R12,
                REG_R13,
                REG_R14,
                REG_R15,
                REG_RBX,
                REG_RDX,
        };

        static constexpr unsigned allRegsN = sizeof(allRegs) / sizeof(allRegs[0]);
        static constexpr unsigned callRegsN = sizeof(callRegs) / sizeof(callRegs[0]);
        static constexpr unsigned preserveRegsN = sizeof(regsPreserve) / sizeof(regsPreserve[0]);

        bool usedRegisters[allRegsN];

        unsigned variableGet(ByteContainer* container, const char* id, unsigned scope) {
            mapper.get(id, scope);
        }

        unsigned getNewRegister(ByteContainer* container, const char* id, unsigned scope) {
            mapper.get(id, scope);
        }

        void init() {
            for (unsigned i = 0; i < allRegsN; i++)
                usedRegisters[i] = false;
            mapper.init();
        }

        void dest() {
            mapper.dest();
        }

        static RegsMaster *New() {
            auto *thou = static_cast<RegsMaster *>(calloc(1, sizeof(RegsMaster)));
            thou->init();
            return thou;
        }

        void Delete() {
            dest();
            free(this);
        }
    };
}

#endif //CallConvention_GUARD
