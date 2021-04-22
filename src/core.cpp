//
// Created by Александр Дремов on 09.04.2021.
//
#include <cstdio>
#include "compiler/NGGCompiler.h"
#include "helpers/ParamsParser.h"

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

    if (params.listing) {
        FILE* listing = fopen(params.listingFileName,"wb");
        fputs(compiler.getListing().getStorage(), listing);
        fclose(listing);
    }

    if (!compiler.isCompileSuccessful()) {
        compiler.dumpCompileErrorStack(params.inputFileRealName);
        printf("error: ngg: Unsuccessful parse\n");
        compiler.dest();
        params.dest();
        return EXIT_FAILURE;
    }

    compiler.genObject(params.objOnly? params.outputFileName: params.outputObjName);
    if (!params.objOnly){
        StrContainer command = {};
        command.init("clang ");
        command.sEndPrintf("%s  /usr/local/lib/nggstdlib.o  -o %s ", params.outputObjName, params.outputFileName);
        command.sEndPrintf(" && chmod a+x %s", params.outputFileName);
        if (!params.keepObj) {
            command.sEndPrintf(" && rm %s", params.outputObjName);
        }
        system(command.begin());
        command.dest();
    }

    compiler.dest();
    params.dest();
    return 0;
}