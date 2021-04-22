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

struct movCommand {
    unsigned char bytecode[3];
};

{{TABLES}}


#endif //NGGC_ELCOMMAND_H
