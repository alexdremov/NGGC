//
// Created by Александр Дремов on 09.12.2020.
//

#ifndef NGG_LEXEME_H
#define NGG_LEXEME_H

#include <cstdlib>
#include "LexemeType.h"
#include "helpers/StrContainer.h"

namespace NGGC {
    class Lexeme {
        LexemeType type;
        bool stringUsed;
        size_t col;
        size_t line;
        char *codeOffset;
        StrContainer *sContent;
        double vContent;

    public:
        void init() {
            type = Lex_None;
            col = 0;
            line = 0;
            stringUsed = false;
            vContent = 0;
            sContent = nullptr;
        }

        void init(LexemeType lexemeType, StrContainer *content, char *offset = nullptr) {
            type = lexemeType;
            sContent = content;
            stringUsed = true;
            codeOffset = offset;
        }

        void init(LexemeType lexemeType, char *offset = nullptr) {
            type = lexemeType;
            stringUsed = false;
            codeOffset = offset;
        }

        void init(LexemeType lexemeType, double content, char *offset = nullptr) {
            type = lexemeType;
            vContent = content;
            stringUsed = false;
            codeOffset = offset;
        }

        void dest() {
            if (sContent) {
                StrContainer::Delete(sContent);
                sContent = nullptr;
            }
        }

        static Lexeme *New() {
            auto *ob = static_cast<Lexeme *>(calloc(1, sizeof(Lexeme)));
            ob->init();
            return ob;
        }

        static void Delete(Lexeme *ob) {
            ob->dest();
            free(ob);
        }

        [[nodiscard]] char *getCodeOffset() const {
            return codeOffset;
        }

        [[nodiscard]] size_t getLine() const {
            return line;
        }

        [[nodiscard]] size_t getCol() const {
            return col;
        }

        void setLineCol(size_t lexLine, size_t lexCol) {
            line = lexLine;
            col = lexCol;
        }

        [[nodiscard]] LexemeType getType() const {
            return type;
        }

        [[nodiscard]] double getDouble() const {
            return vContent;
        }

        [[nodiscard]] int getInt() const {
            return (int) vContent;
        }

        [[nodiscard]] StrContainer *getString() const {
            return sContent;
        }

        [[nodiscard]] bool isStringUsed() const {
            return stringUsed;
        }
    };
}

#endif //NGG_LEXEME_H
