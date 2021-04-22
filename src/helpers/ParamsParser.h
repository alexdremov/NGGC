//
// Created by Александр Дремов on 11.12.2020.
//

#ifndef NGG_PARAMSPARSER_H
#define NGG_PARAMSPARSER_H

#define MAXPATHLEN 1024

struct CLParams {
    const char* inputFileName;
    char* inputFileRealName;
    const char* outputFileName;
    const char* listingFileName;
    char* outputObjName;

    bool  keepObj;
    bool  objOnly;
    bool  listing;
    bool  graph;
    bool  lexemes;
    bool  dumpGraph;
    const char* lexFileName;

    static void printHelpData(){
        printf("NGGC help\n"
               "--input        <input file> input file to be compiled .ngg format (source)\n"
               "-o, --output   <output file> output file. a.out by default (mach-o executable)\n"
               "-h, --help     show this help message\n"
               "-c             object file only\n"
               "-C             keep object file\n"
               "--verbose      output debug information to the console\n"
               "--lex          <.lex file> file to dump lexemes\n"
               "--lst          <.lst file> file to dump detailed listing\n"
               "-g             generate AST graph\n"
               "-d             dump AST graph\n"
               "\n");
    }

    bool parseArgs(int argc, const char* argv[]) {
        if (argc <= 1){
            printHelpData();
            return false;
        }

        this->inputFileRealName = (char*)calloc(MAXPATHLEN, 1);

        for(int i = 1; i < argc; i++) {
            if (strcmp(argv[i], "--input") == 0) {
                if (i + 1 > argc) {
                    printf("error: nggc: No input file specified after --input\n");
                    return false;
                }
                this->inputFileName = *(argv + i + 1);
                realpath(this->inputFileName, this->inputFileRealName);
                i++;
            }else if (strcmp(argv[i], "--output") == 0 || strcmp(argv[i], "-o") == 0) {
                if (i + 1 > argc) {
                    printf("error: nggc: No output file specified after --output\n");
                    return false;
                }
                this->outputFileName = *(argv + i + 1);
                i++;
            }else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
                printHelpData();
            }else if (strcmp(argv[i], "-d") == 0) {
                this->dumpGraph = true;
            }else if (strcmp(argv[i], "-c") == 0) {
                this->objOnly = true;
            }else if (strcmp(argv[i], "-C") == 0) {
                this->keepObj = true;
            }else if (strcmp(argv[i], "-g") == 0) {
                this->graph = true;
            }else if (strcmp(argv[i], "--lex") == 0) {
                if (i + 1 > argc){
                    printf("error: nggc: No lex file specified after --lex\n");
                    return false;
                }
                lexemes = true;
                this->lexFileName = *(argv + i + 1);
                i++;
            }else if (strcmp(argv[i], "--lst") == 0) {
                if (i + 1 > argc){
                    printf("error: nggc: No lst file specified after --lst\n");
                    return false;
                }
                listing = true;
                this->listingFileName = *(argv + i + 1);
                i++;
            }else {
                if (this->inputFileName == nullptr){
                    this->inputFileName = *(argv + i);
                    realpath(this->inputFileName, this->inputFileRealName);
                }
            }
        }

        if (this->inputFileName == nullptr) {
            printf("error: nggc: No input file specified\n");
            return false;
        }

        if (this->outputFileName == nullptr) {
            StrContainer outFileName = {};
            outFileName.init();
            if (this->objOnly){
                outFileName.append(inputFileName);
                outFileName.append(".o");
                this->outputFileName = outFileName.begin();
            }else{
                outFileName.append("a.out");
                this->outputFileName = outFileName.begin();
            }
        }

        int outLen = strlen(outputFileName);
        outputObjName = (char*)calloc(outLen + 10, 1);
        strcpy(outputObjName, outputFileName);
        strcpy(outputObjName + outLen, ".o");

        return true;
    }

    void init() {
        inputFileName = nullptr;
        inputFileRealName = nullptr;
        outputFileName = nullptr;
        graph = false;
        lexemes = false;
        lexFileName = nullptr;
        dumpGraph = false;
    }

    void dest() {
        free(outputObjName);
        free(inputFileRealName);
    }
};

#endif //NGG_PARAMSPARSER_H
