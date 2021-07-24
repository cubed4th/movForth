#include <algorithm>
#include <iomanip>
#include "../headers/Interpreter.h"

using namespace mfc;

Interpreter::Interpreter() : input("../boot.fs"){
    init_words();

    std::string token;
    while (!(token = input.next_token()).empty()){
        Wordptr Wordptr = find(token);
        if(!Wordptr)
            Wordptr = wordGenerator.get(token);

        //std::cout << "[interpreter] " << token << std::endl;

        if(Wordptr == nullptr){
            // might be a number
            try{
                int num = stoi(token);

                if(immediate)
                    stack.push(num);
                else {
                    auto forth_word =
                            try_cast<ForthWord>(dictionary.back());

                    if(forth_word){
                        forth_word->add(Data(find("literal")));
                        forth_word->add(Data(num));
                    }else{
                        std::cout <<
                            "attempted to compile LITERAL to a primitive word"
                        << std::endl;
                    }

                }

            } catch (...) {
                // not a number or word
                std::cerr <<
                    "word " << token <<
                    " not found and not number"
                << std::endl;
            }

        }else{

            // it's a word
            if(Wordptr->immediate || immediate) {
                auto dummy_ip = IP();
                Wordptr->execute(stack, dummy_ip);
            } else {
                auto forth_word =
                        try_cast<ForthWord>(dictionary.back());

                if(forth_word){
                    forth_word->add(Data(Wordptr));
                }else
                    std::cout <<
                        "attempted to compile xts to a primitive word"
                    << std::endl;
            }
        }

    }
}

Wordptr Interpreter::find(const std::string& name) {
    auto find_result = std::find_if(dictionary.begin(), dictionary.end(),
                                    [name](Wordptr other){return other->base_string() == name;});

    if(find_result == dictionary.end())
        // try primitives
        return wordGenerator.get(name);

    return *find_result;
}


void Interpreter::init_words(){

    wordGenerator.register_primitive("+", [](Stack &s, IP &i) {
        s.push(s.pop_number() + s.pop_number());
    });

    wordGenerator.register_primitive("-", [](Stack &s, IP &i) {
        auto one = s.pop_number();
        auto two = s.pop_number();
        s.push(two - one);

    });

    wordGenerator.register_primitive("*", [](Stack &s, IP &i) {
        s.push(s.pop_number() * s.pop_number());

    });

    wordGenerator.register_primitive("/", [](Stack &s, IP &i) {
        s.push(s.pop_number() / s.pop_number());
    });

    wordGenerator.register_primitive("swap", [](Stack &s, IP &i) {
        auto top = s.pop_number(), second = s.pop_number();
        s.push(top);
        s.push(second);
    });

    wordGenerator.register_primitive("dup", [](Stack &s, IP &i) {
        s.push(s.top());
    });

    wordGenerator.register_primitive("drop", [](Stack &s, IP &i) {
        s.pop_number();
    });

    wordGenerator.register_primitive(".", [](Stack &s, IP &i) {
        std::cout << s.pop_number() << std::endl;
    });

    wordGenerator.register_primitive(".S", [](Stack &s, IP &i) {
        std::cout << "Stack size: " << s.size() << " ";
        s.for_each([](Data thing) {
            std::cout << data_to_string(thing) << " ";
        });
        std::cout << "<-top";
        std::cout << std::endl;
    });

    wordGenerator.register_primitive("'", [&](Stack &s, IP &i) {
        std::string next_token = input.next_token();
        auto cfa = find(next_token);
        if (cfa != nullptr)
            s.push(cfa);
    });

    wordGenerator.register_primitive(",", [&](Stack &s, IP &i) {
        if (auto last_word = try_cast<ForthWord>(dictionary.back())) {
            last_word->add(s.pop());
        } else {
            std::cout << "attempted to compile to a primitive" << std::endl;
        }
    });

    wordGenerator.register_primitive("see", [&](Stack &s, IP &i) {
        std::cout << std::endl << "\tSo you want to see?" << std::endl;

        for (Wordptr word_pointer : dictionary) {
            std::cout << std::setfill(' ') << std::setw(15) <<
                      word_pointer->to_string() + "  ";
            std::cout << ((word_pointer->immediate) ? "IMM  " : "     ");

            if (auto forth_word = try_cast<ForthWord>(word_pointer))
                forth_word->definition_to_string();
            std::cout << std::endl;
        }
        std::cout << std::endl;
    });

    wordGenerator.register_lambda_word("[", true, [&](Stack &s, IP &i) {
        immediate = true;
    });

    wordGenerator.register_primitive("]", [&](Stack &s, IP &i) {
        immediate = false;
    });

    wordGenerator.register_lambda_word("immediate", true, [&](Stack &s, IP &i) {
        dictionary.back()->immediate = true;
    });

    wordGenerator.register_primitive("@", [&](Stack &s, IP &i) {
        dictionary.back()->immediate = true;
    });

    wordGenerator.register_primitive("!", [&](Stack &s, IP &i) {
        int address = stack.pop_number();
        Data val = stack.pop();
        auto last_word = try_cast<ForthWord>(dictionary.back());
        if (!last_word)
            std::cout << "used ! for an address not in most recent word"
                      << std::endl;
        else {
            last_word->set(address, val);
        }
    });

    wordGenerator.register_primitive("branch", [&](Stack &s, IP &ip) {
        ip += 1;
        Data data = *(ip.me);
        try {
            ip += (data.as_num() - 2);
            // negative jumps go ++ more
            if (data.as_num() < 0) ip += 1;
        } catch (const std::exception &e) {
            std::cout << "fucker: " << e.what() << std::endl;
        }
    });

    wordGenerator.register_primitive("branchif", [&](Stack &s, IP &ip) {
        ip += 1;
        Data data = *(ip.me);

        if (s.pop_number() > 0) {
            std::cout << "BB jump" << std::endl;

            if (data.is_num()) {
                ip += data.as_num() + 1;
            } else
                std::cout << "fuck" << std::endl;
        } else {
            std::cout << "BB no jump" << std::endl;
            //ip += 1;
        }
    });

    wordGenerator.register_primitive("literal", [&](Stack &s, IP &ip) {
        ip += 1;
        Data data = *(ip.me);

        s.push(Data(data));
    });

    // Use HERE for relative computations only
    // Does not represent specific address
    wordGenerator.register_primitive("here", [&](Stack &s, IP &i) {
        auto last_word = try_cast<ForthWord>(dictionary.back());
        if (!last_word)
            std::cout << "Define some words before calling HERE" << std::endl;
        else
            s.push(last_word->definition_size());
    });

    wordGenerator.register_primitive("create", [&](Stack &s, IP &i) {
        std::string next_token = input.next_token();
        dictionary.push_back(new ForthWord(next_token, false));
    });


    auto colon_word = new ForthWord(":", false);
    colon_word->add(Data(find("create")));
    colon_word->add(Data(find("]")));
    dictionary.push_back(colon_word);

    auto exit_word = new ForthWord(";", true);
    exit_word->add(Data(find("[")));
    dictionary.push_back(exit_word);
}
