//
// Created by Александр Дремов on 09.04.2021.
//
#include <MachOBuilder.h>
#include <cstdio>
#include "LexicalAnalysis/LexParser.h"
#include "Compiler/NGGCompiler.h"
#include "Helpers/ParamsParser.h"
#include "core/bgen/FastStackController.h"

int main(const int argc, const char *argv[]) {
    CLParams params = {};
    params.init();
    bool result = params.parseArgs(argc, argv);
    if (!result) {
        printf("error: nggc: Error processing command line arguments\n");
        return EXIT_FAILURE;
    }

    NGGC::NGGCompiler compiler = {};
    compiler.init();

    if (!compiler.loadFile(params.inputFileName)){
        printf("error: nggc: can't load %s file\n", params.inputFileName);
        compiler.dest();
        params.dest();
        return EXIT_FAILURE;
    }
    if (params.lexemes)
        compiler.printLexemes(params.lexFileName);

    if (!compiler.isParseSuccessful()) {
        compiler.dumpErrorStack(params.inputFileRealName);
        printf("error: ngg: Unsuccessful parse\n");
        compiler.dest();
        params.dest();
        return EXIT_FAILURE;
    }

    if (params.graph)
        compiler.dumpGraph();

    if (params.dumpGraph)
        compiler.dumpTree();

    compiler.compile();

    if (!compiler.isCompileSuccessful()) {
        compiler.dumpCompileErrorStack(params.inputFileRealName);
        printf("error: ngg: Unsuccessful parse\n");
        compiler.dest();
        params.dest();
        return EXIT_FAILURE;
    }
    compiler.dest();
    params.dest();
    return 0;
}