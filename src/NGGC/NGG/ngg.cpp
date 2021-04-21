#include "LexicalAnalysis/LexParser.h"
#include "Compiler/NGGCompiler.h"
#include "Helpers/ParamsParser.h"


int main(const int argc, const char *argv[]) {
    auto params = CLParams {};
    params.init();
    bool result = params.parseArgs(argc, argv);

    if (!result) {
        printf("error: ngg: Error processing command line arguments\n");
        return EXIT_FAILURE;
    }

    NGG::NGGCompiler compiler = {};
    compiler.init();

    compiler.loadFile(params.inputFileName);
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

    compiler.saveAsmSource("a.spus");

    NGG::NGGCompiler::genBytecode("a.spus", params);

    if (!params.keepCode) {
        system("rm a.spus");
    }
    compiler.dest();
    params.dest();
    return 0;
}
