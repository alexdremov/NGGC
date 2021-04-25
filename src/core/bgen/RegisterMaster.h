//
// Created by Александр Дремов on 23.04.2021.
//

#ifndef CallConvention_GUARD
#define CallConvention_GUARD

#include <MachOBuilder.h>
#include <FastList.h>
#include "ByteContainer.h"
#include "ElCommand.h"
#include "RegList.h"
#include "RegVarUsage.h"
#include "../../compiler/VarTable.h"

namespace NGGC {
    class RegisterMaster {
        struct RegisterState {
            FastList<RegVarUsage> usages[REGSNUM];
            ByteContainer *compiled;
            size_t stackLastIndex;

            static unsigned int isRegCallPreserved(unsigned int reg) {
                for (unsigned regPreserved: RegLists::regsPreserve)
                    if (reg == regPreserved)
                        return true;
                return false;
            }

            unsigned leastUsedReg(unsigned anyButNot = -1) {
                unsigned lowest = RegLists::allRegs[anyButNot == 0 ? 1 : 0];
                unsigned lowestSize = usages[lowest].getSize() + 1;
                for (unsigned int currentReg : RegLists::allRegs) {
                    if (currentReg == anyButNot)
                        continue;
                    bool callPreserved = isRegCallPreserved(currentReg);
                    int realUse = int(usages[currentReg].getSize()) + (callPreserved ? -1 : 0);
                    if (realUse < int(lowestSize) || (realUse <= 0 && callPreserved)) {
                        lowest = currentReg;
                        lowestSize = realUse;
                    }
                }
                return lowest;
            }

            size_t findActiveUsageOf(unsigned reg, RegVarUsage **usageFound = nullptr) {
                for (size_t i = usages[reg].begin();
                     i != usages[reg].end(); usages[reg].nextIterator(&i)) {
                    RegVarUsage *usage = nullptr;
                    usages[reg].get(i, &usage);
                    if (usage->active) {
                        if (usageFound)
                            *usageFound = usage;
                        return i;
                    }
                }
                if (usageFound)
                    *usageFound = nullptr;
                return usages[reg].end();
            }

            void deactivateReg(unsigned regNum, bool restore = true) {
                size_t activeIter = findActiveUsageOf(regNum);
                if (activeIter == usages[regNum].end())
                    return;
                RegVarUsage *activeNow = nullptr;
                usages[regNum].get(activeIter, &activeNow);

                if (restore) {
                    switch (activeNow->type) {
                        case LOC_REG: {
                            saveRegRbpOffset(activeNow->offset, regNum);
                            usages[regNum].remove(activeIter);
                            break;
                        }
                        case GLO_REG: {
                            saveRegDataSeg(activeNow->name, regNum);
                            usages[regNum].remove(activeIter);
                            break;
                        }
                        case TMP_REG: {
                            saveTmp(activeNow, regNum);
                            activeNow->active = false;
                            break;
                        }
                    }
                } else {
                    if (activeNow->type == TMP_REG)
                        activeNow->active = false;
                }
            }

            void saveRegDataSeg(const char *name, unsigned int num) {
                assert(false);
            }

            void saveRegRbpOffset(int32_t offset, unsigned int reg) const {
                int32_t displacement = offset * -8;
                const unsigned char *command = REGTOMEMRBP[reg];
                compiled->append((char *) command, sizeof(REGTOMEMRBP[reg]));
                compiled->append((char *) (&displacement), sizeof(int32_t));
            }

            void restRegRbpOffset(int32_t offset, unsigned int reg) const {
                int32_t displacement = offset * -8;
                const unsigned char *command = MEMRBPTOREG[reg];
                compiled->append((char *) command, sizeof(MEMRBPTOREG[reg]));
                compiled->append((char *) (&displacement), sizeof(int32_t));
            }

            void saveRegStack(size_t index, unsigned int reg) const {
                int32_t stackDisplacement = stackLastIndex - index;
                stackDisplacement *= 8;
                const unsigned char *command = REGTOMEMRSP[reg];
                compiled->append((char *) command, sizeof(REGTOMEMRSP[reg]));
                compiled->append((char *) (&stackDisplacement), sizeof(int32_t));
            }

            void restRegStack(size_t index, unsigned int reg) const {
                int32_t stackDisplacement = stackLastIndex - index;
                stackDisplacement *= 8;
                const unsigned char *command = MEMRSPTOREG[reg];
                compiled->append((char *) command, sizeof(MEMRSPTOREG[reg]));
                compiled->append((char *) (&stackDisplacement), sizeof(int32_t));
            }

            void pushRegister(unsigned int num) const {
                compiled->append((char *) PUSH_TABLE[num], sizeof(PUSH_TABLE[num]));
            }

            void saveTmp(RegVarUsage *pUsage, unsigned int num) {
                if (pUsage->stackIndex != (size_t) -1) { // already somewhere in stack
                    saveRegStack(pUsage->stackIndex, num);
                } else {
                    pUsage->stackIndex = (++stackLastIndex);
                    pushRegister(num);
                }
            }

            void arrangeForVar(RegVarUsage *pUsage, bool loadValue = true, unsigned anyButNo = -1) {
                unsigned reg = leastUsedReg(anyButNo);
                deactivateReg(reg);
                pUsage->active = true;
                if (loadValue)
                    restRegRbpOffset(pUsage->offset, reg);
                usages[reg].pushBack(*pUsage);
            }

            void addTmpToReg(size_t id, unsigned int reg) {
                deactivateReg(reg);
                RegVarUsage usage = {};
                usage.initTmpReg(id);
                usage.active = true;
                usages[reg].pushBack(usage);
            }

            void loadTmp(RegVarUsage *pUsage, unsigned reg) const {
                assert(pUsage->type == TMP_REG);
                if (pUsage->stackIndex != (size_t) -1)
                    restRegStack(pUsage->stackIndex, reg);
                pUsage->active = true;
            }

            unsigned activateTmp(size_t id, bool restore = true) {
                RegVarUsage usageFind = {};
                usageFind.initTmpReg(id);

                RegVarUsage *usage = nullptr;
                size_t index = 0;
                unsigned reg = findUsage(index, usageFind);
                usages[reg].get(index, &usage);
                if (usage->active)
                    return reg;
                deactivateReg(reg, restore);


                usages[reg].get(index, &usage);
                if (usage->active)
                    return reg;
                loadTmp(usage, reg);
                return reg;
            }

            unsigned int findUsage(size_t &index, RegVarUsage usage) {
                for (unsigned int allReg : RegLists::allRegs) {
                    for (index = usages[allReg].begin();
                         index != usages[allReg].end(); usages[allReg].nextIterator(&index)) {
                        RegVarUsage *usageTest = nullptr;
                        usages[allReg].get(index, &usageTest);
                        if (*usageTest == usage) {
                            return allReg;
                        }
                    }
                }
                index = 0;
                return 0;
            }

            void dest() {
                for (auto &usage : usages)
                    usage.dest();
            }

            void init(ByteContainer *file) {
                compiled = file;
                for (auto &usage : usages)
                    usage.init();
            }
        };

        RegisterState registerState;
        ByteContainer *compiledBin;
        size_t tmpIdNow;

        void setup() {
            registerState.stackLastIndex = 0;
            tmpIdNow = 0;
            for (unsigned i = 0; i < RegLists::regsPreserveN; i++) {
                size_t index = tmpIdNow++;
                registerState.addTmpToReg(index, RegLists::regsPreserve[i]);
                assert(index == i);
            }
        }

    public:

        RegisterState getState() {
            RegisterState state = {};
            state.stackLastIndex = registerState.stackLastIndex;
            state.compiled = compiledBin;
            for (unsigned i = 0; i < REGSNUM; i++)
                state.usages[i] = registerState.usages[i].copy();
            return state;
        }

        void restoreState(RegisterState &state) {
            RegisterState &old = registerState;
            for (unsigned reg = 0; reg < REGSNUM; reg++) {
                RegVarUsage *usageOld = nullptr;
                old.findActiveUsageOf(reg, &usageOld);
                RegVarUsage *usageNew = nullptr;
                state.findActiveUsageOf(reg, &usageNew);

                if ((usageNew == nullptr && usageOld == nullptr) ||
                    (usageNew != nullptr && usageOld != nullptr && *usageNew == *usageOld))
                    continue;

                if (usageNew != nullptr && usageOld != nullptr) {
                    old.deactivateReg(reg);
                    usageOld = nullptr;
                }

                if (usageNew == nullptr && usageOld != nullptr) {
                    old.deactivateReg(reg);
                } else { // usageOld == nullptr && usageNew != nullptr
                    switch (usageNew->type) {
                        case LOC_REG:
                        case GLO_REG: {
                            old.restRegRbpOffset(usageNew->offset, reg);
                            old.usages[reg].pushBack(*usageNew);
                            break;
                        }
                        case TMP_REG: {
                            old.activateTmp(usageNew->id);
                            break;
                        }
                    }
                }
            }
            if (old.stackLastIndex != state.stackLastIndex) {
                if (old.stackLastIndex > state.stackLastIndex) {
                    addRsp((old.stackLastIndex - state.stackLastIndex) * 8);
                } else {
                    subRsp((state.stackLastIndex - old.stackLastIndex) * 8);
                }
            }
        }

        unsigned allocateTmp(size_t &id, unsigned anyButNot) {
            unsigned reg = registerState.leastUsedReg(anyButNot);
            id = tmpIdNow++;
            registerState.addTmpToReg(id, reg);
            return reg;
        }

        unsigned allocateTmp(size_t &id) {
            unsigned reg = registerState.leastUsedReg();
            id = tmpIdNow++;
            registerState.addTmpToReg(id, reg);
            return reg;
        }

        unsigned getTmp(size_t id) {
            return registerState.activateTmp(id);
        }

        void addRsp(int32_t i) {
            const unsigned char command[] = {ADD_RSPIMM32};
            compiledBin->append((char *) command, sizeof(command));
            compiledBin->append((char *) (&i), sizeof(i));
        }

        void subRsp(int32_t i) {
            const unsigned char command[] = {SUB_RSPIMM32};
            compiledBin->append((char *) command, sizeof(command));
            compiledBin->append((char *) (&i), sizeof(i));
        }

        void releaseTmp(size_t id) {
            size_t index = 0;
            RegVarUsage usageFind = {};
            usageFind.initTmpReg(id);
            unsigned reg = registerState.findUsage(index, usageFind);
            assert(index != 0);
            RegVarUsage *usage = nullptr;
            registerState.usages[reg].get(index, &usage);
            if (usage->stackIndex == registerState.stackLastIndex) {
                registerState.stackLastIndex--;
                addRsp(8);
            }
            registerState.usages[reg].remove(index);
        }

        // offset in bytes
        unsigned getVar(int32_t offset, VarType type, const char *name, bool loadValue = true, unsigned anyButNo = -1) {
            RegVarUsage usageFind = {};
            if (type == Var_Loc)
                usageFind.initLocalVar(offset, name);
            else
                usageFind.initGlobalVar(offset, name);
            size_t index = 0;
            unsigned reg = registerState.findUsage(index, usageFind);

            if (index == 0) {
                registerState.arrangeForVar(&usageFind, loadValue, anyButNo);
                reg = registerState.findUsage(index, usageFind);
                assert(index != 0);
            }
            return reg;
        }

        unsigned getVarLocal(int32_t offset, VarType type, const char *name) {
            getVar(offset, Var_Loc, name);
        }

        unsigned getVarGlobal(int32_t offset, VarType type, const char *name) {
            getVar(offset, Var_Glob, name);
        }

        void releaseSpecificReg(unsigned reg) {
            registerState.deactivateReg(reg);
        }

        void alignRsp() {
            if ((registerState.stackLastIndex % 2) != 0)
                subRsp((registerState.stackLastIndex % 2) * 8); // stack 16 alignment
        }

        void moveRspBackAfterAlign() {
            if ((registerState.stackLastIndex % 2) != 0)
                addRsp((registerState.stackLastIndex % 2) * 8); // stack 16 alignment
        }

        void restorePreserved() {
            for (size_t i = 0; i < RegLists::regsPreserveN; i++)
                registerState.activateTmp(i, false);
        }

        void alignStackBeforeCall(unsigned argsNumber) {
            if (argsNumber <= RegLists::callRegsN)
                return;
            unsigned shift = (argsNumber - RegLists::callRegsN) * 8;
            subRsp(shift % 2);
        }

        void prepareCallArgumentFromRAX(FastList<size_t> &ids, unsigned number) {
            if (number <= RegLists::callRegsN) {
                size_t idNew = tmpIdNow++;
                ids.pushBack(idNew);
                registerState.addTmpToReg(idNew, RegLists::callRegs[number]);
                compiledBin->append((char *) MOV_TABLE[RegLists::callRegs[number]][REG_RAX].bytecode,
                                    sizeof(MOV_TABLE[RegLists::callRegs[number]][REG_RAX].bytecode));
            } else
                compiledBin->append((char *) PUSH_TABLE[REG_RAX], sizeof(PUSH_TABLE[REG_RAX]));
        }

        void defineArguments(unsigned number) {
            // TODO: define local arguments
        }

        void clearCallStack(unsigned argsNumber) {
            if (argsNumber <= RegLists::callRegsN)
                return;
            unsigned shift = (argsNumber - RegLists::callRegsN) * 8;
            addRsp(shift);
        }

        void init(ByteContainer *container) {
            compiledBin = container;
            registerState.init(container);
            setup();
        }

        void deactivateCallNotPreserved() {
            for (unsigned reg: RegLists::callNotPreserved)
                registerState.deactivateReg(reg);
        }

        void dest() {
            registerState.dest();
        }

        void clear() {
            for (auto &usage : registerState.usages)
                usage.clear();
            setup();
        }

        [[nodiscard]] size_t stackUsed() const {
            return registerState.stackLastIndex;
        }

        void deactivateCallRegisters() {
            for (unsigned reg: RegLists::callRegs)
                registerState.deactivateReg(reg);
        }
    };
}

#endif //CallConvention_GUARD
