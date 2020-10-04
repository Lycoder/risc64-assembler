#include <unordered_map>
#include <iostream>
#include <memory>

/*
    .equ directive: .equ <NAME>, <VALUE>
        All instances of <NAME> will be replaced with <VALUE>

    .label/.l/.<>: directive: .label <NAME>/.l <NAME>/.<NAME>:
        Create a name with a value equal to the current working address

    .dif/.d/:<NAME> directive: .dif <NAME>
        Inserts the result of <NAME> - current address
    
    .pad/.p directive: .pad COUNT, VALUE
        Inserts COUNT number of VALUE in the current address

    .padt/.pt directive: padt CVAL, VALUE
        Inserts VALUE until the value of the cursor is equal to CVAL

    .org VALUE
        Change the starting position of the program
    
    .<name> directive:
        Insert the value of <name>

    .loop:
        bz #:loop; 
*/

class preprocessor {
public:
    enum tokens {
        point   = -1,
        colon   = -2,
        import  = -3,
        base    = -4
    };
private:
    std::unique_ptr <std::istream> stream;
    
    std::unordered_map <std::string, std::string> names;
};