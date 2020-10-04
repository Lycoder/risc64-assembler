#pragma once

#pragma once


#include "instruction.hpp"
#include "lexer.hpp"

#include <iostream>
#include <regex>

namespace parser {
    namespace detail {
        using namespace risc64;

        void parse_instruction_mnemonic(std::string m, mnemonic& i) {
            std::smatch sm;
            std::regex r("(addsp|subsp|halt|call|push|test|pop|ret|cmp|abs|add|sub|mul|div|lsp|slc|src|rlc|rrc|not|mod|and|xor|scl|fj|or|lr|sl|sr|rl|rr|i|d|l|s|b|j)(hw|dw|qw|b|w|d|q)?(nv|nz|nc|z|c|n|p)?([us]?)");
            if (std::regex_match(m, sm, r)) {
                i.id = sm[1];
                i.size = operand_size::w;
                i.sign = operand_sign::u;
                i.cond = condition::a;

                if (sm[2].length()) {
                    if (sm[2] == "q" || sm[2] == "qw") i.size = operand_size::q;
                    if (sm[2] == "d" || sm[2] == "dw") i.size = operand_size::d;
                    if (sm[2] == "b" || sm[2] == "hw") i.size = operand_size::b;
                }

                if (sm[3].length()) {
                    if (sm[3] == "p" ) i.cond = condition::p;
                    if (sm[3] == "n" ) i.cond = condition::n;
                    if (sm[3] == "z" ) i.cond = condition::z;
                    if (sm[3] == "c" ) i.cond = condition::c;
                    if (sm[3] == "nz") i.cond = condition::nz;
                    if (sm[3] == "nc") i.cond = condition::nc;
                }

                if (sm[4].length()) {
                    if (sm[4] == "s") i.sign = operand_sign::s;
                }
            }
        }

        void parse_operands(instruction::operand_array_t& operands) {
            lexer::token t = lexer::output.get();

            size_t c = 0;
            operand o;

            while (t.id != lexer::k_semicolon) {
                std::string& data = t.data;

                // If the operand is a constant
                if (std::isdigit(data[0]) || data[0] == '-' || data[0] == '+') {
                    o.const_value = std::stoull(data, nullptr, 0);
                    o.position = c++;
                    o.type = operand_type::c;
                    operands.push_back(o);
                }

                // If the operand is a register
                if (std::isalpha(data[0])) {
                    std::string rt = "", rn = "";
                    size_t i = 0;
                    while (!std::isdigit(data[i])) { rt += data[i++]; }
                    while ( std::isdigit(data[i])) { rn += data[i++]; }
                    if (rt == "r" || rt == "gpr") o.reg_type = register_type::gpr;
                    if (rt == "f" || rt == "fpr") o.reg_type = register_type::fpr;
                    o.reg_num = std::stoi(rn);
                    o.position = c++;
                    o.type = operand_type::r;
                    operands.push_back(o);
                }
                t = lexer::output.get();
            }
        }

        constexpr size_t parse_instruction_length(instruction& i) {
            switch (i.ec) {
                case encoding_class::t_register_all: return 5;
                case encoding_class::t_register_single_const:
                    switch (i.m.size) {
                        case operand_size::b: return 5;
                        case operand_size::w: return 6;
                        case operand_size::q: // [[fallthrough]]
                        case operand_size::d: return 8;
                    }
                case encoding_class::d_register_all: return 4;
                case encoding_class::d_register_single_const:
                    switch (i.m.size) {
                        case operand_size::b: return 4;
                        case operand_size::w: return 5;
                        case operand_size::q: // [[fallthrough]]
                        case operand_size::d: return 7;
                    }
                case encoding_class::s_register: return 3;
                case encoding_class::s_const:
                    switch (i.m.size) {
                        case operand_size::b: return 4;
                        case operand_size::w: return 5;
                        case operand_size::q: // [[fallthrough]]
                        case operand_size::d: return 7;
                    }
                case encoding_class::no_operand: return 2;
                default: return 0;
            }
        }

        void parse_encoding_class(instruction& i) {
            bool has_const = false;

            for (operand& o : i.operands) {
                if (o.type == operand_type::c) { has_const = true; break; }
            }

            switch (i.operands.size()) {
                case 0: i.ec = encoding_class::no_operand; break;
                case 1: {
                    if (has_const) { i.ec = encoding_class::s_const; break; }
                    i.ec = encoding_class::s_register;
                } break;
                case 2: {
                    if (has_const) { i.ec = encoding_class::d_register_single_const; break; }
                    i.ec = encoding_class::d_register_all;
                } break;
                case 3: {
                    if (has_const) { i.ec = encoding_class::t_register_single_const; break; }
                    i.ec = encoding_class::t_register_all;
                } break;
            }
        }
    }

    lexer::detail::stream<detail::instruction> output;

    void init() {
        output.set_policy(lexer::detail::stream_order::reverse);
    }

    void parse() {
        lexer::token t = lexer::output.get();
        while (!lexer::output.eof()) {
            if (t.id == lexer::k_instruction) {
                detail::instruction i;
                detail::parse_instruction_mnemonic(t.data, i.m);
                detail::parse_operands(i.operands);
                detail::parse_encoding_class(i);
                output.put(i);
            }
            t = lexer::output.get();
        }
    }
}

const std::string print_operand(parser::detail::operand& o) {
    std::stringstream ss;
    if (o.type == parser::detail::operand_type::r) {
        ss << "(register: type = " << (int)o.reg_type << ", num = " << o.reg_num << ", pos = " << o.position << ")";
    } else {
        ss << "(const: value = 0x" << std::hex << o.const_value << ", pos = " << o.position << ")";
    }
    return ss.str();
}

const std::string print_mnemonic(parser::detail::mnemonic& m) {
    std::stringstream ss;
    ss << "(id: " << m.id << ", size: " << (int)m.size << ", sign: " << (int)m.sign << ", cond: " << (int)m.cond << ")";
    return ss.str();
}

const std::string print_instruction(parser::detail::instruction& i) {
    std::stringstream ss;
    ss << "(instruction: {mnemonic: " << print_mnemonic(i.m) << ", encoding: " << (int)i.ec << ", operands: {" << (i.operands.size() ? "\n" : "");
    for (parser::detail::operand& o : i.operands) {
        ss << "\t" << print_operand(o) << std::endl;
    }
    ss << "}})";
    return ss.str();
}