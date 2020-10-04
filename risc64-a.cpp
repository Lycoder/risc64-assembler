#include <iostream>
#include <fstream>
#include <cstdlib>

#include "cli.hpp"
#include "log.hpp"

#include "global.hpp"

#include "lexer.hpp"
#include "parser.hpp"
#include "emitter.hpp"


static inline void error_exit() {
    _log(error, "Assembly terminated");

    if (lexer::detail::input) lexer::release_stream();

    if (emitter::detail::output) emitter::release_stream();

    exit(EXIT_SUCCESS);
}

int main(int argc, const char* argv[]) {
    cli::init(argc, argv);

    cli::parse();

    std::ofstream output_file;
    std::ifstream input_file;

    if (!cli::is_defined("input")) {
        if (std::cin.eof()) {
            _log(error, "%s: No input", __FUNCTION__);
            error_exit();
        }
        lexer::init(std::move(std::cin));
    } else {
        input_file.open(cli::settings["input"]);
        if (input_file.good()) {
            lexer::init(std::move(input_file));
        } else {
            _log(error, "%s: Couldn't open input file", __FUNCTION__);
            error_exit();
        }
    }

    if (cli::is_defined("output")) {
        output_file.open(cli::settings["output"]);
        emitter::init(std::move(output_file));
    } else {
        output_to_stdout = true;
    }

    if (!lexer::lex()) error_exit();

    parser::init();

    parser::parse();

    emitter::assemble();

    lexer::release_stream();

    emitter::release_stream();
}