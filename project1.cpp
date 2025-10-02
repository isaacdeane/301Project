#ifndef __PROJECT1_CPP__
#define __PROJECT1_CPP__

#include "project1.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <iostream>
#include <sstream>
#include <fstream>
#include <unordered_set>

// Define a set of valid MIPS operation mnemonics for instruction validation
const std::unordered_set<std::string> MIPS_OPS = {
    "add", "sub", "sll", "srl", "mult", "div", "mflo", "mfhi", "slt",
    "addi", "la", "lw", "sw", "beq", "bne",
    "j", "jal", "jr", "jalr",
    "syscall",
    "and", "nor", "or", "xor", "andi", "ori", "xori", "lui",
    "mov", "li", "sge", "sgt", "sle", "seq", "sne"
};

int main(int argc, char* argv[]) {
    if (argc < 4) // Checks that at least 3 arguments are given in command line
    {
        std::cerr << "Expected Usage:\n ./assemble infile1.asm infile2.asm ... infilek.asm staticmem_outfile.bin instructions_outfile.bin\n" << std::endl;
        exit(1);
    }
    //Prepare output files
    std::ofstream inst_outfile, static_outfile;
    static_outfile.open(argv[argc - 2], std::ios::binary);
    inst_outfile.open(argv[argc - 1], std::ios::binary);
    std::vector<std::string> instructions;

    /**
     * Phase 1:
     * Read all instructions, clean them of comments and whitespace DONE
     * TODO: Determine the numbers for all static memory labels
     * (measured in bytes starting at 0)
     * TODO: Determine the line numbers of all instruction line labels
     * (measured in instructions) starting at 0
    */

    //For each input file:

    int static_mem_body = 0; // Tracker for static mem body
    int line_num = 0; // Tracker for the current instruction

    std::unordered_map<std::string, int> offsets; // Map: instruction line number
    std::unordered_map<std::string, int> static_memory;  // Map: static memory address

    bool in_text = false;  // Section flag: true = .text, false = .data
    std::vector<std::string> pending_labels;  // Labels waiting to bind to next instruction

    for (int i = 1; i < argc - 2; i++) {
        std::ifstream infile(argv[i]); //  open the input file for reading
        if (!infile) { // if file can't be opened, need to let the user know
            std::cerr << "Error: could not open file: " << argv[i] << std::endl;
            exit(1);
        }

        std::string str;
        while (getline(infile, str)){ //Read a line from the file
            str = clean(str); // remove comments, leading and trailing whitespace
            if (str == "") { //Ignore empty lines
                continue;
            }

            std::vector<std::string> parts = split(str, WHITESPACE + ",()");

            // Switch to .text section
            if (str == ".text") {
                in_text = true;
                instructions.push_back(str);
                continue;
            }
            // Switch to .data section
            if (str == ".data") {
                in_text = false;
                instructions.push_back(str);
                continue;
            }

            // Detect and store label lines (like "main:", "loop:")
            if (parts.size() < 2 && !parts.empty() && parts[0].back() == ':') {
                std::string label = parts[0];
                label.pop_back();
                pending_labels.push_back(label); // delay label binding
                instructions.push_back(str);
                continue;
            }

            instructions.push_back(str);

            // Only increment line_num when in .text section, the line is not empty, 
            // not a directive (.globl/.data/.text), not a label (xxx:), 
            // and is a valid MIPS instruction (like addi, lw, beq).
            if (in_text && (parts.size() >= 1) && str.at(0) != '.' && parts[0].back() != ':' && MIPS_OPS.count(parts[0])) {
                for (auto label : pending_labels) {
                    offsets[label] = line_num;
                }
                pending_labels.clear();
                line_num++;
            }
        }
        infile.close();
    }



    /** Phase 2
     * Process all static memory, output to static memory file
     * TODO: All of this
     */

     int memory_address = 0; // Address within static memory (starts at 0)
    for (std::string inst : instructions) {
        std::vector<std::string> terms = split(inst, WHITESPACE+",()");
        
        // Check if it's a static memory declaration like "a: .word 1"
        if (terms.size() >= 2 && terms[1] == ".word") {
            std::string label = terms[0];
            if (label.back() == ':') label.pop_back();

            static_memory[label] = memory_address; // Assign memory address to label

            // Loop through all values after ".word"
            for (int i = 2; i < terms.size(); ++i) {
                int value;

                // If the term is a known label, use its offset
                if (offsets.find(terms[i]) != offsets.end()) {
                    value = offsets.at(terms[i]) * 4;
                } else {
                    value = stoi(terms[i]);
                }

                write_binary(value, static_outfile);
                memory_address += 4;
            }
        }
    }


    /** Phase 3
     * Process all instructions, output to instruction memory file
     * TODO: Almost all of this, it only works for adds
     */
    for(std::string inst : instructions) {
        std::vector<std::string> terms = split(inst, WHITESPACE+",()");
        std::string inst_type = terms[0];
        if (inst_type == "add") {
            int result = encode_Rtype(0,registers[terms[2]], registers[terms[3]], registers[terms[1]], 0, 32);
            write_binary(encode_Rtype(0,registers[terms[2]], registers[terms[3]], registers[terms[1]], 0, 32),inst_outfile);
        }
    }
}

#endif