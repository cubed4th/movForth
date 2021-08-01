
#include <set>
#include "Structures.h"

#ifndef MOVFORTH_WORD_H
#define MOVFORTH_WORD_H

namespace sym{

    struct Effects{
        int num_popped = 0;
        int num_pushed = (0);

        bool consume_token = false; // ONLY for single token consumers, leave out "

        unsigned int compiled_slots = 0; // TODO still unsure how to handle memory

        enum interpret_state{NONE, TOCOMPILE, TOINTERPRET} interpret_state = NONE;

        bool define_new_word = false;

        // mapping of index for identical registers
        std::vector<std::pair<int, int>> out_in_pairs;

        void acquire_side_effects(Effects& other){
            // pop and push handled elsewhere

            if(other.consume_token) consume_token = true;

            compiled_slots += other.compiled_slots;

            if(other.interpret_state != NONE)
                interpret_state = other.interpret_state;

            if(other.define_new_word) define_new_word = true;
        }

    };

    struct BranchInstruction;
    struct BranchIfInstruction;
    struct Instruction{
        NodeList pop_nodes;
        NodeList push_nodes;

        Wordptr linked_word;
        Data data = Data(nullptr); // acquired from next in token list

        BranchInstruction* as_branch();
        BranchIfInstruction* as_branchif();
        explicit Instruction(Wordptr linked_word) : linked_word(linked_word) {}
    };

    struct BasicBlockEntry {
        Instruction *target;
    };

    struct BranchInstruction : Instruction {
        BasicBlockEntry *jump_to = nullptr;
        explicit BranchInstruction(Wordptr linked_word) : Instruction(linked_word){}
    };
    struct BranchIfInstruction : Instruction {
        BasicBlockEntry *jump_to_close = nullptr;
        BasicBlockEntry *jump_to_far = nullptr;
        BranchIfInstruction(Wordptr linked_word) : Instruction(linked_word){}
    };

    class Word {
    public:
        std::string name;
        Effects effects;

        // components for graph
        //std::vector<Wordptr> definition;
        NodeList my_graphs_outputs;
        NodeList my_graphs_inputs;

        void definition_to_string();

        std::set<BasicBlockEntry*> BasicBlockEntries;

        std::vector<Instruction*> instructions;

        static Wordptr nop;
    };
}

#endif //MOVFORTH_WORD_H
