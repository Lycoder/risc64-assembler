#pragma once

#include <optional>
#include <istream>
#include <ostream>
#include <sstream>
#include <memory>
#include <vector>

#include "log.hpp"

namespace lexer {
    bool error_out = false;
    
    struct token {
        int id = 0;
        std::string data = "";

        token() = default;
        token(int id, std::string data = "") : id(id), data(data) {}
    };

    enum tokens {
        k_unknown     = 0,
        k_instruction = 1,
        k_register    = 2,
        k_number      = 3,
        k_semicolon   = 4,
        k_eof         = 5
    };

    namespace detail {
        enum class stream_order {
            normal,
            reverse
        };

        template <class T> class stream : protected std::vector<T> {
            stream_order policy = stream_order::normal;

            size_t index = 0;

        public:
            T last = T();

            void set_policy(stream_order o) {
                policy = o;
            }

            inline bool eof() {
                return (policy == stream_order::normal) ? !(index) : (index == this->size());
            }

            inline T& get() {
                if (policy == stream_order::normal) index--;
                last = this->at(index);
                if (policy == stream_order::reverse) index++;
                return last;
            }

            inline T& peek() {
                return policy == stream_order::normal ? this->at(index-1) : this->at(index+1);
            }

            inline void put(T value) {
                if (policy == stream_order::normal) index++;
                this->push_back(value);
            }
        };


        // Pointer to an arbitrary input stream
        std::unique_ptr <std::istream> input;
        
        // Contains what's been just lexed
        std::string data;

        // Last character that was 'get' from the input stream
        char current_char = ' ';

        // This function extracts the next character in the input
        inline int get_next_char(size_t count = 1) { while (--count) { input->get(); } return input->get(); }

        inline void advance(size_t count = 1) { current_char = get_next_char(count); }
        inline void append() { data += current_char; }
        inline void append_advance() { append(); advance(); }

        inline int peek_next_char() { return input->peek(); }

        // This function skips whitespace characters until a non-whitespace character is found
        inline void ignore_whitespace() { while (std::isspace(current_char)) { current_char = get_next_char(); } }

        // [[:alpha:]]+
        std::optional <int> lex_instruction() {
            if (!(std::isalpha(current_char))) {
                return {};
            }

            while (std::isalpha(current_char)) append_advance();
            
            return tokens::k_instruction;
        }

        // %[[:alpha:]]+[[:digit:]]+
        std::optional <int> lex_register() {
            if (!(current_char == '%')) {
                return {};
            }

            if (!std::isalpha(peek_next_char())) {
                _log(error, "%s: Expected register-type after '%%'", __FUNCTION__);
                error_out = true;

                return {};
            }

            // Ignore '%'
            advance();

            // Lex register type
            while (std::isalpha(current_char)) append_advance();

            // The register number must immediately follow the register type
            if (!std::isdigit(current_char)) {
                _log(error, "%s: Expected register-number after register-type", __FUNCTION__);
                error_out = true;

                return {};
                // Error, expected a register number
            }

            // Lex register number
            while (std::isdigit(current_char)) append_advance();

            // A register operand must be followed by either a ',' (keep parsing operands), or a ';' (single operand)
            // There may be whitespace between the register and either ',' or ';'
            if (std::isspace(current_char)) { ignore_whitespace(); }

            if (!((current_char == ',') || (current_char == ';'))) {
                _log(error, "%s: Expected separator after register-number", __FUNCTION__);
                error_out = true;

                return {};
            }
            
            return k_register;
        }

        // #([+-]?0x[[:xdigit:]]+)[;,]
        // #([+-]?0b[01]+)[;,]
        // #([+-]?[[:digit:]]+)[;,]
        std::optional <int> lex_number() {
            bool is_hex = false,
                 is_bin = false,
                 positive = true,
                 already_decimal = false;

            char delim  = '\'';

            if (!current_char == '#') {
                return {};
            }

            current_char = get_next_char();

            if (current_char == '+' || current_char == '-') {
                if (current_char == '-') { append(); positive = false; }
                current_char = get_next_char();
            }
            
            if (std::isdigit(current_char)) {
                char d = input->peek();
                switch (d) {
                    case 'x': is_hex = true; break;
                    case 'b': is_bin = true; break;
                    default: {
                        if (std::isdigit(d)) {
                            while (std::isdigit(current_char)) append_advance();
                            return k_number;
                        }

                        if (d == ';' || d == ',' || std::isspace(d)) {
                            append_advance();
                            return k_number;
                        } else {
                            _log(error, "%s: Unexpected character '%c' after '#'", __FUNCTION__, d);
                            error_out = true;

                            return {};
                        }
                    } break;
                }

                if (is_hex) {
                    data += "0x";
                    current_char = get_next_char(2);
                    
                    if (!std::isxdigit(current_char)) {
                        _log(error, "%s: Expected a hex value after '0x'", __FUNCTION__);
                        error_out = true;

                        return {};
                    }

                    while (std::isxdigit(current_char)) { append_advance(); }

                    return k_number;
                }

                if (is_bin) {
                    // Store bits here
                    uint64_t q = 0;

                    std::string bin = "";

                    // Use this so we can do std::hex
                    std::stringstream ss;

                    // Prepare for stoull
                    data += "0x";
                    current_char = get_next_char(2);

                    if (!((current_char == '0') || (current_char == '1'))) {
                        _log(error, "%s: Expected a binary value after '0b'", __FUNCTION__);
                        error_out = true;
                        
                        return {};
                    }

                    while ((current_char == '0') || (current_char == '1')) {
                        bin += current_char;
                        current_char = get_next_char();
                    }

                    // Convert binary to hexadecimal
                    for (int b = bin.size(); b >= 0; b--) {
                        if (bin[b] == '1') { q |= (1ull << ((bin.size() - 1) - b)); }
                    }
                    
                    ss << std::hex << q;
                    data += ss.str();

                    return k_number;
                }
            }
            return {};
        }

#define INIT_OPT_HANDLER std::optional <int> t;
#define HANDLE_OPT(k) t = lex_##k(); if (t) return t.value();

        std::optional <int> lex_operand() {
            INIT_OPT_HANDLER;

            HANDLE_OPT(register);
            HANDLE_OPT(number);

            return {};
        }
    }

    int get_next_token() {
        using namespace detail;

        if (input.get()->eof()) return tokens::k_eof;

        ignore_whitespace();

        data = "";

        INIT_OPT_HANDLER

        if (current_char == ';') {
            data += current_char;
            current_char = get_next_char();
            return k_semicolon;
        }

        HANDLE_OPT(instruction);
        HANDLE_OPT(operand);

        return tokens::k_unknown;
    }

#undef INIT_OPT_HANDLER
#undef HANDLE_OPT

    // Output stream
    detail::stream<token> output;

    inline void init(std::istream&& t_stream) {
        using namespace detail;
        input.reset(&t_stream);
        current_char = input->get();
        output.set_policy(detail::stream_order::reverse);
    }

    inline void release_stream() {
        using namespace detail;
        input.release()->clear();
    }


    int lex() {
        int t = 0;
        while (t != k_eof) {
            t = get_next_token();
            if (error_out) return 0;
            if (t) { output.put({t, detail::data}); }
        }
        return 1;
    }
};