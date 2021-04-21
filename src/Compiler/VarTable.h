//
// Created by Александр Дремов on 10.12.2020.
//

#ifndef NGG_VARTABLE_H
#define NGG_VARTABLE_H

#include <FastList.h>
#include "Helpers/StrContainer.h"
#include "Helpers/Stack.h"
#include "Helpers/Optional.h"

namespace NGGC {
    enum VarType {
        Var_Glob,
        Var_Loc,
        Var_None
    };

    struct VarSingle {
        VarType type;
        StrContainer *name;
        int rbpOffset;

        bool operator==(const VarSingle &other) const {
            return strcmp(other.name->getStorage(), name->getStorage()) == 0;
        }

        void init(StrContainer *identifierName) {
            name = identifierName;
            type = Var_None;
            rbpOffset = 0;
        }

        void init(StrContainer *identifierName, int offset) {
            name = identifierName;
            rbpOffset = offset;
            type = Var_None;
        }
    };

    class VarTable {
        ClassicStack <FastList<VarSingle>> storage;
        ClassicStack <size_t> functionScope;
    public:
        Optional <IdCompiler> get(StrContainer *id) {
            Optional <IdCompiler> retValue = {};
            retValue.init();
            VarSingle tmp = {};
            tmp.init(id);
            size_t pos = 0;

            ListOpResult result = LIST_OP_NOTFOUND;
            auto i = (long long) storage.getSize() - 1;
            int offsetModifier = 0;
            for (; i >= 0; i--) {
                result = storage.get(i).search(&pos, tmp);
                if (i + 1 != (long long) storage.getSize())
                    offsetModifier += storage.get(i).getSize();
                if (result == LIST_OP_OK) {
                    break;
                }
            }
            if (result == LIST_OP_NOTFOUND) {
                return retValue;
            }
            storage.get(i).get(pos, &tmp);
            if (tmp.type != Var_Glob)
                tmp.rbpOffset -= offsetModifier;
            retValue.init(tmp);
            return retValue;
        }

        Optional <IdCompiler> getLocalOnly(StrContainer *id) {
            Optional <IdCompiler> retValue = {};
            retValue.init();
            VarSingle tmp = {};
            tmp.init(id);

            size_t pos = 0;
            ListOpResult result = storage.top()->search(&pos, tmp);
            if (result == LIST_OP_NOTFOUND)
                return retValue;
            storage.top()->get(pos, &tmp);
            retValue.init(tmp);
            return retValue;
        }

        bool def(StrContainer *id) {
            VarSingle tmp {};
            tmp.init(id);
            auto trySearch = getLocalOnly(id);
            if (trySearch.hasValue())
                return false;

            tmp.rbpOffset = getLocalOffset();
            tmp.type = Var_Loc;
            if (storage.getSize() == 1)
                tmp.type = Var_Glob;
            storage.top()->pushBack(tmp);
            return true;
        }

        void deleteLocal() {
            auto *list = storage.top();
            list->dest();
            if (!functionScope.isEmpty()) {
                if (*functionScope.top() == storage.getSize())
                    functionScope.pop();
            }
            storage.pop();
        }

        void addNewLevel(bool function = false) {
            FastList<VarSingle> newScope = {};
            newScope.init(0);
            if (function) {
                functionScope.push(storage.getSize());
            }
            storage.push(newScope);
        }

        void init() {
            storage.init(1);
            addNewLevel();
        }

        void dest() {
            while (!storage.isEmpty()) {
                deleteLocal();
            }
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
            return storage.top()->getSize();
        }

        size_t getFunctionOffset() {
            if (functionScope.isEmpty())
                return 0;
            size_t offset = 0;
            for (size_t i = *functionScope.top(); i + 1 < storage.getSize(); ++i)
                offset += storage.get(i).getSize();

            return offset;
        }
    };
}

#endif //NGG_VARTABLE_H
