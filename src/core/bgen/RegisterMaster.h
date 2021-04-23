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
        FastList<RegVarUsage> usages[REGSNUM];
        ByteContainer *compiled;
        size_t stackLastIndex;
        size_t tmpIdNow;

        void deactivateReg(unsigned regNum) {
            size_t activeIter = findActiveUsageOf(regNum);
            if (activeIter == usages[regNum].end())
                return;
            RegVarUsage *activeNow = nullptr;
            usages[regNum].get(activeIter, &activeNow);

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
        }

        void saveRegDataSeg(const char *name, unsigned int num) {
            assert(false);
        }

        void saveRegRbpOffset(size_t offset, unsigned int reg) {
            int32_t displacement = offset;
            const unsigned char *command = REGTOMEMRBP[reg];
            compiled->append((char *) command, sizeof(REGTOMEMRBP[reg]));
            compiled->append((char *) (&displacement), sizeof(int32_t));
        }

        void saveRegStack(size_t index, unsigned int reg) {
            int32_t stackDisplacement = stackLastIndex - index;
            stackDisplacement *= -8;
            const unsigned char *command = REGTOMEMRSP[reg];
            compiled->append((char *) command, sizeof(REGTOMEMRSP[reg]));
            compiled->append((char *) (&stackDisplacement), sizeof(int32_t));
        }

        void restRegStack(size_t index, unsigned int reg) {
            int32_t stackDisplacement = stackLastIndex - index;
            stackDisplacement *= -8;
            const unsigned char *command = MEMRSPTOREG[reg];
            compiled->append((char *) command, sizeof(MEMRSPTOREG[reg]));
            compiled->append((char *) (&stackDisplacement), sizeof(int32_t));
        }

        void pushRegister(unsigned int num) {
            compiled->append(PUSH_TABLE[num]);
        }

        void saveTmp(RegVarUsage *pUsage, unsigned int num) {
            if (pUsage->stackIndex != (size_t) -1) { // already somewhere in stack
                saveRegStack(pUsage->stackIndex, num);
            } else {
                pUsage->stackIndex = (++stackLastIndex);
                pushRegister(num);
            }
        }

        unsigned leastUsedReg() {
            unsigned lowest = RegLists::allRegs[0];
            for (unsigned i = 1; i < RegLists::allRegsN; i++) {
                bool active = false;
                for (size_t iter = usages[RegLists::allRegs[i]].begin(); iter != usages[RegLists::allRegs[i]].end();
                     usages[RegLists::allRegs[i]].nextIterator(&iter)){
                    RegVarUsage* usage = nullptr;
                    usages[RegLists::allRegs[i]].get(iter, &usage);
                    if (usage->active) {
                        active = true;
                        break;
                    }
                }
                if (!active)
                    return RegLists::allRegs[i];
                if (usages[RegLists::allRegs[i]].getSize() < usages[lowest].getSize())
                    lowest = RegLists::allRegs[i];
            }
            return lowest;
        }

        size_t findActiveUsageOf(unsigned reg) {
            for (size_t i = usages[reg].begin(); i != usages[reg].end(); usages[reg].nextIterator(&i)) {
                RegVarUsage *usage = nullptr;
                usages[reg].get(i, &usage);
                if (usage->active)
                    return i;
            }
            return usages[reg].end();
        }

        void arrangeForVar(RegVarUsage *pUsage) {
            unsigned reg = leastUsedReg();
            deactivateReg(reg);
            pUsage->active;
            usages[reg].pushBack(*pUsage);
        }

        void addTmpToReg(size_t &id, unsigned int reg) {
            id = tmpIdNow++;
            deactivateReg(reg);
            RegVarUsage usage = {};
            usage.initTmpReg(id);
            usage.active = true;
            usages[reg].pushBack(usage);
        }

        void loadTmp(RegVarUsage *pUsage, unsigned reg) {
            assert(pUsage->type == TMP_REG);
            restRegStack(pUsage->stackIndex, reg);
            pUsage->active = true;
        }

        unsigned activateTmp(size_t id) {
            RegVarUsage usageFind = {};
            usageFind.initTmpReg(id);

            size_t index = 0;
            unsigned reg = findUsage(index, usageFind);
            deactivateReg(reg);

            RegVarUsage *usage = nullptr;
            usages[reg].get(index, &usage);
            if (usage->active)
                return reg;
            loadTmp(usage, reg);
            return reg;
        }

    public:
        unsigned allocateTmp(size_t &id){
            unsigned reg = leastUsedReg();
            addTmpToReg(id, reg);
            return reg;
        }

        unsigned getTmp(size_t id){
            return activateTmp(id);
        }

        void addRsp(int32_t i) {
            const unsigned char command[] = {ADD_RSPIMM32};
            compiled->append((char *) command, sizeof(command));
            compiled->append((char *) (&i), sizeof(i));
        }

        void subRsp(int32_t i) {
            const unsigned char command[] = {SUB_RSPIMM32};
            compiled->append((char *) command, sizeof(command));
            compiled->append((char *) (&i), sizeof(i));
        }

        unsigned int findUsage(size_t& index, RegVarUsage usage) {
            for (unsigned i = 0; i < RegLists::allRegsN; i++){
                for (index = usages[RegLists::allRegs[i]].begin(); index != usages[RegLists::allRegs[i]].begin(); usages[RegLists::allRegs[i]].nextIterator(&index)){
                    RegVarUsage *usageTest = nullptr;
                    usages[RegLists::allRegs[i]].get(index, &usageTest);
                    if (*usageTest == usage) {
                        return RegLists::allRegs[i];
                    }
                }
            }
            index = 0;
            return 0;
        }

        void releaseTmp(size_t id){
            size_t index = 0;
            RegVarUsage usageFind = {};
            usageFind.initTmpReg(id);
            unsigned reg = findUsage(index, usageFind);
            assert(index != 0);
            RegVarUsage* usage = nullptr;
            usages[reg].get(index, &usage);
            if (usage->stackIndex == stackLastIndex) {
                stackLastIndex--;
                addRsp(8);
            }
            usages[reg].remove(index);
        }

        unsigned getVar(size_t offset, VarType type, const char *name){
            RegVarUsage usageFind = {};
            if (type == Var_Loc)
                usageFind.initLocalVar(offset, name);
            else
                usageFind.initGlobalVar(offset, name);
            size_t index = 0;
            unsigned reg = findUsage(index, usageFind);
            if (index == 0) {
                arrangeForVar(&usageFind);
                reg = findUsage(index, usageFind);
                assert(index != 0);
            }
            return reg;
        }

        unsigned getVarLocal(size_t offset, VarType type, const char *name){
            getVar(offset, Var_Loc, name);
        }

        unsigned getVarGlobal(size_t offset, VarType type, const char *name){
            getVar(offset, Var_Glob, name);
        }

        void releaseSpecificReg(unsigned reg){
            deactivateReg(reg);
        }

        void prepareCall(){
            for(unsigned i = 0; i < RegLists::callNotPreservedN; i++){
                deactivateReg(RegLists::callNotPreserved[i]);
            }
            subRsp((stackLastIndex % 2) * 8); // stack 16 alignment
        }

        void revertCall(){
            addRsp((stackLastIndex % 2) * 8); // stack 16 alignment
        }

        void restorePreserved(){
            for (size_t i = 0; i < RegLists::regsPreserveN; i++)
                activateTmp(i);
        }

        void init(ByteContainer* container){
            compiled = container;
            usages->init();
            stackLastIndex = 0;
            tmpIdNow = 0;
            for (unsigned i = 0; i < RegLists::regsPreserveN; i++){
                size_t index = 0;
                addTmpToReg(index, RegLists::regsPreserve[i]);
                assert(index == i);
            }
        }
    };
}

#endif //CallConvention_GUARD
