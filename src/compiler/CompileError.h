//
// Created by Александр Дремов on 12.12.2020.
//

#ifndef NGG_COMPILEERROR_H
#define NGG_COMPILEERROR_H

#include "lexicalAnalysis/Lexeme.h"

namespace NGGC {

    struct CompileError {
        StrContainer *msg;
        size_t line;
        size_t col;

        void dest() {
            if (msg)
                StrContainer::Delete(msg);
        }

        void init() {
            msg = nullptr;
            line = 0;
            col = 0;
        }

        void init(StrContainer *message, const Lexeme& lex) {
            msg = message;
            line = lex.getLine();
            col = lex.getCol();
        }

        void init(const char *message, const Lexeme& lex) {
            msg = StrContainer::New();
            msg->init(message);
            line = lex.getLine();
            col = lex.getCol();
        }
    };
}

#endif //NGG_COMPILEERROR_H
