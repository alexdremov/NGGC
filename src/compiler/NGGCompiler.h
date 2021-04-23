#ifndef NGG_NGG_H
#define NGG_NGG_H

#include <MachOBuilder.h>
#include "VarTable.h"
#include "helpers/ParamsParser.h"
#include "compiler/CompileError.h"
#include "RegisterMaster.h"
#include "ASTLoader/ASTLoader.h"
#include "lexicalAnalysis/LexParser.h"
#include "helpers/ByteContainer.h"
#include "../src/core/bgen/FastStackController.h"
#include "core/eloquent/ASMStructure/ElCommand.h"

namespace NGGC {
    struct offset {
        size_t value;

        void dest() {}

        offset(size_t val) : value(val) {}

        explicit offset(int val) : value(val) {}

        explicit operator size_t() const {
            return value;
        }
    };

    struct functionDefinition {
        ClassicStack<offset> usages;
        size_t definitionOffset;
        unsigned localVarsNum;

        void init() {
            const int startSize = 8;
            usages.init(startSize);
            definitionOffset = 0;
        }

        void dest() {
            usages.dest();
        }
    };

    class NGGCompiler {
        HashMasm<ClassicStack<size_t>> globalVars;
        HashMasm<functionDefinition> functions;
        ClassicStack<CompileError> *cErrors;
        functionDefinition *currentDecl;
        FastList<Lexeme> *parsed;
        ByteContainer *compiled;
        size_t lastDescription;
        RegisterMaster master;
        const char *fileName;
        StrContainer listing;
        VarTable table;
        AST tree;

        void addInstructions(const unsigned *arr, const char *description) {
            addDescription(description);
            while (*arr != COMMANDEND) {
                compiled->append((char) *arr);
                arr++;
            }
        }

        void addInstructions(const unsigned *arr) {
            while (*arr != COMMANDEND) {
                compiled->append((char) *arr);
                arr++;
            }
        }

        void addDescription(const char *msg) {
            printAddedBytes();
            listing.sEndPrintf("%05d | %s\n", compiled->getLen(), msg);
        }

        void addDescription(const char *msg, const char *additional) {
            printAddedBytes();
            listing.sEndPrintf("%05d | %s %s\n", compiled->getLen(), msg, additional);
        }

        void printAddedBytes() {
            if (lastDescription == compiled->getLen())
                return;
            listing.sEndPrintf("%05d |    ", lastDescription);
            for (; lastDescription < compiled->getLen(); lastDescription++)
                listing.sEndPrintf("0x%x ", ((unsigned char *) compiled->getStorage())[lastDescription]);
            listing.sEndPrintf("\n");
        }

        void printImm32(int32_t num) {
            addDescription("IMM32");
            compiled->append((const char *) &num, sizeof(num));
        }

        void printImm8(int8_t num) {
            addDescription("IMM8");
            compiled->append((const char *) &num, sizeof(num));
        }

        void storeRaxToOffsetRbp(int32_t offset) {
            offset *= -1;
            const unsigned movrbx[] = {
                    MOV_MEM_RBP_DISPL32RAX,
                    COMMANDEND};
            addInstructions(movrbx, "Storing rax to rbp - offset");
            printImm32(offset);
        }

        void storeRaxToOffsetRsp(int32_t offset) {
            offset *= -1;
            const unsigned movrbx[] = {
                    MOV_MEM_RSP_DISPL32RAX,
                    COMMANDEND};
            addInstructions(movrbx, "Storing rax to rsp - offset");
            printImm32(offset);
        }

        void leave() {
            const unsigned leave[] = {POP_RBP, RET, COMMANDEND};
            addInstructions(leave, "Leave");
        }

        void processFurther(ASTNode *head, bool noScope = false) {
            if (head == nullptr)
                return;
            switch (head->getKind()) {
                case Kind_Linker:
                    c_Linker(head, noScope);
                    break;
                case Kind_FuncDecl:
                    c_FuncDecl(head);
                    break;
                case Kind_Identifier:
                    c_Identifier(head);
                    break;
                case Kind_Number:
                    c_Number(head);
                    break;
                case Kind_AssignExpr:
                    c_AssignExpr(head);
                    break;
                case Kind_VarDef:
                    c_VarDef(head);
                    break;
                case Kind_MaUnOperator:
                    c_MaUnOperator(head);
                    break;
                case Kind_MaOperator:
                    c_MaOperator(head);
                    break;
                case Kind_Statement:
                    c_Statement(head);
                    break;
                case Kind_FuncCall:
                    c_FuncCall(head);
                    break;
                case Kind_CmpOperator:
                    c_CmpOperator(head);
                    break;
                case Kind_ReturnStmt:
                    c_ReturnStmt(head);
                    break;
                case Kind_Print:
                    c_Print(head);
                    break;
                case Kind_IfStmt:
                    c_IfStmt(head);
                    break;
                case Kind_Input:
                    c_Input(head);
                    break;
                case Kind_None:
                    c_None();
                    break;
                case Kind_WhileStmt:
                    c_WhileStmt(head);
                    break;
                case Kind_BasicFunction:
                    c_BasicFunction(head);
                    break;
                case Kind_Setpix:
                    c_Setpix(head);
                    break;
                default: {
                    CompileError err {};
                    err.init("Undefined sequence: ", head->getLexeme());
                    err.msg->sEndPrintf("%s", ASTNodeKindToString(head->getKind()));
                    cErrors->push(err);
                    break;
                }
            }
        }

        void c_Linker(ASTNode *head, bool noScope = false) {
            bool genNewScope = (head->getLinkKind() == Kind_Link_NewScope) && !noScope;

            if (genNewScope) {
                table.addNewLevel();
                addDescription("New scope created");
            }
            processFurther(head->getLeft());
            if (head->getRight())
                processFurther(head->getRight());
            if (genNewScope) {
                table.deleteLocal();
                addDescription("Exited scope");
            }
        }

        unsigned int countLocalVars(ASTNode *pNode) {
            if (pNode == nullptr)
                return 0;
            return countLocalVars(pNode->getLeft()) + countLocalVars(pNode->getRight()) +
                   (pNode->getKind() == Kind_VarDef ? 1 : 0);
        }

        size_t reserveStack(unsigned int num) {
            if (num == 0)
                return 0;
            const unsigned moversp[] = {SUB_RSPIMM32, COMMANDEND};
            addInstructions(moversp, "sub rsp, imm32 - reserving stack");
            printImm32(num * 8);
            return num;
        }

        void freeStack(unsigned int num) {
            if (num == 0)
                return;
            const unsigned moversp[] = {ADD_RSPIMM32, COMMANDEND};
            addInstructions(moversp, "add rsp, imm32 - free stack");
            printImm32(num * 8);
        }

        void c_FuncDecl(ASTNode *head) {
            StrContainer label = {};
            label.init(head->getLexeme().getString()->begin());
            size_t argCount = 0;
            ASTNode *arg = head->getLeft();
            while (arg != nullptr && arg->getKind() != Kind_None) {
                argCount++;
                arg = arg->getRight();
            }
            label.sEndPrintf("%zu", argCount);

            functionDefinition *elem = nullptr;
            if (functions.find(label.getStorage()) != functions.end()) {
                elem = &functions[label.begin()];
                if (elem->definitionOffset != (size_t) -1) {
                    CompileError err {};
                    err.init("Duplicate function declaration ", head->getLexeme());
                    err.msg->sEndPrintf("%s", label.getStorage());
                    cErrors->push(err);
                    label.dest();
                    return;
                } else {
                    elem->definitionOffset = compiled->getLen();
                    elem->localVarsNum = countLocalVars(head) + argCount;
                }
            } else {
                elem = &functions[label.begin()];
                elem->init();
                elem->definitionOffset = compiled->getLen();
                elem->localVarsNum = countLocalVars(head) + argCount;
            }
            currentDecl = elem;
            addDescription("Function declaration start:", label.getStorage());
            table.addNewLevel();
            master.clear();
            if (head->getLeft() != nullptr && head->getLeft()->getKind() != Kind_None) {
                ASTNode *cur = head->getLeft();
                while (cur != nullptr && cur->getKind() != Kind_None && cur->getLeft() != nullptr) {
                    Lexeme name = cur->getLeft()->getLexeme();
                    table.def(name.getString());
                    cur = cur->getRight();
                }
            }
            const unsigned moverbp[] = {PUSH_RBP, MOV_RBPRSP, COMMANDEND};
            addInstructions(moverbp, "Function enter");
            reserveStack(elem->localVarsNum + elem->localVarsNum % 2);
            processFurther(head->getRight(), true);
            addDescription("Restore preserved");
            master.restorePreserved();
            size_t shift = elem->localVarsNum + elem->localVarsNum % 2 + master.stackUsed();
            freeStack(shift);
            leave();
            table.deleteLocal();
            addDescription("Function declaration end");
            label.dest();
            currentDecl = nullptr;
        }

        void c_FuncCall(ASTNode *head) {
            ASTNode *lastNode = head->getLeft();
            master.moveRsp();

            int argOffset = 3; // return address and rbp
            while (lastNode != nullptr && lastNode->getKind() != Kind_None) {
                processFurther(lastNode->getLeft(), true);
                const unsigned movearg[] = {MOV_MEM_RSP_DISPL32RAX, COMMANDEND};
                addInstructions(movearg, "preparing argument");
                printImm32(argOffset * -8);
                lastNode = lastNode->getRight();
                argOffset++;
            }

            StrContainer label {};
            label.init(head->getLexeme().getString()->begin());
            label.sEndPrintf("%d", argOffset - 3);

            auto foundFunc = functions.find(label.begin());
            if (foundFunc == functions.end()) {
                functions[label.begin()].init();
                functions[label.begin()].definitionOffset = -1;
                foundFunc = functions.find(label.begin());
            }
            addDescription("Prepare call");
            master.prepareCall();
            foundFunc->value.usages.push((size_t) compiled->getLen() + 1);
            const unsigned call[] = {CALL_REL, COMMANDEND};
            addInstructions(call, "call");
            printImm32(0);
            master.moveRspBack();
            label.dest();
        }

        void c_Identifier(ASTNode *head) {
            StrContainer *label = head->getLexeme().getString();
            Optional<VarSingle> found = table.get(label);
            if (!found.hasValue()) {
                CompileError err {};
                err.init("Identifier is not defined: ", head->getLexeme());
                err.msg->sEndPrintf("%s", label->begin());
                cErrors->push(err);
                return;
            }

            addDescription("Load variable from memory:", label->begin());
            unsigned storedReg = master.getVar(found->rbpOffset + (found->type == Var_Loc ? 1 : 0), found->type,
                                               label->begin());
            movCommand movOperation = MOV_TABLE[REG_RAX][storedReg];
            addDescription("Loading var: mov rax, tmpReg");
            compiled->append((char *) movOperation.bytecode, sizeof(movOperation.bytecode));
        }

        void c_Number(ASTNode *head) {
            const unsigned movOperation[] = {MOV_RAXIMM32, COMMANDEND};
            addInstructions(movOperation, "Load number");
            printImm32((int) head->getLexeme().getDouble());
        }

        void c_Setpix(ASTNode *head) {
        }

        void c_MaOperator(ASTNode *head) {
            auto type = head->getLexeme().getType();
            addDescription("Processing math operation:", lexemeTypeToString(type));
            processFurther(head->getLeft(), true);
            addDescription("Processed left of math operator, saving");

            size_t tmpId = 0;
            unsigned regTmp = master.allocateTmp(tmpId);
            movCommand movOperation = MOV_TABLE[regTmp][REG_RAX];
            compiled->append((char *) movOperation.bytecode, sizeof(movOperation.bytecode));

            processFurther(head->getRight(), true);

            regTmp = master.getTmp(tmpId);
            switch (type) {
                case Lex_Plus: {
                    addDescription("add rax, tmpReg");
                    const unsigned char *processed = ADDTABLE[REG_RAX][regTmp];
                    compiled->append((char *) processed, sizeof(ADDTABLE[REG_RAX][regTmp]));
                    break;
                }
                case Lex_Minus: {
                    addDescription("sub rax, tmpReg");
                    const unsigned char *processed = SUBTABLE[regTmp][REG_RAX];
                    compiled->append((char *) processed, sizeof(SUBTABLE[regTmp][REG_RAX]));
                    addDescription("mov rax, tmpReg");
                    const movCommand mov = MOV_TABLE[REG_RAX][regTmp];
                    compiled->append((char *) mov.bytecode, sizeof(mov.bytecode));
                    break;
                }
                case Lex_Mul: {
                    addDescription("imul rax, tmpReg");
                    const unsigned char *processed = IMULTABLE[REG_RAX][regTmp];
                    compiled->append((char *) processed, sizeof(IMULTABLE[REG_RAX][regTmp]));
                    break;
                }
                case Lex_Div: {
                    const unsigned processed[] = {
                            XCHG_RAXRBX,
                            XOR_RDXRDX,
                            IDIV_RBX,
                            COMMANDEND};
                    addInstructions(processed, "idiv rbx, rax");
                    break;
                }
                case Lex_Pow:
                default: {
                    CompileError err {};
                    err.init("Unknown operation in c_MaOperator: ", head->getLexeme());
                    err.msg->sEndPrintf("%s", lexemeTypeToString(type));
                    cErrors->push(err);
                    return;
                }
            }
            master.releaseTmp(tmpId);
        }

        void c_AssignExpr(ASTNode *head) {
            auto type = head->getLexeme().getType();

            ASTNode *idNode = head->getLeft();
            ASTNode *valNode = head->getRight();

            StrContainer *name = idNode->getLexeme().getString();
            Optional<VarSingle> found = table.get(name);

            if (!found.hasValue()) {
                CompileError err {};
                err.init("Identifier was not declared", head->getLexeme());
                err.msg->sEndPrintf("%s", idNode->getLexeme().getString()->begin());
                cErrors->push(err);
                return;
            }
            addDescription("Processing assignment type node. Evaluating rvalue");
            processFurther(valNode, true);
            const unsigned movrbx[] = {MOV_RBXRAX, COMMANDEND};
            addInstructions(movrbx, "mov rbx, rax");
            addDescription("Evaluated. Modifying stored value");
            size_t offset = (found->rbpOffset + 1) * 8;
            switch (type) {
                case Lex_AdAssg: {
                    processFurther(idNode, true);
                    const unsigned processed[] = {
                            ADD_RAXRBX,
                            COMMANDEND
                    };
                    addInstructions(processed, "add rax, rbx - += operation");
                    storeRaxToOffsetRbp(offset);
                    break;
                }
                case Lex_MiAssg: {
                    processFurther(idNode, true);
                    const unsigned processed[] = {
                            SUB_RBXRAX,
                            MOV_RAXRBX,
                            COMMANDEND
                    };
                    addInstructions(processed, "sub rax, rbx - -= operation");
                    storeRaxToOffsetRbp(offset);
                    break;
                }
                case Lex_MuAssg: {
                    processFurther(idNode, true);
                    const unsigned processed[] = {
                            IMUL_RAXRBX,
                            COMMANDEND
                    };
                    addInstructions(processed, "imul rax, rbx - *= operation");
                    storeRaxToOffsetRbp(offset);
                    break;
                }
                case Lex_DiAssg: {
                    processFurther(idNode, true);
                    const unsigned processed[] = {
                            XOR_RDXRDX,
                            XCHG_RAXRBX,
                            IDIV_RBX,
                            COMMANDEND
                    };
                    addInstructions(processed, "idiv rax, rbx - /= operation");
                    storeRaxToOffsetRbp(offset);
                    break;
                }
                case Lex_Assg: {
                    const unsigned processed[] = {
                            MOV_RBXRAX,
                            COMMANDEND
                    };
                    addInstructions(processed, "mov rbx, rax - = operation");
                    storeRaxToOffsetRbp(offset);
                    break;
                }
                default: {
                    CompileError err {};
                    err.init("Unknown operation in c_AssignExpr: ", head->getLexeme());
                    err.msg->sEndPrintf("%s", lexemeTypeToString(type));
                    cErrors->push(err);
                    return;
                }
            }
        }

        void c_VarDef(ASTNode *head) {
            auto type = head->getLexeme();
            bool res = table.def(type.getString());
            if (!res) {
                CompileError err {};
                err.init("Can't redeclare variable: ", head->getLexeme());
                err.msg->sEndPrintf("%s", head->getLexeme().getString()->begin());
                cErrors->push(err);
                return;
            }

            if (head->getLeft() == nullptr || (head->getLeft() != nullptr && head->getLeft()->getKind() == Kind_None)) {
                return;
            }

            processFurther(head->getLeft(), true);
            Optional<VarSingle> found = table.get(type.getString());
            unsigned reg = master.getVar(found->rbpOffset + 1, Var_Loc, type.getString()->begin(), false);
            //            storeRaxToOffsetRbp((found->rbpOffset + 1) * 8);
            const movCommand command = MOV_TABLE[reg][REG_RAX];
            compiled->append((char*)command.bytecode, sizeof(command.bytecode));
        }

        void c_MaUnOperator(ASTNode *head) {
            auto type = head->getLexeme().getType();
            processFurther(head->getLeft(), true);

            if (type == Lex_Minus) {
                const unsigned processed[] = {
                        IMUL_RAXIMM32,
                        COMMANDEND
                };
                addInstructions(processed, "unary * (-1)");
                printImm32(-1);
            }
        }

        void c_Statement(ASTNode *head) {
            processFurther(head->getLeft());
            processFurther(head->getRight());
        }

        void call(const StrContainer &label) {
            master.moveRsp();
            master.prepareCall();
            auto foundFunc = functions.find(label.begin());
            if (foundFunc == functions.end()) {
                functions[label.begin()].init();
                functions[label.begin()].definitionOffset = -1;
                foundFunc = functions.find(label.begin());
            }
            foundFunc->value.usages.push((size_t) compiled->getLen() + 1);
            const unsigned call[] = {CALL_REL, COMMANDEND};
            addInstructions(call, "call");
            printImm32(0);
            master.moveRspBack();
        }

        void c_CmpOperator(ASTNode *head) {
            auto type = head->getLexeme().getType();

            processFurther(head->getLeft(), true);

            size_t tmpId = 0;
            unsigned regTmp = master.allocateTmp(tmpId);
            movCommand movOperation = MOV_TABLE[regTmp][REG_RAX];
            compiled->append((char *) movOperation.bytecode, sizeof(movOperation.bytecode));

            processFurther(head->getRight(), true);

            size_t tmpSecond = 0;
            unsigned regTmpSecond = master.allocateTmp(tmpSecond);
            movOperation = MOV_TABLE[regTmpSecond][REG_RAX];
            compiled->append((char *) movOperation.bytecode, sizeof(movOperation.bytecode));

            regTmp = master.getTmp(tmpId);
            // regTmp - left register; regTmpSecond - rightRegister;

            const unsigned preparecmp[] = {XOR_RAXRAX, COMMANDEND};
            addInstructions(preparecmp, "Xor rax before");

            const unsigned char *compare = CMPTABLE[regTmp][regTmpSecond];
            compiled->append((char *) compare, sizeof(CMPTABLE[regTmp][regTmpSecond]));

            const unsigned valid[] = {MOV_RAXIMM32, 0x01, 0x00, 0x00, 0x00, COMMANDEND};
            const unsigned commSize = sizeof(valid) / sizeof(valid[0]) - 1;
            switch (type) {
                case Lex_Eq: {
                    const unsigned jumpWrong[] = {JNE_REL8, (char) commSize, COMMANDEND};
                    addInstructions(jumpWrong, "equal expected");
                    break;
                }
                case Lex_Leq: {
                    const unsigned jumpWrong[] = {JG_REL8, (char) commSize, COMMANDEND};
                    addInstructions(jumpWrong, "less equal expected");
                    break;
                }
                case Lex_Geq: {
                    const unsigned jumpWrong[] = {JL_REL8, (char) commSize, COMMANDEND};
                    addInstructions(jumpWrong, "greater equal expected");
                    break;
                }
                case Lex_Neq: {
                    const unsigned jumpWrong[] = {JE_REL8, (char) commSize, COMMANDEND};
                    addInstructions(jumpWrong, "not equal expected");
                    break;
                }
                case Lex_Gr: {
                    const unsigned jumpWrong[] = {JLE_REL8, (char) commSize, COMMANDEND};
                    addInstructions(jumpWrong, "greater expected");
                    break;
                }
                case Lex_Le: {
                    const unsigned jumpWrong[] = {JGE_REL8, (char) commSize, COMMANDEND};
                    addInstructions(jumpWrong, "less expected");
                    break;
                }
                default: {
                    CompileError err {};
                    err.init("Unknown operator in c_CmpOperator: ", head->getLexeme());
                    err.msg->sEndPrintf("%s", lexemeTypeToString(type));
                    cErrors->push(err);
                    return;
                }
            }
            addInstructions(valid, "set rax to 1");
            master.releaseTmp(tmpId);
            master.releaseTmp(tmpSecond);
        }

        void c_ReturnStmt(ASTNode *head) {
            if (head->getLeft())
                processFurther(head->getLeft(), true);
            master.restorePreserved();
            size_t backShift = currentDecl->localVarsNum + currentDecl->localVarsNum % 2 + master.stackUsed();
            freeStack(backShift + backShift % 2);
            leave();
        }

        void c_Print(ASTNode *head) {
            processFurther(head->getLeft());
            master.releaseSpecificReg(REG_RDI);
            const unsigned prepareArgs[] = {MOV_RDIRAX, COMMANDEND};
            addInstructions(prepareArgs, "preparing SystemV args");
            StrContainer label = {};
            label.init("_print");
            call(label);
            label.dest();
        }

        void c_Input(ASTNode *head) {
            StrContainer label = {};
            label.init("_in");
            call(label);
            label.dest();
        }

        void c_IfStmt(ASTNode *head) {
            ASTNode *ifBranch = head->getRight()->getLeft();
            ASTNode *elseBranch = head->getRight()->getRight();
            processFurther(head->getLeft());

            const unsigned ifcmd[] = {TEST_RAXRAX, JE_REL32, COMMANDEND};
            addInstructions(ifcmd, "If compare head");
            size_t jumpNumberPos = compiled->getLen();
            printImm32(0);
            size_t trueBranchStart = compiled->getLen();
            processFurther(ifBranch);
            size_t trueBranchEnd = compiled->getLen();
            if (elseBranch != nullptr && elseBranch->getKind() != Kind_None) {
                const unsigned elsecmd[] = {JMP_REL32, COMMANDEND};
                addInstructions(elsecmd, "else command");
                size_t jumpElseNumberPos = compiled->getLen();
                printImm32(0);
                trueBranchEnd = compiled->getLen();
                processFurther(elseBranch);
                size_t elseBranchEnd = compiled->getLen();

                int32_t elseDisplacement = elseBranchEnd - trueBranchEnd;
                compiled->append((char *) &elseDisplacement, sizeof(elseDisplacement), jumpElseNumberPos);
                int32_t displacement = trueBranchEnd - trueBranchStart;
                compiled->append((char *) &displacement, sizeof(displacement), jumpNumberPos);
            } else {
                int32_t displacement = trueBranchEnd - trueBranchStart;
                compiled->append((char *) &displacement, sizeof(displacement), jumpNumberPos);
            }
        }

        void c_WhileStmt(ASTNode *head) {
            size_t beginPosition = compiled->getLen();
            addDescription("While condition");

            processFurther(head->getLeft(), true);

            const unsigned whileExitcmd[] = {TEST_RAXRAX, JE_REL32, COMMANDEND};
            addInstructions(whileExitcmd, "while compare head");
            size_t jumpNumberPos = compiled->getLen();
            printImm32(0);

            size_t bodyStart = compiled->getLen();
            processFurther(head->getRight());
            size_t bodyEnd = compiled->getLen();

            const unsigned whileIterate[] = {JMP_REL32, COMMANDEND};
            addInstructions(whileIterate, "while iterate");
            size_t jumpNumberPosIter = compiled->getLen();
            printImm32(0);
            size_t endPosition = compiled->getLen();

            int32_t displacement = int32_t(endPosition) - bodyStart;
            compiled->append((char *) (&displacement), sizeof(displacement), jumpNumberPos);

            auto displacementIter = int32_t(bodyEnd - beginPosition) * -1 - 5;
            compiled->append((char *) (&displacementIter), sizeof(displacementIter), jumpNumberPosIter);
        }

        void c_BasicFunction(ASTNode *head) {
            CompileError err {};
            err.init("Unsupported at this time: ", head->getLexeme());
            cErrors->push(err);
        }

        void c_None() {}

    public:
        void init() {
            parsed = FastList<Lexeme>::New();
            compiled = ByteContainer::New();
            cErrors = ClassicStack<CompileError>::New();
            functions.init();
            globalVars.init();
            listing.init();
            table.init();
            lastDescription = 0;
            master.init(compiled);
        }

        void init(ASTNode *head) {
            init();
            tree.dest();
            tree.init(head);
        }

        void dest() {
            ClassicStack<CompileError>::Delete(cErrors);
            ByteContainer::Delete(compiled);
            for (auto i = functions.begin(); i != functions.end(); i++) {
                (*i).value.dest();
                i->dest();
            }
            functions.dest();
            for (auto i = globalVars.begin(); i != globalVars.end(); i++)
                i->dest();
            globalVars.dest();
            parsed->Delete();
            listing.dest();
            master.dest();
            table.dest();
            tree.dest();
        }

        static NGGCompiler *New() {
            auto *ob = static_cast<NGGCompiler *>(calloc(1, sizeof(NGGCompiler)));
            ob->init();
            return ob;
        }

        static void Delete(NGGCompiler *ob) {
            ob->dest();
            free(ob);
        }

        void compile() {
            processFurther(tree.getHead());
            printAddedBytes();
        }

        bool loadFile(const char *filePath) {
            StrContainer content {};
            content.init();
            fileName = filePath;

            FILE *sourceCode = fopen(filePath, "rb");
            if (!sourceCode) {
                return false;
            }

            content.readFromFile(sourceCode);
            fclose(sourceCode);

            auto *res = NGGC::LexParser::parse(&content);

            auto ASTParser = NGGC::AST {};
            ASTParser.init();
            ASTParser.parse(res);
            this->tree = ASTParser;
            this->parsed->Delete();
            this->parsed = res;
            content.dest();
            return true;
        }

        void printLexemes(const char *lexfileName) {
            FILE *file = fopen(lexfileName, "w");
            LexParser::dumpLexemes(*parsed, file);
            fclose(file);
        }

        bool isParseSuccessful() {
            return !tree.hasError();
        }

        bool isCompileSuccessful() {
            return cErrors->isEmpty();
        }

        void dumpCompileErrorStack(const char *inputFileName) {
            for (unsigned i = 0; i < cErrors->getSize(); ++i) {
                const CompileError &err = cErrors->get(i);
                printf("%s:%zu:%zu: error: %s\n", inputFileName, err.line + 1, err.col, err.msg->begin());
            }
        }

        void dumpErrorStack(const char *inputFileName) {
            tree.dumpParseErrorStack(inputFileName);
        };

        void dumpGraph() {
            FILE *file = fopen("graph.gv", "wb");
            tree.dumpTree(file);
            fclose(file);
            system("dot -Tsvg graph.gv -o code.svg");
            system("dot -Tpng graph.gv -o code.png && rm graph.gv");
        }

        void dumpTree() {
            FILE *file = fopen("treestruct.txt", "w");
            ASTLoader::dump(tree.getHead(), file);
        }

        const StrContainer &getListing() {
            return listing;
        }

        void genObject(const char *file) {
            FILE *res = fopen(file, "wb");
            binaryFile binary = {};
            binary.init(res);

            ObjectMachOGen mgen = {};
            mgen.init();

            unsigned char data[] = {
                    0xDE, 0xD3, 0x2D, 0xED, 0x32, 0xDE, 0xD3, 0x2D, 0xED, 0x32,
                    0xDE, 0xD3, 0x2D, 0xED, 0x32, 0xDE, 0xD3, 0x2D, 0xED, 0x32,
            };

            mgen.addCode(compiled->begin(), compiled->getLen());
            mgen.addData(data, sizeof(data));

            for (auto &elem: functions) {
                StrContainer label = {};
                label.init("_");
                label.sEndPrintf("%s", elem.key);
                if (elem.value.definitionOffset != size_t(-1)) {
                    mgen.addInternalCodeSymbol(label.begin(), elem.value.definitionOffset);
                    for (auto &usage: elem.value.usages)
                        mgen.bindSignedOffset(label.begin(), usage.value);
                } else {
                    for (auto &usage: elem.value.usages)
                        mgen.bindBranchExt(elem.key, usage.value);
                }
                label.dest();
            }

            mgen.dumpFile(binary);

            mgen.dest();
            binary.dest();
        }
    };
}

#endif //NGG_NGG_H
