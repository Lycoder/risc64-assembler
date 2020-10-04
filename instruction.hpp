#pragma once

#include <cstdint>
#include <vector>
#include <string>

namespace risc64 {
    enum class register_type { gpr, fpr, tsr };
    enum class operand_size { b, w, d, q };
    enum class operand_sign { u, s };
    enum class operand_type { r, c };
    enum class condition { z, c, n, a, nv, p, nc, nz };

    enum class encoding_class : uint8_t {
        t_register_all              = 0b00000,
        t_register_single_const     = 0b00100,
        d_register_all              = 0b01000,
        d_register_single_const     = 0b01100,
        d_register_double_const     = 0b10000,
        s_register                  = 0b10100,
        s_const                     = 0b11000,
        no_operand                  = 0b11100
    };

    struct operand {
        uint64_t        const_value;
        size_t          position;
        register_type   reg_type;
        size_t          reg_num;
        operand_type    type;
    };

    struct mnemonic {
        std::string     id;
        operand_size    size;
        operand_sign    sign;
        condition       cond;
    };

    struct instruction {
        typedef std::vector<operand> operand_array_t;
        mnemonic        m;
        encoding_class  ec;
        operand_array_t operands;
    };
}