//
// Created by Александр Дремов on 09.12.2020.
//

#ifndef NGG_OPTIONAL_H
#define NGG_OPTIONAL_H
#include <cstdlib>

template<typename T>
class Optional {
    T      _value;
    bool   _hasValue;
public:
    void init(T value){
        _value = value;
        _hasValue = true;
    }

    void init(){
        _hasValue = false;
    }

    void dest(){}

    static Optional* New(){
        auto* ob = static_cast<Optional*>(calloc(1, sizeof(Optional)));
        ob->init();
        return ob;
    }

    static void Delete(Optional* ob){
        ob->dest();
        free(ob);
    }

    bool hasValue(){
        return _hasValue;
    }

    T unwrap() {
        return _value;
    }

    T* operator->(){
        return &this->_value;
    }
};

#endif //NGG_OPTIONAL_H
