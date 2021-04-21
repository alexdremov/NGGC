//
// Created by Александр Дремов on 09.12.2020.
//

#ifndef NGG_AST_H
#define NGG_AST_H

#include "ASTNode.h"
#include "LexicalAnalysis/Lexeme.h"
#include "Helpers/Optional.h"
#include "ParsePosition.h"
#include "Helpers/Stack.h"
#include "ASTError.h"

#define LEX_SET_IF_NONE(storage, lex) \
if (expectLexAndMove(pos, lex) && storage == Lex_None) { \
    storage = lex; \
}

namespace NGGC {
    class AST {
        ASTNode *head;
        ClassicStack<ASTError> errorStack;

        void error(ParsePosition &pos, const char *msg) {
            ASTError err = {pos.get(), msg};
            errorStack.push(err);
        }

        static bool expectLex(ParsePosition &pos, LexemeType type) {
            Lexeme lex = pos.get();
            return type == lex.getType();
        }

        static bool expectLexAndMove(ParsePosition &pos, LexemeType type) {
            Lexeme lex = pos.get();
            bool res = type == lex.getType();
            if (res)
                ++pos;
            return res;
        }

        static ASTNode *getNoneNode() {
            ASTNode *node = ASTNode::New();
            node->init();
            return node;
        }

        static Lexeme getNoneLexeme() {
            Lexeme lex {};
            lex.init(Lex_None);
            return lex;
        }

        Optional<ASTNode *> p_FuncDecl(ParsePosition &pos) {
            Optional<ASTNode *> retValue {};
            retValue.init();

            if (!expectLexAndMove(pos, Lex_FDecl)) {
                return retValue;
            }

            if (!expectLex(pos, Lex_Identifier)) {
                error(pos, "Identifier expected in function declaration");
                return retValue;
            }

            Lexeme name = pos.getMove();

            if (!expectLexAndMove(pos, Lex_LPA)) {
                error(pos, "Left parenthesis expected in function arg list");
                return retValue;
            }

            Optional<ASTNode *> argList = p_ArgumentsList(pos);
            if (!argList.hasValue()) {
                argList.init(getNoneNode());
            }

            if (!expectLexAndMove(pos, Lex_RPA)) {
                error(pos, "Right parenthesis expected in function arg list");
                argList.unwrap()->dest();
                return retValue;
            }

            Optional<ASTNode *> blockStmt = p_BlockStmt(pos);
            if (!blockStmt.hasValue()) {
                error(pos, "Block statement expected in function definition");
                return retValue;
            }

            ASTNode *node = ASTNode::New();
            node->init(Kind_FuncDecl, name, argList.unwrap(), blockStmt.unwrap());

            retValue.init(node);
            return retValue;
        }

        Optional<ASTNode *> p_ArgumentsList(ParsePosition &pos) {
            Optional<ASTNode *> retValue {};
            retValue.init();

            if (!expectLex(pos, Lex_Identifier))
                return retValue;

            ASTNode *linksHead = ASTNode::New();
            ASTNode *linkEnd = linksHead;

            linksHead->init(Kind_Linker, Kind_Identifier);

            Lexeme lex = pos.getMove();

            ASTNode *newNode = ASTNode::New();
            newNode->init(Kind_Identifier, lex);
            linkEnd->setLeft(newNode);

            while (!pos.isEnded()) {
                if (!expectLexAndMove(pos, Lex_Comma))
                    break;

                lex = pos.getMove();
                if (lex.getType() != Lex_Identifier) {
                    error(pos, "Identifier expected after comma in arg list");
                    return retValue;
                }

                ASTNode *newLinkNode = ASTNode::New();
                newLinkNode->init(Kind_Linker, Kind_Identifier);

                newNode = ASTNode::New();
                newNode->init(Kind_Identifier, lex);
                newLinkNode->setLeft(newNode);

                linkEnd->setRight(newLinkNode);
                linkEnd = newLinkNode;
            }

            retValue.init(linksHead);
            return retValue;
        }

        Optional<ASTNode *> p_BlockStmt(ParsePosition &pos) {
            Optional<ASTNode *> retValue {};
            retValue.init();

            if (!expectLexAndMove(pos, Lex_BStart))
                return retValue;

            ASTNode *linksHead = nullptr;
            ASTNode *linkEnd = nullptr;

            while (!pos.isEnded()) {
                Optional<ASTNode *> statement = p_Statement(pos);
                if (!statement.hasValue())
                    break;

                if (linksHead == nullptr) {
                    linksHead = ASTNode::New();
                    linksHead->init(Kind_Linker, Kind_Link_NewScope);
                    linkEnd = linksHead;
                }

                if (linkEnd->getLeft() != nullptr) {
                    ASTNode *linksNew = ASTNode::New();
                    linksNew->init(Kind_Linker, Kind_Statement);
                    linkEnd->setRight(linksNew);
                    linkEnd = linksNew;
                }
                linkEnd->setLeft(statement.unwrap());
            }

            if (!expectLexAndMove(pos, Lex_BEnd)) {
                error(pos, "No block end found");
                return retValue;
            }

            if (linksHead == nullptr)
                linksHead = getNoneNode();

            retValue.init(linksHead);
            return retValue;
        }

        Optional<ASTNode *> p_Statement(ParsePosition &pos) {
            Optional<ASTNode *> retValue {};
            retValue.init();
            Optional<ASTNode *> content {};
            content.init();

            size_t checkpoint = pos.getPos();

            content = p_VarDef(pos);
            if (!content.hasValue())
                content = p_Print(pos);
            if (!content.hasValue())
                content = p_AssignExpr(pos);
            if (!content.hasValue())
                content = p_Setpix(pos);
            if (!content.hasValue())
                content = p_FuncCall(pos);

            if (content.hasValue()) {
                if (!expectLexAndMove(pos, Lex_StEnd)) {
                    pos.restore(checkpoint);
                    return retValue;
                }
            } else {
                content = p_ReturnStmt(pos);
                if (!content.hasValue())
                    content = p_IfStmt(pos);
                if (!content.hasValue())
                    content = p_WhileStmt(pos);
                if (!content.hasValue())
                    content = p_BlockStmt(pos);
                if (!content.hasValue())
                    return retValue;
            }
            return content;
        }

        Optional<ASTNode *> p_VarDef(ParsePosition &pos) {
            Optional<ASTNode *> retValue {};
            retValue.init();

            if (!expectLexAndMove(pos, Lex_VDecl))
                return retValue;

            if (!expectLex(pos, Lex_Identifier)) {
                error(pos, "No identifier after var definition");
                return retValue;
            }

            Lexeme name = pos.getMove();
            auto *node = ASTNode::New();
            node->init(Kind_VarDef, name);

            if (expectLexAndMove(pos, Lex_Assg)) {
                Optional<ASTNode *> value = p_rValue(pos);
                if (!value.hasValue()) {
                    error(pos, "No rvalue found after assignment in var declaration");
                    return retValue;
                }
                node->setLeft(value.unwrap());
            } else {
                node->setLeft(getNoneNode());
            }
            retValue.init(node);
            return retValue;
        }

        Optional<ASTNode *> p_Setpix(ParsePosition &pos) {
            Optional<ASTNode *> retValue {};
            retValue.init();

            if (!expectLexAndMove(pos, Lex_Setpix))
                return retValue;

            Optional<ASTNode *> x = p_rValue(pos);
            Optional<ASTNode *> y = p_rValue(pos);
            Optional<ASTNode *> val = p_rValue(pos);

            if (!(x.hasValue() && y.hasValue() && val.hasValue()))
                return retValue;

            auto *node = ASTNode::New();
            auto *coordsNode = ASTNode::New();

            coordsNode->init(Kind_Linker, getNoneLexeme(), x.unwrap(), y.unwrap());
            node->init(Kind_Setpix, getNoneLexeme(), coordsNode, val.unwrap());

            retValue.init(node);
            return retValue;
        }

        Optional<ASTNode *> p_VarDefStmt(ParsePosition &pos) {
            Optional<ASTNode *> retValue {};
            retValue.init();

            Optional<ASTNode *> assg = p_VarDef(pos);

            if (!assg.hasValue())
                return retValue;

            if (!expectLexAndMove(pos, Lex_StEnd)) {
                error(pos, "No statement end after var assignment in VarDefStmt");
                return retValue;
            }

            auto *node = ASTNode::New();
            node->init(Kind_Statement, getNoneLexeme(), assg.unwrap());
            retValue.init(node);
            return retValue;
        }

        Optional<ASTNode *> p_rValue(ParsePosition &pos) {
            Optional<ASTNode *> retValue {};
            retValue.init();

            Optional<ASTNode *> firstValue = p_AddSubExpr(pos);

            if (!firstValue.hasValue())
                return retValue;

            Lexeme opLex = pos.get();
            LexemeType op = Lex_None;
            LEX_SET_IF_NONE(op, Lex_Eq)
            else LEX_SET_IF_NONE(op, Lex_Leq)
            else LEX_SET_IF_NONE(op, Lex_Geq)
            else LEX_SET_IF_NONE(op, Lex_Neq)
            else LEX_SET_IF_NONE(op, Lex_Gr)
            else LEX_SET_IF_NONE(op, Lex_Le)

            if (op != Lex_None) {
                Optional<ASTNode *> secondValue = p_AddSubExpr(pos);
                if (!secondValue.hasValue()) {
                    error(pos, "No second operand found after condition operator");
                    return retValue;
                }

                auto *opNode = ASTNode::New();
                opNode->init(Kind_CmpOperator, opLex, firstValue.unwrap(), secondValue.unwrap());
                retValue.init(opNode);
                return retValue;
            } else {
                retValue.init(firstValue.unwrap());
                return retValue;
            }
        }

        Optional<ASTNode *> p_AddSubExpr(ParsePosition &pos) {
            Optional<ASTNode *> retValue {};
            retValue.init();

            Optional<ASTNode *> firstValue = p_MulDivExpr(pos);

            if (!firstValue.hasValue())
                return retValue;

            ASTNode *lastNode = firstValue.unwrap();

            while (!pos.isEnded()) {
                Lexeme opLex = pos.get();
                LexemeType op = Lex_None;

                LEX_SET_IF_NONE(op, Lex_Plus)
                else LEX_SET_IF_NONE(op, Lex_Minus)

                if (op != Lex_None) {
                    Optional<ASTNode *> secondValue = p_MulDivExpr(pos);
                    if (!secondValue.hasValue()) {
                        error(pos, "No operand found after +/- operator");
                        return retValue;
                    }
                    auto *opNode = ASTNode::New();
                    opNode->init(Kind_MaOperator, opLex, lastNode, secondValue.unwrap());
                    lastNode = opNode;
                } else {
                    retValue.init(lastNode);
                    return retValue;
                }
            }

            retValue.init(lastNode);
            return retValue;
        }

        Optional<ASTNode *> p_MulDivExpr(ParsePosition &pos) {
            Optional<ASTNode *> retValue {};
            retValue.init();

            Optional<ASTNode *> firstValue = p_UnaryExpr(pos);

            if (!firstValue.hasValue())
                return retValue;

            ASTNode *lastNode = firstValue.unwrap();

            while (!pos.isEnded()) {
                Lexeme opLex = pos.get();
                LexemeType op = Lex_None;

                LEX_SET_IF_NONE(op, Lex_Mul)
                else LEX_SET_IF_NONE(op, Lex_Div)
                else LEX_SET_IF_NONE(op, Lex_Pow)

                if (op != Lex_None) {
                    Optional<ASTNode *> secondValue = p_UnaryExpr(pos);
                    if (!secondValue.hasValue()) {
                        error(pos, "No operand found after */ operator");
                        return retValue;
                    }
                    auto *opNode = ASTNode::New();
                    opNode->init(Kind_MaOperator, opLex, lastNode, secondValue.unwrap());
                    lastNode = opNode;
                } else {
                    retValue.init(lastNode);
                    return retValue;
                }
            }

            retValue.init(lastNode);
            return retValue;
        }

        Optional<ASTNode *> p_UnaryExpr(ParsePosition &pos) {
            Optional<ASTNode *> retValue {};
            retValue.init();

            Lexeme opLex = pos.get();
            LexemeType op = Lex_None;

            LEX_SET_IF_NONE(op, Lex_Plus)
            else LEX_SET_IF_NONE(op, Lex_Minus)
            Optional<ASTNode *> value = p_PrimaryExpr(pos);

            if (op != Lex_None) {
                if (!value.hasValue()) {
                    error(pos, "No value found in unary after +/-");
                    return retValue;
                }
                auto *opNode = ASTNode::New();

                opNode->init(Kind_MaUnOperator, opLex, value.unwrap(), nullptr);
                retValue.init(opNode);
                return retValue;
            } else {
                if (!value.hasValue()) {
                    return retValue;
                }
                retValue.init(value.unwrap());
                return retValue;
            }
        }

        Optional<ASTNode *> p_Input(ParsePosition &pos) {
            Optional<ASTNode *> retValue {};
            retValue.init();

            if (expectLexAndMove(pos, Lex_Input)) {
                auto *node = ASTNode::New();
                node->init(Kind_Input, getNoneLexeme());
                retValue.init(node);
                return retValue;
            }
            return retValue;
        }

        Optional<ASTNode *> p_Sin(ParsePosition &pos) {
            return commonBasicFunction(pos, Lex_Sin);
        }

        Optional<ASTNode *> p_Cos(ParsePosition &pos) {
            return commonBasicFunction(pos, Lex_Cos);
        }

        Optional<ASTNode *> p_Tan(ParsePosition &pos) {
            return commonBasicFunction(pos, Lex_Tan);
        }

        Optional<ASTNode *> p_Exp(ParsePosition &pos) {
            return commonBasicFunction(pos, Lex_Exp);
        }

        Optional<ASTNode *> p_Sqrt(ParsePosition &pos) {
            return commonBasicFunction(pos, Lex_Sqrt);
        }

        Optional<ASTNode *> p_Abs(ParsePosition &pos) {
            return commonBasicFunction(pos, Lex_Abs);
        }

        Optional<ASTNode *> p_BasicFunc(ParsePosition &pos) {
            Optional<ASTNode *> retValue {};
            retValue.init();

            Optional<ASTNode *> tryParse = p_Sin(pos);
            if (tryParse.hasValue()) return tryParse;

            tryParse = p_Cos(pos);
            if (tryParse.hasValue()) return tryParse;

            tryParse = p_Tan(pos);
            if (tryParse.hasValue()) return tryParse;

            tryParse = p_Sqrt(pos);
            if (tryParse.hasValue()) return tryParse;

            tryParse = p_Abs(pos);
            if (tryParse.hasValue()) return tryParse;

            tryParse = p_Input(pos);
            if (tryParse.hasValue()) return tryParse;

            tryParse = p_Exp(pos);
            if (tryParse.hasValue()) return tryParse;
            return retValue;
        }

        Optional<ASTNode *> p_PrimaryExpr(ParsePosition &pos) {
            Optional<ASTNode *> retValue {};
            retValue.init();

            if (expectLexAndMove(pos, Lex_LPA)) {
                Optional<ASTNode *> value = p_rValue(pos);
                if (!value.hasValue()) {
                    error(pos, "No value found in parenthesis");
                    return retValue;
                }
                if (!expectLexAndMove(pos, Lex_RPA)) {
                    error(pos, "No closing parenthesis");
                    return retValue;
                }

                retValue.init(value.unwrap());
                return retValue;
            }

            Optional<ASTNode *> basicFunction = p_BasicFunc(pos);
            if (basicFunction.hasValue()) {
                retValue.init(basicFunction.unwrap());
                return retValue;
            }

            if (expectLex(pos, Lex_Number)) {
                Lexeme num = pos.getMove();

                auto *node = ASTNode::New();
                node->init(Kind_Number, num);
                retValue.init(node);
                return retValue;
            }

            Optional<ASTNode *> funcCall = p_FuncCall(pos);
            if (funcCall.hasValue()) {
                retValue.init(funcCall.unwrap());
                return retValue;
            } else if (expectLex(pos, Lex_Identifier)) {
                Lexeme id = pos.getMove();
                auto *node = ASTNode::New();
                node->init(Kind_Identifier, id);
                retValue.init(node);
                return retValue;
            }
            return retValue;
        }

        Optional<ASTNode *> p_FuncCall(ParsePosition &pos) {
            Optional<ASTNode *> retValue {};
            retValue.init();

            if (!expectLex(pos, Lex_Identifier))
                return retValue;

            Lexeme name = pos.getMove();
            if (!expectLexAndMove(pos, Lex_LPA)) {
                --pos;
                return retValue;
            }

            Optional<ASTNode *> callList = p_CallList(pos);

            if (!callList.hasValue()) {
                callList.init(getNoneNode());
            }

            if (!expectLexAndMove(pos, Lex_RPA)) {
                error(pos, "No closing parenthesis after arg list");
                return retValue;
            }

            auto *node = ASTNode::New();
            node->init(Kind_FuncCall, name, callList.unwrap());
            retValue.init(node);

            return retValue;
        }

        Optional<ASTNode *> p_CallList(ParsePosition &pos) {
            Optional<ASTNode *> retValue {};
            retValue.init();

            Optional<ASTNode *> firstArg = p_rValue(pos);
            if (!firstArg.hasValue())
                return retValue;

            ASTNode *linksHead = ASTNode::New();
            ASTNode *linkEnd = linksHead;

            linksHead->init(Kind_Linker, Kind_rValue);
            linkEnd->setLeft(firstArg.unwrap());

            while (!pos.isEnded()) {
                if (!expectLexAndMove(pos, Lex_Comma))
                    break;

                Optional<ASTNode *> nextArg = p_rValue(pos);

                if (!nextArg.hasValue()) {
                    error(pos, "No argument found after comma in arg list");
                    return retValue;
                }

                ASTNode *newLinkNode = ASTNode::New();
                newLinkNode->init(Kind_Linker, Kind_rValue);
                newLinkNode->setLeft(nextArg.unwrap());

                linkEnd->setRight(newLinkNode);
                linkEnd = newLinkNode;
            }

            retValue.init(linksHead);
            return retValue;
        }

        Optional<ASTNode *> p_ReturnStmt(ParsePosition &pos) {
            Optional<ASTNode *> retValue {};
            retValue.init();

            if (!expectLexAndMove(pos, Lex_Return))
                return retValue;

            Optional<ASTNode *> value = p_rValue(pos);

            if (!value.hasValue()) {
                value.init(getNoneNode());
            }

            if (!expectLexAndMove(pos, Lex_StEnd)) {
                error(pos, "No statement end after return statement");
                return retValue;
            }

            auto *node = ASTNode::New();
            node->init(Kind_ReturnStmt, getNoneLexeme(), value.unwrap());
            retValue.init(node);
            return retValue;
        }

        Optional<ASTNode *> p_AssignExpr(ParsePosition &pos) {
            Optional<ASTNode *> retValue {};
            retValue.init();

            if (!expectLex(pos, Lex_Identifier))
                return retValue;

            Lexeme name = pos.getMove();
            Lexeme opLex = pos.get();
            LexemeType op = Lex_None;

            LEX_SET_IF_NONE(op, Lex_Assg)
            else LEX_SET_IF_NONE(op, Lex_AdAssg)
            else LEX_SET_IF_NONE(op, Lex_MiAssg)
            else LEX_SET_IF_NONE(op, Lex_MuAssg)
            else LEX_SET_IF_NONE(op, Lex_DiAssg)

            if (op == Lex_None) {
                --pos;
                return retValue;
            }
            Optional<ASTNode *> value = p_rValue(pos);
            if (!value.hasValue()) {
                error(pos, "No value after assign-type operator");
                return retValue;
            }

            auto *node = ASTNode::New();
            auto *nodeId = ASTNode::New();
            nodeId->init(Kind_Identifier, name);

            node->init(Kind_AssignExpr, opLex, nodeId, value.unwrap());

            retValue.init(node);
            return retValue;
        }

        Optional<ASTNode *> p_Print(ParsePosition &pos) {
            Optional<ASTNode *> retValue {};
            retValue.init();

            if (!expectLexAndMove(pos, Lex_Print))
                return retValue;

            Optional<ASTNode *> value = p_rValue(pos);
            if (!value.hasValue()) {
                error(pos, "No value after print req");
                return retValue;
            }

            auto *node = ASTNode::New();
            node->init(Kind_Print, getNoneLexeme(), value.unwrap());
            retValue.init(node);
            return retValue;
        }

        Optional<ASTNode *> p_IfStmt(ParsePosition &pos) {
            Optional<ASTNode *> retValue {};
            retValue.init();

            if (!expectLex(pos, Lex_If))
                return retValue;
            Lexeme ifLexeme = pos.getMove();

            if (!expectLexAndMove(pos, Lex_LPA)) {
                error(pos, "No open parenthesis after if");
                return retValue;
            }

            Optional<ASTNode *> cond = p_rValue(pos);
            if (!cond.hasValue()) {
                error(pos, "Invalid if condition");
                return retValue;
            }

            if (!expectLexAndMove(pos, Lex_RPA)) {
                error(pos, "No close parenthesis after condition");
                return retValue;
            }

            Optional<ASTNode *> posStmt = p_BlockStmt(pos);

            if (!posStmt.hasValue()) {
                error(pos, "No block statement after if");
                return retValue;
            }

            Optional<ASTNode *> elseStmt {};
            elseStmt.init();

            if (expectLexAndMove(pos, Lex_Else)) {
                elseStmt = p_BlockStmt(pos);
                if (!elseStmt.hasValue()) {
                    error(pos, "No block statement after if-else");
                    return retValue;
                }
            }

            if (!elseStmt.hasValue())
                elseStmt.init(getNoneNode());

            auto *fork = ASTNode::New();
            fork->init(Kind_Linker, Kind_IfStmt, posStmt.unwrap(), elseStmt.unwrap());

            auto *node = ASTNode::New();
            node->init(Kind_IfStmt, ifLexeme, cond.unwrap(), fork);
            retValue.init(node);
            return retValue;
        }

        Optional<ASTNode *> commonBasicFunction(ParsePosition &pos, LexemeType type) {
            Optional<ASTNode *> retValue {};
            retValue.init();

            Lexeme lex = pos.get();
            if (!expectLexAndMove(pos, type)) {
                return retValue;
            }
            if (!expectLexAndMove(pos, Lex_LPA)) {
                error(pos, "No open parenthesis in basic function call");
                return retValue;

            }

            Optional<ASTNode *> value = p_rValue(pos);
            if (!value.hasValue()) {

                error(pos, "No vaue found after basic function");
                return retValue;

            }
            if (!expectLexAndMove(pos, Lex_RPA)) {

                error(pos, "No close parenthesis in basic function call");
                return retValue;

            }

            ASTNode *node = ASTNode::New();
            node->init(Kind_BasicFunction, lex, value.unwrap(), nullptr);
            retValue.init(node);
            return retValue;
        }

        Optional<ASTNode *> p_WhileStmt(ParsePosition &pos) {
            Optional<ASTNode *> retValue {};
            retValue.init();

            Lexeme lex = pos.get();
            if (!expectLexAndMove(pos, Lex_While))
                return retValue;

            if (!expectLexAndMove(pos, Lex_LPA)) {
                error(pos, "No open parenthesis after condition");
                return retValue;
            }

            Optional<ASTNode *> cond = p_rValue(pos);
            if (!cond.hasValue()) {
                error(pos, "Invalid while condition");
                return retValue;
            }

            if (!expectLexAndMove(pos, Lex_RPA)) {
                error(pos, "No close parenthesis after condition");
                return retValue;
            }

            Optional<ASTNode *> exec = p_BlockStmt(pos);

            if (!exec.hasValue()) {
                error(pos, "No block statement after while");
                return retValue;
            }

            auto *node = ASTNode::New();
            node->init(Kind_WhileStmt, lex, cond.unwrap(), exec.unwrap());
            retValue.init(node);
            return retValue;
        }

        void recursiveDump(FILE *file, ASTNode *node) {
            if (!node)
                return;

            if (node->getKind() == Kind_Number) {
                fprintf(file, "node%p[label=\"Number<%lg>:%zu:%zu\" shape=oval fillcolor=pink style=filled]\n", (void*)node,
                        node->getLexeme().getDouble(), node->getLexeme().getLine(), node->getLexeme().getCol());
            } else if (node->getKind() == Kind_Identifier) {
                fprintf(file, "node%p[label=\"ID<%s>:%zu:%zu\" shape=oval fillcolor=aquamarine style=filled]\n", (void*)node,
                        node->getLexeme().getString()->begin(), node->getLexeme().getLine(),
                        node->getLexeme().getCol());
            } else {
                if (node->getLexeme().isStringUsed())
                    fprintf(file,
                            "node%p[label=\"%s<%s<%s>>:%zu:%zu\" shape=invhouse fillcolor=darkseagreen1 style=filled]\n",
                            (void*)node,
                            ASTNodeKindToString(node->getKind()), lexemeTypeToString(node->getLexeme().getType()),
                            node->getLexeme().getString()->begin(), node->getLexeme().getLine(),
                            node->getLexeme().getCol());
                else if (node->getKind() == Kind_Linker)
                    if (node->getLinkKind() != Kind_Link_NewScope)
                        fprintf(file, "node%p[label=\"%s\" shape=invhouse fillcolor=darkseagreen1 style=filled]\n",
                                (void*)node,
                                ASTNodeKindToString(node->getKind()));
                    else
                        fprintf(file, "node%p[label=\"%s(ns)\" shape=invhouse fillcolor=yellow style=filled]\n",
                                (void*)node,
                                ASTNodeKindToString(node->getKind()));
                else
                    fprintf(file,
                            "node%p[label=\"%s<%s>:%zu:%zu\" shape=invhouse fillcolor=darkseagreen1 style=filled]\n",
                            (void*)node,
                            ASTNodeKindToString(node->getKind()), lexemeTypeToString(node->getLexeme().getType()),
                            node->getLexeme().getLine(),
                            node->getLexeme().getCol());

            }


            if (node->getLeft()) {
//                if (node->getLeft()->getKind() == Kind_Linker && node->getKind() == node->getLeft()->getKind())
//                    fprintf(file, "{rank=same; node%p; node%p}\n", node, node->getLeft());

                fprintf(file, "node%p->node%p[color=green label=L]\n", (void*)node, (void*)node->getLeft());
                recursiveDump(file, node->getLeft());
            }
            if (node->getRight()) {
//                if (node->getRight()->getKind() == Kind_Linker && node->getKind() == node->getRight()->getKind())
//                    fprintf(file, "{rank=same; node%p; node%p}\n", node, node->getRight());
                fprintf(file, "node%p->node%p[color=red label=R]\n", (void*)node, (void*)node->getRight());
                recursiveDump(file, node->getRight());
            }
        }

    public:
        void init() {
            this->errorStack.init(0);
        }

        void init(ASTNode* newHead) {
            this->head = newHead;
            this->errorStack.init(0);
        }

        void dest() {
            if (head)
                ASTNode::Delete(head);
            this->errorStack.dest();
        }

        static ASTNode *New() {
            auto *ob = static_cast<ASTNode *>(calloc(1, sizeof(ASTNode)));
            ob->init();
            return ob;
        }

        static void Delete(ASTNode *ob) {
            ob->dest();
            free(ob);
        }

        void parse(FastList<Lexeme> *input) {
            ParsePosition pos {};
            pos.init(input);

            if (input->isEmpty()) {
                head = getNoneNode();
                return;
            }

            auto res = p_FuncDecl(pos);
            if (!res.hasValue()) {
                res = p_VarDefStmt(pos);
                if (!res.hasValue()) {
                    error(pos, "Undefined root sequence");
                    return;
                }
            }

            ASTNode *linksHead = ASTNode::New();
            ASTNode *linkEnd = linksHead;

            linksHead->init(Kind_Linker, Kind_SourceRoot);
            linkEnd->setLeft(res.unwrap());

            while (!pos.isEnded()) {
                if (pos.get().getType() == Lex_None) {
                    ++pos;
                    continue;
                }
                res = p_FuncDecl(pos);
                if (!res.hasValue()) {
                    res = p_VarDefStmt(pos);
                    if (!res.hasValue()) {
                        error(pos, "Undefined root sequence");
                        head = nullptr;
                        return;
                    }
                }

                ASTNode *newLinkNode = ASTNode::New();
                newLinkNode->init(Kind_Linker, Kind_SourceRoot);
                newLinkNode->setLeft(res.unwrap());

                linkEnd->setRight(newLinkNode);
                linkEnd = newLinkNode;
            }
            head = linksHead;
        }

        void dumpTree(FILE *file) {
            fprintf(file, "digraph structure {\n");
            recursiveDump(file, head);
            fprintf(file, "}\n");
        }

        void dumpParseErrorStack(const char *inputFileName) {
            ASTError *storage = errorStack.getStorage();
            if (errorStack.isEmpty()) {
                printf("No errors detected\n");
            } else {
                printf("\n");
                for (size_t i = 0; i < errorStack.getSize(); i++) {
                    printf("%s:%zu:%zu: error: %s at %s:%zu:%zu\n", inputFileName,
                           storage[i].errorIt.getLine() + 1, storage[i].errorIt.getCol(),
                           storage[i].errorMsg,
                           lexemeTypeToString(storage[i].errorIt.getType()),
                           storage[i].errorIt.getLine() + 1, storage[i].errorIt.getCol());
                }
            }
        }

        ClassicStack<ASTError> &getErrorStack() {
            return errorStack;
        };

        [[nodiscard]] bool hasError() const {
            return !errorStack.isEmpty();
        }

        [[nodiscard]] ASTNode *getHead() const {
            return head;
        }
    };
}

#endif //NGG_AST_H
