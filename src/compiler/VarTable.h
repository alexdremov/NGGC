//
// Created by Александр Дремов on 10.12.2020.
//

#ifndef NGG_VARTABLE_H
#define NGG_VARTABLE_H

#include <MachOBuilder.h>
#include "../helpers/StrContainer.h"
#include "../helpers/Stack.h"
#include "../helpers/Optional.h"

namespace NGGC {
    enum VarType {
        Var_Glob,
        Var_Loc,
        Var_None,
    };

    struct VarSingle {
        unsigned rbpOffset;
        VarType type;

        bool operator==(const VarSingle &other) const {
            return rbpOffset == other.rbpOffset && type == other.type;
        }

        void init(unsigned offset) {
            rbpOffset = offset;
            type = Var_None;
        }
    };

    class VarTable {
        ClassicStack <HashMasm<VarSingle>> storage;
        size_t offsetTop;
    public:
        Optional <VarSingle> get(StrContainer *id) {
            Optional <VarSingle> retValue = {};
            retValue.init();

            for (auto elem = storage.rbegin(); elem != storage.rend(); elem++) {
                auto foundElem = elem->find(id->getStorage());
                if (foundElem != elem->end()) {
                    retValue.init((*foundElem).value);
                    return retValue;
                }
            }
            return retValue;
        }

        Optional <VarSingle> getLocalOnly(StrContainer *id) {
            Optional <VarSingle> retValue = {};
            retValue.init();

            auto foundVar = storage.top()->find(id->getStorage());
            if(foundVar == storage.top()->end())
                return retValue;
            retValue.init(foundVar->value);
            return retValue;
        }

        bool def(StrContainer *id) {
            auto trySearch = getLocalOnly(id);
            if (trySearch.hasValue())
                return false;
            VarSingle tmp {};
            tmp.rbpOffset = getLocalOffset();
            tmp.type = Var_Loc;
            if (storage.getSize() == 1)
                tmp.type = Var_Glob;
            storage.top()->set(id->getStorage(), tmp);
            return true;
        }

        void deleteLocal() {
            auto *list = storage.top();
            list->dest();
            storage.pop();
            if (storage.isEmpty())
                offsetTop = 0;
        }

        void addNewLevel() {
            HashMasm<VarSingle> newScope = {};
            newScope.init();
            if (!storage.isEmpty())
                offsetTop += storage.top()->getSize();
            storage.push(newScope);
        }

        void init() {
            offsetTop = 0;
            storage.init(5);
            addNewLevel();
        }

        void dest() {
            while (!storage.isEmpty())
                deleteLocal();
            storage.dest();
        }

        static VarTable *New() {
            auto *ob = static_cast<VarTable *>(calloc(1, sizeof(VarTable)));
            ob->init();
            return ob;
        }

        [[maybe_unused]] static void Delete(VarTable *ob) {
            ob->dest();
            free(ob);
        }

        [[nodiscard]] size_t getLocalOffset() {
            return offsetTop + storage.top()->getSize();
        }
    };
}

#endif //NGG_VARTABLE_H
