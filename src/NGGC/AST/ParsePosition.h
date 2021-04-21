//
// Created by Александр Дремов on 09.12.2020.
//

#ifndef NGG_PARSEPOSITION_H
#define NGG_PARSEPOSITION_H

#include <cstdlib>
#include "LexicalAnalysis/Lexeme.h"

namespace NGGC {
    class ParsePosition {
        FastList<Lexeme> *input;
        size_t it;
    public:
        void init() {
            input = nullptr;
            it = 0;
        }

        void init(FastList<Lexeme> *inputList) {
            input = inputList;
            it = inputList->begin();
        }

        void init(FastList<Lexeme> *inputList, size_t pos) {
            input = inputList;
            it = pos;
        }

        void dest() {}

        static ParsePosition *New() {
            auto *ob = static_cast<ParsePosition *>(calloc(1, sizeof(ParsePosition)));
            ob->init();
            return ob;
        }

        static void Delete(ParsePosition *ob) {
            ob->dest();
            free(ob);
        }

        [[nodiscard]] Lexeme get() const {
            Lexeme tmp {};
            tmp.init();
            input->get(it, &tmp);
            return tmp;
        }

        [[nodiscard]] size_t getPos() const {
            return it;
        }

        void restore(size_t pos) {
            it = pos;
        }

        [[nodiscard]] Lexeme getMove() {
            Lexeme tmp {};
            tmp.init();
            input->get(it, &tmp);
            it = input->nextIterator(it);
            return tmp;
        }

        void operator++() {
            it = input->nextIterator(it);
        }

        void operator--() {
            it = input->prevIterator(it);
        }

        [[nodiscard]] bool isEnded() const{
            return it == 0;
        }
    };
}

#endif //NGG_PARSEPOSITION_H
