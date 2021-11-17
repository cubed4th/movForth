#include "../../headers/Interpretation/iWord.h"
#include "../../headers/Interpretation/iData.h"
#include "../../headers/Interpretation/Interpreter.h"

using namespace mov;

iWord::iWord(std::string name, bool immediate, bool stateful)
        : _name(std::move(name)), immediate(immediate), stateful(stateful) {}

iWord::iWord(std::string name, primitive_words id, bool immediate, bool stateful)
    : _name(std::move(name)), id(id), immediate(immediate), stateful(stateful) {}

std::string iWord::name() {
    return _name;
}
bool iWord::branchy() {
    return id == primitive_words::BRANCH || id == primitive_words::BRANCHIF;
}


ForthWord::ForthWord(std::string name, bool immediate)
    : iWord(std::move(name), immediate, false) {}

void ForthWord::execute(IP &ip, Interpreter &interpreter) {
    for(auto it = definition.begin(); it != definition.end(); it++) {

        // println("       [exec] ", (it->is_word()?it->as_word()->name:std::to_string(it->as_number())), " ");

        iData current_cell = *it;
        if(current_cell.is_word())
            current_cell.as_word()->execute(it, interpreter);
        else
            println("Too many numbers in definition, no LITERAL to consume them");
    }
}
void ForthWord::add(iData data){
    definition.push_back(data);
}

void ForthWord::definition_to_string() {
    for(auto thing : definition)
        print(thing.to_string(), " ");
}

void ForthWord::set(int index, iData value) {
    auto front = definition.begin();
    std::advance(front, index);
    *front = value;
}

Primitive::Primitive(std::string name, primitive_words id, bool immediate, std::function<void(IP&, Interpreter&)> action, bool stateful)
    : iWord(std::move(name), id, immediate, stateful), action(std::move(action)) {}

void Primitive::execute(IP &ip, Interpreter &interpreter) {
    action(ip, interpreter);
}

ToLocal::ToLocal()
        : Primitive("toLocal", primitive_words::TOLOCAL, true, [&](IP &ip, Interpreter &interpreter) {
            std::string next_token = interpreter.input.next_token();
            localname = next_token;
        }, false)
{}


FromLocal::FromLocal()
        : Primitive("toLocal", primitive_words::TOLOCAL, true, [&](IP &ip, Interpreter &interpreter) {
            std::string next_token = interpreter.input.next_token();
            localname = next_token;
        }, false)
{}
