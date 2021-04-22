//
// Created by Александр Дремов on 10.12.2020.
//

#ifndef NGG_ASTERROR_H
#define NGG_ASTERROR_H
#include <cstdlib>
#include "lexicalAnalysis/Lexeme.h"

namespace NGGC {
    struct ASTError {
        Lexeme      errorIt;
        const char *errorMsg;

        void dest(){

        }
    };
}

#endif //NGG_ASTERROR_H
