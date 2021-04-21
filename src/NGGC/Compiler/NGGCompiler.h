//
// Created by Александр Дремов on 10.12.2020.
//

#ifndef NGG_NGG_H
#define NGG_NGG_H

#include "VarTable.h"
#include "Helpers/ParamsParser.h"
#include "Compiler/CompileError.h"
#include "ASTLoader/ASTLoader.h"
#include "LexicalAnalysis/LexParser.h"
#include "Helpers/ByteContainer.h"
#include "core/eloquent/ASMStructure/ElCommand.h"

namespace NGGC {
    class NGGCompiler {
        ByteContainer *compiled;
        FastList<Lexeme> *parsed;
        ClassicStack<CompileError> *cErrors;
        AST tree;
        VarTable env;
        const char *fileName;

        void addInstructions(const unsigned* arr){
            while(*arr != COMMANDEND){
                compiled->append((char) *arr);
                arr++;
            }
        }

        void processFurther(ASTNode *head, bool valueNeeded = false, bool noScope = false) {
            if (head == nullptr)
                return;
            switch (head->getKind()) {
                case Kind_Linker:
                    c_Linker(head, valueNeeded, noScope);
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
                    c_FuncCall(head, valueNeeded);
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
                    c_BasicFunction(head, valueNeeded);
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

        void c_Linker(ASTNode *head, bool valueNeeded, bool noScope = false) {

        }

        void c_FuncDecl(ASTNode *head) {

        }

        void c_Identifier(ASTNode *head) {

        }

        void c_Number(ASTNode *head) {

        }

        void c_Setpix(ASTNode *head) {

        }

        void c_MaOperator(ASTNode *head) {

        }

        void c_AssignExpr(ASTNode *head) {

        }

        void c_VarDef(ASTNode *head) {

        }

        void c_MaUnOperator(ASTNode *head) {

        }

        void c_Statement(ASTNode *head) {
        }

        void c_FuncCall(ASTNode *head, bool valueNeeded = false) {

        }

        void c_CmpOperator(ASTNode *head) {

        }

        void c_ReturnStmt(ASTNode *head) {

        }

        void c_Print(ASTNode *head) {

        }

        void c_Input(ASTNode *head) {

        }

        void c_IfStmt(ASTNode *head) {

        }

        void c_None() {}

        void c_WhileStmt(ASTNode *head) {

        }

        void c_BasicFunction(ASTNode *head, bool valueNeeded = false) {

        }

    public:
        void init() {
            tree.init();
            env.init();
            fileName = nullptr;
            parsed = FastList<Lexeme>::New();
            compiled = ByteContainer::New();
            cErrors = ClassicStack<CompileError>::New();
        }

        void init(ASTNode *head) {
            this->init();
            tree.dest();
            tree.init(head);
            parsed = FastList<Lexeme>::New();
        }

        void dest() {
            tree.dest();
            env.dest();
            parsed->Delete();
            ByteContainer::Delete(compiled);
            ClassicStack<CompileError>::Delete(cErrors);
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
    };
}

#endif //NGG_NGG_H
