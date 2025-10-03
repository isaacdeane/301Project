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

#include <exception>  
#include <stdexcept> 

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

            // Detect and store ".asciiz"
            // Expecting outputs like { "greeting": 0, "arr": 56, "val1": 64 }
            if (!in_text && str.find(".asciiz") != std::string::npos) {
                size_t asz   = str.find(".asciiz"); // Look for the start of .asciiz
                size_t colon = str.find(':');     // Look for the colon (label)
                if (colon != std::string::npos && colon < asz) {
                    std::string label = rtrim(ltrim(str.substr(0, colon)));
                    if (!label.empty()) {
                        static_memory[label] = static_mem_body;
                    }
                }
                size_t q1 = str.find('"', asz);   // Look for the first "
                size_t q2 = (q1 == std::string::npos) ? std::string::npos : str.rfind('"');   // Look for the second "
                std::string raw = str.substr(q1 + 1, q2 - q1 - 1); 
                int n = (int)raw.size();
                // 4 bytes/char + 4 bytes for terminating '\0'
                static_mem_body += 4 * (n + 1);
                instructions.push_back(str);
                continue;           
            }

            std::vector<std::string> parts = split(str, WHITESPACE + ",()");

            // Detect and store label lines (like "main:", "loop:")
            if (parts.size() < 2 && !parts.empty() && parts[0].back() == ':') {
                std::string label = parts[0];
                label.pop_back();
                pending_labels.push_back(label); // delay label binding
                instructions.push_back(str);
                continue;
            }

            instructions.push_back(str);

            // Only increment when in .data section, the line is not empty, starts with a label (xxx:),
            // the second element is ".word".
            // Expecting output like static_memory = {"val1" : 0, "arr" : 4, "val1" : 16}
            if (!in_text && parts.size() >= 2 && !parts[0].empty() && parts[0].back() == ':' && parts[1] == ".word") {
                std::string label = parts[0];
                label.pop_back();  // delete ':'
                for (std::string l : { label }) {
                    static_memory[l] = static_mem_body;
                }
                pending_labels.clear();
                int cnt = (int)parts.size() - 2;        // number of elements after .word
                static_mem_body += 4 * cnt; 
            }     
                    
            // Only increment line_num when in .text section, the line is not empty, 
            // not a directive (.globl/.data/.text), not a label (xxx:), 
            // and is a valid MIPS instruction (like addi, lw, beq).
            // Expecting output like offsets = {"main" : 0, "loop" : 1, "end" : 3}
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
        // Deal with .asciiz
        size_t asz = inst.find(".asciiz");
        if (asz != std::string::npos) {
            size_t colon = inst.find(':');
            if (colon != std::string::npos && colon < asz) {
                std::string label = rtrim(ltrim(inst.substr(0, colon)));
                auto it = static_memory.find(label);
            }
            size_t q1 = inst.find('"', asz);
            size_t q2 = (q1 == std::string::npos) ? std::string::npos: inst.find('"', q1 + 1);
            if (q1 == std::string::npos || q2 == std::string::npos || q2 <= q1) {
                std::cerr << "[Phase2] .asciiz missing quotes: " << inst << "\n";
                exit(1);
            }
            std::string raw = inst.substr(q1 + 1, q2 - q1 - 1);
            for (unsigned char ch : raw) {
                write_binary(static_cast<int>(ch), static_outfile); 
                memory_address += 4;
            }
            write_binary(0, static_outfile);
            memory_address += 4;
            continue; 
        }

        // Deal with .word
        std::vector<std::string> terms = split(inst, WHITESPACE+",()");
        
        if (terms.size() < 2) continue;

        std::string label = terms[0];
        if (label.back() == ':') {
            label.pop_back();
        }

        if (terms[1] != ".word") continue;

        // Loop through all values after ".word"
        for (int i = 2; i < terms.size(); ++i) {
            int value;

            // If the term is a known label, use its offset
            try {
                if (offsets.find(terms[i]) != offsets.end()) {
                value = offsets.at(terms[i]) * 4;
                } else {
                value = stoi(terms[i]);  // Not a label, regard it as an element
                }
            } catch (const std::exception& e) {
                std::cerr << "[Phase2] invalid .word operand: '" << terms[i]
                        << "' in line: " << inst << "\n";
                exit(1);
            }

            write_binary(value, static_outfile);
            memory_address += 4;
        }
    }


    /** Phase 3
     * Process all instructions, output to instruction memory file
     * TODO: Almost all of this, it only works for adds
     * TODO: perhaps just use "result" instead of calling the function again, would save on runtime
     * terms[] goes in order of the instruction
     * ex^: for an instruction add rd, rs, rt
     * rd is terms[1], rs is terms[2], rt is terms[3] (for add instruction, not all of them)
     * this is the expected order for r-types: int encode_Rtype(int opcode, int rs, int rt, int rd, int shftamt, int funccode)
     * TODO: FIX  beq, and bne
     */
     int i = 0; // counter that is used for finding offset in beq and bne instructions
     for(std::string inst : instructions) {
        std::vector<std::string> terms = split(inst, WHITESPACE+",()");
        std::string inst_type = terms[0];
        std::cout << i << std::endl;
        if (inst_type == "add") { // R-TYPES: encode_Rtype(int opcode, int rs, int rt, int rd, int shftamt, int funccode)
            int result = encode_Rtype(0,registers[terms[2]], registers[terms[3]], registers[terms[1]], 0, 32);
            write_binary(encode_Rtype(0,registers[terms[2]], registers[terms[3]], registers[terms[1]], 0, 32),inst_outfile);
            i++;
        } else if (inst_type == "sub") {
            int result = encode_Rtype(0,registers[terms[2]], registers[terms[3]], registers[terms[1]], 0, 34);
            write_binary(encode_Rtype(0,registers[terms[2]], registers[terms[3]], registers[terms[1]], 0, 34),inst_outfile);
            i++;
        } else if (inst_type == "sll") {
            int shamt = std::stoi(terms[3]);
            int result = encode_Rtype(0,0, registers[terms[2]], registers[terms[1]], shamt, 0);
            write_binary(encode_Rtype(0,0, registers[terms[2]], registers[terms[1]], shamt, 0), inst_outfile);
            i++;
        } else if (inst_type == "srl") {
            int shamt = std::stoi(terms[3]);
            int result = encode_Rtype(0,0, registers[terms[2]], registers[terms[1]], shamt, 2);
            write_binary(encode_Rtype(0,0, registers[terms[2]], registers[terms[1]], shamt, 2), inst_outfile);
            i++;
        } else if (inst_type == "mult") {
            int result = encode_Rtype(0, registers[terms[1]], registers[terms[2]], 0, 0, 24);
            write_binary(encode_Rtype(0, registers[terms[1]], registers[terms[2]], 0, 0, 24), inst_outfile);
            i++;
        } else if (inst_type == "div") {
            int result = encode_Rtype(0, registers[terms[1]], registers[terms[2]], 0, 0, 26);
            write_binary(encode_Rtype(0, registers[terms[1]], registers[terms[2]], 0, 0, 26), inst_outfile);
            i++;
        } else if (inst_type == "mflo") {
            int result = encode_Rtype(0, 0, 0, registers[terms[1]], 0, 18);
            write_binary(encode_Rtype(0, 0, 0, registers[terms[1]], 0, 18), inst_outfile);
            i++;
        } else if (inst_type == "mfhi") {
            int result = encode_Rtype(0, 0, 0, registers[terms[1]], 0, 16);
            write_binary(encode_Rtype(0, 0, 0, registers[terms[1]], 0, 16), inst_outfile);
            i++;
        } else if (inst_type == "slt") { 
            int result = encode_Rtype(0, registers[terms[2]], registers[terms[3]], registers[terms[1]], 0, 42);
            write_binary(encode_Rtype(0, registers[terms[2]], registers[terms[3]], registers[terms[1]], 0, 42), inst_outfile);
            i++;
        } else if (inst_type == "jr") { 
            int result = encode_Rtype(0, registers[terms[1]], 0, 0, 0, 8);
            write_binary(encode_Rtype(0, registers[terms[1]], 0, 0, 0, 8), inst_outfile);
            i++;
        } else if (inst_type == "jalr") {
            if(terms.size() == 3) { // two register jalr 
                int result = encode_Rtype(0, registers[terms[1]], 0, registers[terms[2]], 0, 9);
                write_binary(encode_Rtype(0, registers[terms[1]], 0, registers[terms[2]], 0, 9), inst_outfile);
                i++;
            }
            else if(terms.size() == 2) { // one register jalr
                int result = encode_Rtype(0, registers[terms[1]], 0, 31, 0, 9);
                write_binary(encode_Rtype(0, registers[terms[1]], 0, 31, 0, 9), inst_outfile);
                i++;
            } 
            else {
                std::cerr << "something is wrong with jalr handling" << std::endl;
                exit(1);
            }
        } else if (inst_type == "syscall") {
            int result = encode_Rtype(0, 0, 0, 26, 0, 12);
            write_binary(encode_Rtype(0, 0, 0, 26, 0, 12), inst_outfile);
            i++;
        } else if (inst_type == "addi") { // I-TYPES: encode_Itype(int opcode, int rs, int rt, int immediate)
            int immediate = std::stoi(terms[3]);
            int result = encode_Itype(8, registers[terms[2]], registers[terms[1]], immediate); 
            write_binary(encode_Itype(8, registers[terms[2]], registers[terms[1]], immediate), inst_outfile);
            i++;
        } else if (inst_type == "lw") {
            int offset = std::stoi(terms[2]);
            int result = encode_Itype(35, registers[terms[3]], registers[terms[1]], offset);
            write_binary(encode_Itype(35, registers[terms[3]], registers[terms[1]], offset), inst_outfile);
            i++;
        } else if (inst_type == "sw") {
            int offset = std::stoi(terms[2]);
            int result = encode_Itype(43, registers[terms[3]], registers[terms[1]], offset);
            write_binary(encode_Itype(43, registers[terms[3]], registers[terms[1]], offset), inst_outfile);
            i++;
        } else if (inst_type == "beq") {
            //std::cout << (offsets[terms[3]]) << std::endl;
            int offset = offsets[terms[3]] - (i+1);
            int result = encode_Itype(4, registers[terms[1]], registers[terms[2]], offset);
            write_binary(encode_Itype(4, registers[terms[1]], registers[terms[2]], offset), inst_outfile);
            i++;
        } else if (inst_type == "bne") {
            //std::cout << offsets[terms[3]] << std::endl;
            int offset = offsets[terms[3]] - (i+1);
            int result = encode_Itype(5, registers[terms[1]], registers[terms[2]], offset);
            write_binary(encode_Itype(5, registers[terms[1]], registers[terms[2]], offset), inst_outfile);
            i++;
        } else if (inst_type == "j") { //J-TYPES: int encode_Jtype(int opcode, int address)
            int result = encode_Jtype(2, offsets[terms[1]]);
            write_binary(encode_Jtype(2, offsets[terms[1]]), inst_outfile);
            i++;
        } else if (inst_type == "jal") {
            int result = encode_Jtype(3, offsets[terms[1]]);
            write_binary(encode_Jtype(3, offsets[terms[1]]), inst_outfile);       
            i++;     
        } else if (inst_type == "la") { //PSEUDO-INSTRUCTION (using I-type)
            int address = static_memory[terms[2]];
            int result = encode_Itype(8, registers["$zero"], registers[terms[1]], address);
            write_binary(encode_Itype(8, registers["$zero"], registers[terms[1]], address), inst_outfile);
            i++;

        // Extra MIPS Instructions
        } else if (inst_type == "and") {
            int result = encode_Rtype(0,registers[terms[2]], registers[terms[3]], registers[terms[1]], 0, 36);
            write_binary(encode_Rtype(0,registers[terms[2]], registers[terms[3]], registers[terms[1]], 0, 36),inst_outfile);
            i++;
        } else if (inst_type == "or") {
            int result = encode_Rtype(0,registers[terms[2]], registers[terms[3]], registers[terms[1]], 0, 37);
            write_binary(encode_Rtype(0,registers[terms[2]], registers[terms[3]], registers[terms[1]], 0, 37),inst_outfile);
            i++;
        } else if (inst_type == "nor") {
            int result = encode_Rtype(0,registers[terms[2]], registers[terms[3]], registers[terms[1]], 0, 39);
            write_binary(encode_Rtype(0,registers[terms[2]], registers[terms[3]], registers[terms[1]], 0, 39),inst_outfile);
            i++;
        } else if (inst_type == "xor") {
            int result = encode_Rtype(0,registers[terms[2]], registers[terms[3]], registers[terms[1]], 0, 38);
            write_binary(encode_Rtype(0,registers[terms[2]], registers[terms[3]], registers[terms[1]], 0, 38),inst_outfile);
            i++;
        } else if (inst_type == "andi") {
            int result = encode_Itype(12,registers[terms[2]], registers[terms[1]], stoi(terms[3]));
            write_binary(encode_Itype(12,registers[terms[2]], registers[terms[1]], stoi(terms[3])),inst_outfile);
            i++;
        } else if (inst_type == "ori") {
            int result = encode_Itype(13,registers[terms[2]], registers[terms[1]], stoi(terms[3]));
            write_binary(encode_Itype(13,registers[terms[2]], registers[terms[1]], stoi(terms[3])),inst_outfile);
            i++;
        } else if (inst_type == "xori") {
            int result = encode_Itype(14,registers[terms[2]], registers[terms[1]], stoi(terms[3]));
            write_binary(encode_Itype(14,registers[terms[2]], registers[terms[1]], stoi(terms[3])),inst_outfile);
            i++;
        } else if (inst_type == "lui") {
            int result = encode_Itype(15,0, registers[terms[1]], stoi(terms[3]));
            write_binary(encode_Itype(15,0, registers[terms[1]], stoi(terms[3])),inst_outfile);
            i++;
        } else if (inst_type == "mov") {
            int result = encode_Rtype(0, registers[terms[2]], 0, registers[terms[1]], 0, 32);
            write_binary(result, inst_outfile);
            i++;
        } else if (inst_type == "li") {
            int imm = stoi(terms[2]);  // Convert immediate to integer
            if (imm >= -32768 && imm <= 32767) {  // Fits in 16-bit immediate
                int result = encode_Itype(8, 0, registers[terms[1]], imm);  // addi $rd, $zero, imm
                write_binary(result, inst_outfile);
                i++;
            } else {  // If immediate is larger than 16 bits, use lui + ori
                int upper = (imm >> 16) & 0xFFFF;
                int lower = imm & 0xFFFF;
                int lui_inst = encode_Itype(15, 0, registers[terms[1]], upper);  // lui $rd, upper
                int ori_inst = encode_Itype(13, registers[terms[1]], registers[terms[1]], lower);  // ori $rd, $rd, lower
                write_binary(lui_inst, inst_outfile);
                write_binary(ori_inst, inst_outfile);
                i++;
            }
        } else if (inst_type == "sge") {
            int result1 = encode_Rtype(0, registers[terms[1]], registers[terms[2]], registers["$at"], 0, 42);
            int result2 = encode_Itype(14, registers["$at"], registers["$at"], 1); 
            write_binary(result1, inst_outfile);
            write_binary(result2, inst_outfile);
            i++;
        }
        else if (inst_type == "sgt") {
            int result = encode_Rtype(0, registers[terms[2]], registers[terms[1]], registers[terms[1]], 0, 42);
            write_binary(result, inst_outfile);
            i++;
        }else if (inst_type == "sle") {
            int result1 = encode_Rtype(0, registers[terms[2]], registers[terms[1]], registers["$at"], 0, 42);
            int result2 = encode_Itype(14, registers["$at"], registers["$at"], 1);
            write_binary(result1, inst_outfile);
            write_binary(result2, inst_outfile);
            i++;
        }else if (inst_type == "seq") {
            int result1 = encode_Rtype(0, registers[terms[1]], registers[terms[2]], registers["$at"], 0, 38);
            int result2 = encode_Itype(10, registers["$at"], registers["$at"], 1);
            write_binary(result1, inst_outfile);
            write_binary(result2, inst_outfile);
            i++;
        }else if (inst_type == "sne") {
            int result1 = encode_Rtype(0, registers[terms[1]], registers[terms[2]], registers["$at"], 0, 38);
            int result2 = encode_Rtype(0, 0, registers["$at"], registers["$at"], 0, 43);
            write_binary(result1, inst_outfile);
            write_binary(result2, inst_outfile);
            i++;
        } else if (inst_type == "blt") {
        // slt $at, rs, rt
        write_binary(encode_Rtype(0, registers[terms[1]], registers[terms[2]], registers["$at"], 0, 42), inst_outfile);
        // bne $at, $zero, label
        int off = offsets[terms[3]] - (i + 1);
        write_binary(encode_Itype(5, registers["$at"], registers["$zero"], off), inst_outfile);
        i++;
        } else if (inst_type == "bge") {
        // slt $at, rs, rt
        write_binary(encode_Rtype(0, registers[terms[1]], registers[terms[2]], registers["$at"], 0, 42), inst_outfile);
        // beq $at, $zero, label
        int off = offsets[terms[3]] - (i + 1);
        write_binary(encode_Itype(4, registers["$at"], registers["$zero"], off), inst_outfile);
        i++;
        } else if (inst_type == "bgt") {
        // slt $at, rt, rs
        write_binary(encode_Rtype(0, registers[terms[2]], registers[terms[1]], registers["$at"], 0, 42), inst_outfile);
        // bne $at, $zero, label
        int off = offsets[terms[3]] - (i + 1);
        write_binary(encode_Itype(5, registers["$at"], registers["$zero"], off), inst_outfile);
        i++;
        } else if (inst_type == "ble") {
        // slt $at, rt, rs
        write_binary(encode_Rtype(0, registers[terms[2]], registers[terms[1]], registers["$at"], 0, 42), inst_outfile);
        // beq $at, $zero, label
        int off = offsets[terms[3]] - (i + 1);
        write_binary(encode_Itype(4, registers["$at"], registers["$zero"], off), inst_outfile);
        i++;
        } 
    } //increment counting index
    }
#endif