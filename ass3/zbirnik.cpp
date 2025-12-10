#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <map>

enum Name
{
    A,
    X,
    L,
    B,
    S,
    T,
    F,
    PC = 8,
    SW = 9
};

class Token
{
public:
    std::string left_symbol;
    std::string mnemonic;
    std::string right_symbol;
    int location_counter;
    int right_symbol_value;
};

void print_token(Token const& token)
{
    std::cout << "|" << token.left_symbol << "|" << token.mnemonic << "|" << token.right_symbol << "|                 |" 
    << token.location_counter << "|" << token.right_symbol_value << "|" << std::endl;
}

std::vector<Token> tokenize(std::string const& file_name)
{
    std::ifstream file(file_name);
    std::vector<Token> tokens;
    std::string line;

    while (std::getline(file, line))
    { 
        // Remove carriage return if present
        if (!line.empty() && line.back() == '\r') line.pop_back();

        Token token;
        size_t length = line.length();
        
        if (length < 4) continue;
        token.left_symbol = line.substr(0, 7);
        
        if (length < 16) token.mnemonic = line.substr(8);
        else token.mnemonic = line.substr(8, 7);

        if (length < 17) token.right_symbol = "";
        else token.right_symbol = line.substr(16);

        token.location_counter = -1;
        token.right_symbol_value = -1;
        tokens.push_back(token);
    }
    return tokens;
}

std::vector<Token> strip_white_spaces(std::vector<Token>& tokens)
{
    for (auto& token : tokens)
    {
        int idx1 = 0, idx2 = 0, idx3 = 0;
        while (idx1 < token.left_symbol.size())
        {
            if (token.left_symbol[idx1] == ' ' || token.left_symbol[idx1] == '\t' || 
                token.left_symbol[idx1] == '\r' || token.left_symbol[idx1] == '\n') token.left_symbol.erase(idx1, 1);
            else idx1++;
        }
        while (idx2 < token.mnemonic.size())
        {
            if (token.mnemonic[idx2] == ' ' || token.mnemonic[idx2] == '\t' || 
                token.mnemonic[idx2] == '\r' || token.mnemonic[idx2] == '\n') token.mnemonic.erase(idx2, 1);
            else idx2++;
        }
        while (idx3 < token.right_symbol.size())
        {
            if (token.right_symbol[idx3] == ' ' || token.right_symbol[idx3] == '\t' || 
                token.right_symbol[idx3] == '\r' || token.right_symbol[idx3] == '\n') token.right_symbol.erase(idx3, 1);
            else idx3++;
        }
    }
    return tokens;
}

class Instruction
{
public:
    std::string mnemonic;
    int opcode;
    int size;
};

const std::vector<Instruction> instructions = 
{
    {"LDA",    0x00, 3},
    {"LDX",    0x04, 3},
    {"LDL",    0x08, 3},
    {"STA",    0x0C, 3},
    {"STX",    0x10, 3},
    {"STL",    0x14, 3},
    {"ADD",    0x18, 3},
    {"SUB",    0x1C, 3},
    {"MUL",    0x20, 3},
    {"DIV",    0x24, 3},
    {"COMP",   0x28, 3},
    {"J",      0x3C, 3},
    {"JEQ",    0x30, 3},
    {"JGT",    0x34, 3},
    {"JLT",    0x38, 3},
    {"JSUB",   0x48, 3},
    {"RSUB",   0x4C, 3},
    {"LDCH",   0x50, 3},
    {"STCH",   0x54, 3},
    {"AND",    0x40, 3},
    {"OR",     0x44, 3},
    {"RD",     0xD8, 3},
    {"WD",     0xDC, 3},
    {"TD",     0xE0, 3},
    {"LDB",    0x68, 3},
    {"LDS",    0x6C, 3},
    {"LDT",    0x74, 3},
    {"STB",    0x78, 3},
    {"STS",    0x7C, 3},
    {"STT",    0x84, 3},

    // format-2 (size 2)
    {"ADDR",   0x90, 2},
    {"SUBR",   0x94, 2},
    {"MULR",   0x98, 2},
    {"DIVR",   0x9C, 2},
    {"COMPR",  0xA0, 2},
    {"SHIFTL", 0xA4, 2},
    {"SHIFTR", 0xA8, 2},
    {"RMO",    0xAC, 2},
    {"CLEAR",  0xB4, 2}
};

const std::vector<std::string> directives = 
{
    "START", // todo name
    "END",
    "BYTE",
    "WORD",
    "RESB",
    "RESW",
    "ORG",
    "EQU", // todo 
    "BASE", // todo
    "NOBASE" // todo
};

std::map<std::string, int> sym_tab;

void first_pass(std::vector<Token>& tokens)
{
    int location_counter = 0;
    for (auto& token : tokens)
    {
        if (!token.left_symbol.empty())
        {
            sym_tab[token.left_symbol] = location_counter;
        }

        bool found = false;
        for (auto const& instr : instructions)
        {
            if (token.mnemonic.front() == '+')
            {
                location_counter += 4;
                found = true;
                break;
            }

            if (token.mnemonic == instr.mnemonic)
            {
                location_counter += instr.size;
                found = true;
                break;
            }
        }

        if (!found) 
        {
            for (auto const& dir : directives)
            {
                if (token.mnemonic == dir)
                {
                    if (dir == "WORD") location_counter += 3;
                    else if (dir == "RESW") location_counter += 3 * std::stoi(token.right_symbol);
                    else if (dir == "RESB") location_counter += std::stoi(token.right_symbol);
                    else if (dir == "BYTE") 
                    {
                        if (token.right_symbol.front() == 'C')
                        {
                            location_counter += token.right_symbol.length() - 3; // Subtracting 3 for C'' 
                        }
                        else if (token.right_symbol.front() == 'X')
                        {
                            location_counter += (token.right_symbol.length() - 3 + 1) / 2; // Subtracting 3 for X'' and rounding up
                        }
                    }
                    else if (dir == "START")
                    {
                        location_counter = std::stoi(token.right_symbol, nullptr, 10);
                    }
                    else if (dir == "ORG")
                    {
                        location_counter = std::stoi(token.right_symbol, nullptr, 10);
                    }

                    found = true;
                    break;
                }
            }
        }

        token.location_counter = location_counter;

        if (!found)
        {
            std::cerr << "Error: Unknown mnemonic " << token.mnemonic << std::endl;
            exit(1);
        }
    }

    sym_tab[tokens.front().left_symbol] = stoi(tokens.front().right_symbol, nullptr, 10);
}

bool is_directive(std::string const& mnemonic)
{
    for (auto const& dir : directives)
    {
        if (mnemonic == dir) return true;
    }
    return false;
}

bool is_reg_reg(std::string const& mnemonic)
{
    for (auto const& instr : instructions)
    {
        if (mnemonic == instr.mnemonic && instr.size == 2) return true;
    }
    return false;
}

int name_to_num(std::string const& name)
{
    if (name.front() == 'A') return 0;
    else if (name.front() == 'X') return 1;
    else if (name.front() == 'L') return 2;
    else if (name.front() == 'B') return 3;
    else if (name.front() == 'S' && name.back() != 'W') return 4;
    else if (name.front() == 'T') return 5;
    else if (name.front() == 'F') return 6;
    else if (name == "PC") return 8;
    else if (name == "SW") return 9;
    else 
    {
        std::cerr << "Error: Unknown register name " << name << std::endl;
        exit(1);
    }
}

void second_pass(std::vector<Token>& tokens)
{
    for (auto& token : tokens)
    {
        if (!token.right_symbol.empty())
        {
            if (is_directive(token.mnemonic))
            {
                // 
            }
            else if (is_reg_reg(token.mnemonic))
            {
                int value = name_to_num(token.right_symbol.substr(0,2));
                token.right_symbol_value = value << 4;
                value = name_to_num(token.right_symbol.substr(2));
                token.right_symbol_value |= value;
            }
            else
            {
                bool immediate = false;
                bool indirect = false;
                std::string effective_symbol = token.right_symbol;

                if (token.right_symbol.front() == '#')
                {
                    immediate = true;
                    effective_symbol = token.right_symbol.substr(1);
                }
                else if (token.right_symbol.front() == '@')
                {
                    indirect = true;
                    effective_symbol = token.right_symbol.substr(1);
                }

                if (sym_tab.find(effective_symbol) != sym_tab.end())
                {
                    token.right_symbol_value = sym_tab[effective_symbol];
                }
                else
                {
                    try
                    {
                        token.right_symbol_value = std::stoi(effective_symbol, nullptr, 16);
                    }
                    catch (const std::invalid_argument&)
                    {
                        std::cerr << "Error: Undefined symbol " << effective_symbol << std::endl;
                        exit(1);
                    }
                }
            }
        }
    }
}

std::string assemble(Token t)
{
    if (is_directive(t.mnemonic))
    {
        if (t.mnemonic == "WORD")
        {
            if (t.right_symbol.front() == 'X') return t.right_symbol.substr(2, t.right_symbol.length() - 3);
            else return t.right_symbol;
        }
        else if (t.mnemonic == "BYTE")
        {
            if (t.right_symbol.front() == 'C')
            {
                std::string chars = t.right_symbol.substr(2, t.right_symbol.length() - 3);
                std::string result;
                for (char c : chars)
                {
                    char buf[3];
                    snprintf(buf, sizeof(buf), "%02X", static_cast<unsigned char>(c));
                    result += buf;
                }
                return result;
            }
            else if (t.right_symbol.front() == 'X')
            {
                return t.right_symbol.substr(2, t.right_symbol.length() - 3);
            }
        }
        return "";
    }
    else if (is_reg_reg(t.mnemonic))
    {
        int opcode = 0;
        for (auto const& instr : instructions)
        {
            if (t.mnemonic == instr.mnemonic)
            {
                opcode = instr.opcode;
                break;
            }
        }
        int reg_bits = t.right_symbol_value;
        int machine_code = (opcode << 8) | reg_bits;
        char buffer[5];
        snprintf(buffer, sizeof(buffer), "%04X", machine_code);
        return std::string(buffer);
    }
    else
    {
        return "not implemented";
    }
}

int main()
{
    std::vector<Token> tokens = tokenize("hanoi.asm");
    tokens = strip_white_spaces(tokens);
    first_pass(tokens);
    second_pass(tokens);
    for (auto const& token : tokens)
    {
        print_token(token);
    }

    std::cout << "Machine code:" << std::endl;
    for (auto const& token : tokens)
    {
        std::string machine_code = assemble(token);
        if (!machine_code.empty()) std::cout << machine_code << std::endl;
    }
    
    return 0;
}