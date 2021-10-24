#include "../../headers/Symbolic/Pass.h"

using namespace mov;


void Analysis::propagate_stack(NodeList &stack,
                     Instruction *instruction,
                     NodeList &params,
                     RegisterGen &register_gen) {

    const auto effects = instruction->linked_word->effects;

    // if the stack does not have at least (effects_without_push_pop.num_popped) items for us,
    // then we will create param nodes until there are enough items
    unsigned int nodes_from_stack = std::min(effects.num_popped, stack.size());
    // we must make this comparison in signed integers
    unsigned int nodes_from_params = std::max((int)effects.num_popped - (int)stack.size(), 0);

    // pop param nodes from stack to current instruction
    NodeList::move_top_elements(stack, instruction->pop_nodes,
                                (int) nodes_from_stack);

    while (nodes_from_params -- > 0)
        Node::link_bidirection(params.new_top(),
                   instruction->pop_nodes.new_top(),
                   register_gen.get_param());

    // make empty output nodes
    for (int i = 0; i < effects.num_pushed; i++)
        instruction->push_nodes.new_top();

    // link output and param nodes that represent the same data
    for (auto out_in_pair : effects.out_in_pairs)
    {
        auto pop_node = instruction->pop_nodes[out_in_pair.second];
        auto push_node = instruction->push_nodes[out_in_pair.first];
        Node::link(pop_node, push_node, pop_node->backward_edge_register);
    }

    // link output nodes to stack
    for (auto push_node : instruction->push_nodes)
        if(push_node->target != nullptr)
            Node::link_bidirection(push_node, stack.new_top(), push_node->backward_edge_register);
        else
            Node::link_bidirection(push_node, stack.new_top(), register_gen.get());

    d("pops:");
    for (auto node : instruction->pop_nodes)
        d(" ", node->backward_edge_register.to_string());
    dln();

    d("pushes:");
    for (auto thing : instruction->push_nodes)
        d(" ", thing->forward_edge_register.to_string());
    dln();
}


NodeList Analysis::basic_block_stack_graph(NodeList &running_stack, Block &bb, NodeList &params,
                                           RegisterGen register_gen) {

    dln();

    bb.inputs = running_stack;

    for (auto instruction : bb.instructions)
    {
        auto definee = instruction->linked_word;

        // update the bb's total Effects
        bb.effects_without_push_pop.acquire_side_effects_ignore_push_pop(definee->effects);

        // propagate the stack state
        dln();
        dln("[", definee->name, "]");
        Analysis::propagate_stack(running_stack, instruction, params, register_gen);

        dln();
        dln("[stack:]");
        for (auto thing : running_stack)
            dln( thing->backward_edge_register.to_string());
    }

    bb.outputs = running_stack;

    return running_stack;
}
