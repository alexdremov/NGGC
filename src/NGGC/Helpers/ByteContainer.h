//
// Created by Александр Дремов on 09.12.2020.
//

#ifndef NGG_BYTECONTAINER_H
#define NGG_BYTECONTAINER_H
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cstdarg>

class ByteContainer {
    char*  storage;
    size_t len;
    size_t maxLen;

    void reallocate(size_t additional = 1) {
        if (len + additional >= maxLen){
            storage = static_cast<char*>(realloc(storage, maxLen + additional + 1));
            maxLen += additional;
        }
    }

public:
    void init(){
        storage = nullptr;
        len = 0;
        maxLen = 0;
    }

    void init(const char* line, size_t size){
        len = size;
        storage = (char*)calloc(size, 1);
        memcpy(storage, line, size);
        maxLen = len;
    }

    void dest(){
        if (storage) {
            free(storage);
            storage = nullptr;
        }
    }

    static ByteContainer* New(){
        auto* ob = static_cast<ByteContainer*>(calloc(1, sizeof(ByteContainer)));
        ob->init();
        return ob;
    }

    static void Delete(ByteContainer* ob){
        ob->dest();
        free(ob);
    }

    [[nodiscard]] const char* getStorage() const {
        return const_cast<const char*>(storage);
    }

    [[nodiscard]] char* getStorage() {
        return storage;
    }

    [[nodiscard]] size_t getLen() const {
        return len;
    }

    void append(char symbol) {
        reallocate(5);
        storage[len++] = symbol;
    }

    void append(const char* str, size_t size) {
        reallocate(size + 1);
        memcpy(storage + len, str, size);
        len += size;
    }

    void appendReversed(const char* str, size_t size) {
        for(int i = size - 1; i >=0; i--) {
            append(str[i]);
        }
    }

    [[nodiscard]] char* begin() const {
        return storage;
    }

    [[nodiscard]] char* end() const {
        return storage + len;
    }

    void readFromFile(FILE* file){
        fseek(file, 0, SEEK_END);
        size_t fsize = ftell(file);
        fseek(file, 0, SEEK_SET);

        reallocate(fsize + 10);
        fread(storage, 1, fsize, file);
    }

    bool isValid(){
        return storage != (char*) 0xBADF;
    }
};

#endif //NGG_STRCONTAINER_H
