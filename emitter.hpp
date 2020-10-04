#pragma once

#include <unordered_map>
#include <ostream>
#include <sstream>
#include <iomanip>
#include <memory>
#include <array>

#include "parser.hpp"

namespace emitter {
    namespace detail {
        template <class T> std::string hex(T value) {
            std::stringstream ss;
            ss << std::setfill('0') << std::setw(sizeof(T)*2) << std::hex << (unsigned long long)value;
            return ss.str();
        }

        enum condition {
            z = 0,	// z: zero/equal (0, 0b000)
            c,		// c: overflow/carry (1, 0b001)
            n,		// n: negative (2, 0b010)
            a,		// a: always (3, 0b011)
            nv,		// nv: never (4, 0b100)
            p,		// p/nn: not negative/positive (5, 0b101)
            nc,		// nc: not carry/overflow (6, 0b110)
            nz,		// nz: not zero/equal (7, 0b111)		 
        };

        // Instruction classes and subclasses
        enum instruction_type {
            alu = 0b00,                         // ALU (Arithmetic-Logic Unit) instruction class
            lsu = 0b01,                         // LSU (Load-Store Unit) instruction class
            bnj = 0b10,                         // BNJ (Branches aNd Jumps) instruction class
            sys = 0b11,                         // SYS (SYStem) instruction class
            t_operand_register_all 	= 0b00000, // <ins> <r0> <r1> <r2>
            t_operand_single_const 	= 0b00100, // <ins> <r0> <r1> <const>
            d_operand_register_all 	= 0b01000, // <ins> <r0> <r1>
            d_operand_single_const 	= 0b01100, // <ins> <r0> <const>
            d_operand_double_const 	= 0b10000, // <ins> <const> <const>
            s_operand_register 		= 0b10100, // <ins> <r0>
            s_operand_const			= 0b11000, // <ins> <const>
            no_operand				= 0b11100, // <ins>
        };

        // Operand sizes
        enum operand_size {
            hw = 0b00,         // 8-bit halfword (byte)
            w  = 0b01,         // 16-bit word
            dw = 0b10,         // 32-bit doubleword
            qw = 0b11          // 64-bit quadword
        };

        std::unique_ptr <std::ostream> output;

        struct instruction {
            uint64_t 	opcode = 0, target = 0;
            uint8_t		id = 0,
                        type 			: 5,
                        cond 			: 3,
                        operand_size 	: 2,
                        dest			: 5;
            uint16_t    ext64 = 0;
            uint32_t	operand0 = 0,
                        operand1 = 0,
                        ext = 0;
            bool 	    operand_sign = 0;
        };

        std::unordered_map <std::string, uint8_t> id_opcode_map = {
        //    ALU binary:           ALU unary:            LSU:                  BNJ:                  SYS:
            { "add"     , 0x0  }, { "not"     , 0x0  }, { "l"       , 0x0  }, { "b"       , 0x0  }, { "halt"    , 0xfe },
            { "sub"     , 0x1  }, { "i"       , 0x1  }, { "s"       , 0x1  }, { "j"       , 0x1  },
            { "rsub"    , 0x2  }, { "d"       , 0x2  }, { "lr"      , 0x2  }, { "call"    , 0xfe },
            { "mul"     , 0x3  }, { "abs"     , 0x3  }, { "lsp"     , 0xe0 }, { "ret"     , 0xff },
            { "div"     , 0x4  },                       { "push"    , 0xd0 },
            { "rdiv"    , 0x5  },                       { "pop"     , 0xd1 },
            { "mod"     , 0x6  },
            { "and"     , 0x7  },
            { "or"      , 0x8  },
            { "xor"     , 0x9  },
            { "sl"      , 0xa  },
            { "sr"      , 0xb  },
            { "cmp"     , 0xc  },
            { "test"    , 0xd  },
            { "addsp"   , 0xe0 },
            { "subsp"   , 0xe1 },
        };

        std::unordered_map <std::string, uint8_t> id_type_map = {
        //    ALU binary:          ALU unary:           LSU:                 BNJ:                 SYS:
            { "add"     , 0x0 }, { "not"     , 0x0 }, { "l"       , 0x1 }, { "b"       , 0x2 }, { "halt"    , 0x3 },
            { "sub"     , 0x0 }, { "i"       , 0x0 }, { "s"       , 0x1 }, { "j"       , 0x2 },
            { "rsub"    , 0x0 }, { "d"       , 0x0 }, { "lr"      , 0x1 }, { "call"    , 0x2 },
            { "mul"     , 0x0 }, { "abs"     , 0x0 }, { "lsp"     , 0x1 }, { "ret"     , 0x2 },                     
            { "div"     , 0x0 },                      { "push"    , 0x1 },
            { "rdiv"    , 0x0 },                      { "pop"     , 0x1 },
            { "mod"     , 0x0 },
            { "and"     , 0x0 },
            { "or"      , 0x0 },
            { "xor"     , 0x0 },
            { "sl"      , 0x0 },
            { "sr"      , 0x0 },
            { "cmp"     , 0x0 },
            { "test"    , 0x0 },
            { "addsp"   , 0x0 },
            { "subsp"   , 0x0 },
        };

        uint64_t encode(parser::detail::instruction& i) {
            size_t sv = 19;
            uint64_t opcode = 0;

            // Encode common fields
            opcode  |= (uint8_t)i.m.cond
                    | ((uint8_t)i.ec << 3)
                    | (id_type_map[i.m.id] << 3)
                    | (id_opcode_map[i.m.id] << 8);

            if (!(i.ec == parser::detail::encoding_class::no_operand)) {
                opcode  |= ((uint8_t)i.m.sign << 16)
                        |  ((uint8_t)i.m.size << 17);
            }

            // Encode operands
            for (parser::detail::operand& o: i.operands) {
                if (o.type == parser::detail::operand_type::r) {
                    opcode |= (o.reg_num << sv);
                    sv += 5;
                } else {
                    // Constants are always MSB
                    opcode |= (uint64_t)((uint64_t)o.const_value << sv);
                }
            }
            return opcode;
        }
    }

    static void assemble() {
        parser::detail::instruction i;

        if (parser::output.eof()) return;

        while (!parser::output.eof()) {
            i = parser::output.get();
            
            size_t len = parser::detail::parse_instruction_length(i);

            uint64_t opcode = detail::encode(i);

            for (int i = 0; i < len; i++) {
                uint8_t masked = (uint8_t)((opcode & (0xffull << (i*8))) >> (i*8));
                if (output_to_stdout) {
                    std::cout.put(masked);
                } else {
                    detail::output->put(masked);
                }
            }
        }
    }

    inline void init(std::ostream&& t_stream) {
        detail::output.reset(&t_stream);
    }

    inline void release_stream() {
        if (!output_to_stdout) {
            detail::output.release()->clear();
        }
    }
}