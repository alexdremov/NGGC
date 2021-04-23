//
// Created by Александр Дремов on 15.04.2021.
//

#ifndef NGGC_FASTSTACKCONTROLLER_H
#define NGGC_FASTSTACKCONTROLLER_H

#include <cstdlib>
#include "ByteContainer.h"
#include "ElCommand.h"
#include "RegisterMaster.h"

class FastStackController {
    static constexpr unsigned regs[] = {
            REG_RCX,
            REG_R10,
            REG_R11,
            REG_R12,
            REG_R13,
            REG_R14,
            REG_R15,
    };
    static constexpr unsigned usedRegsN = sizeof(regs) / sizeof(regs[0]);
    size_t regsUsed;
    size_t top;

    void pushRegisterStack(ByteContainer &container, unsigned reg) {
        const movCommand &cmd = MOV_TABLE[regs[regsUsed]][reg];
        container.append(reinterpret_cast<const char *>(cmd.bytecode), 3);
        regsUsed++;
    }

    void popRegisterStack(ByteContainer &container, unsigned reg) {
        const movCommand &cmd = MOV_TABLE[reg][regs[--regsUsed]];
        container.append(reinterpret_cast<const char *>(cmd.bytecode), 3);
    }

public:
    void push(ByteContainer &container, unsigned reg) {
        if (regsUsed >= usedRegsN)
            container.append((char*)PUSH_TABLE[reg], sizeof(PUSH_TABLE[reg]));
        else
            pushRegisterStack(container, reg);
        top++;
    }

    void pop(ByteContainer &container, unsigned reg) {
        if (top > usedRegsN)
            container.append((char*)POP_TABLE[reg], sizeof(PUSH_TABLE[reg]));
        else
            popRegisterStack(container, reg);
        top--;
    }

    void saveStack(ByteContainer &container) const {
        for (unsigned i = 0; i < regsUsed; i++)
            container.append((char*)PUSH_TABLE[regs[i]], sizeof(PUSH_TABLE[regs[i]]));
    }

    void restoreStack(ByteContainer &container) const {
        for (int i = (int) regsUsed - 1; i >= 0; i--)
            container.append((char*)POP_TABLE[regs[i]], sizeof(PUSH_TABLE[regs[i]]));
    }

    void init() {
        top = 0;
        regsUsed = 0;
    }

    void clear() {
        init();
    }

    void dest() {
    }

    static FastStackController *New() {
        auto thou = (FastStackController *) calloc(1, sizeof(FastStackController));
        thou->init();
        return thou;
    }

    static void Delete(FastStackController *thou) {
        thou->dest();
        free(thou);
    }

    size_t getTop() const {
        return top;
    }
};

#endif //NGGC_FASTSTACKCONTROLLER_H
