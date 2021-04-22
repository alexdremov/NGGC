//
// Created by Александр Дремов on 14.04.2021.
//

#ifndef NGGC_ELCOMMAND_H
#define NGGC_ELCOMMAND_H

// int 3
#define BREAKPOINT 0xCD, 0x03
#define COMMANDEND 0x100
// syscall
#define SYSCALL    0x0F, 0x05
#define REGSNUM {{REGNUM}}

{{DEFINES}}

#define PUSH_IMM8 0x6A
#define PUSH_IMM16 0x68

#define CALL_REL 0xE8
#define RET 0xC3

#define JE_REL8 0x74
#define JNE_REL8 0x75
#define JL_REL8 0x7C
#define JLE_REL8 0x7E
#define JG_REL8 0x7F
#define JGE_REL8 0x7D
#define JMP_REL8 0xEB

#define JE_REL32 0x0F, 0x84
#define JNE_REL32 0x0F, 0x85
#define JL_REL32 0x0F, 0x8C
#define JLE_REL32 0x0F, 0x8E
#define JG_REL32 0x0F, 0x8F
#define JGE_REL32 0x0F, 0x8D
#define JMP_REL32 0xE9


#define XCHG_RAXRBX 0x48, 0x93

struct movCommand {
    unsigned char bytecode[3];
};

{{TABLES}}


#endif //NGGC_ELCOMMAND_H
