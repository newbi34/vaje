#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <memory>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <iomanip>

#define MAX_ADDRESS 1024 * 1024 // 1 MB of RAM

static volatile sig_atomic_t stop_requested = 0;
static float sleep_time_us = 1000.0f; 

void singalHandler(int sig)  
{ 
    stop_requested = 1;
    return;
} 

typedef struct breakpoint
{
    unsigned int address;
    struct breakpoint* next;
} Breakpoint;

Breakpoint* first;

void add_breakpoint(unsigned int address)
{
    Breakpoint* new_bp = new Breakpoint();
    new_bp->address = address;
    new_bp->next = nullptr;

    if (first == nullptr)
    {
        first = new_bp;
    }
    else
    {
        Breakpoint* current = first;
        while (current->next != nullptr)
        {
            current = current->next;
        }
        current->next = new_bp;
    }
}

bool on_breakpoint(unsigned int address)
{
    while (first != nullptr && first->next != nullptr)
    {
        if (first->next->address == address) return true;
        first = first->next;
    }
    return false;
}

class Opcode
{
public:
    const static int LDA = 0x00;
    const static int LDX = 0x04;
    const static int LDL = 0x08;
    const static int LDB = 0x68;
    const static int LDT = 0x74;
    const static int LDS = 0x6C;
    const static int STA = 0x0C;
    const static int STX = 0x10;
    const static int STL = 0x14;
    const static int STB = 0x78;
    const static int STT = 0x84;
    const static int STS = 0x7C;
    const static int LDCH = 0x50;
    const static int STCH = 0x54;
    const static int AND = 0x40;
    const static int OR = 0x44;
    const static int ADD = 0x18;
    const static int SUB = 0x1C;
    const static int MUL = 0x20;
    const static int DIV = 0x24;
    const static int COMP = 0x28;
    const static int RD = 0xD8;
    const static int WD = 0xDC;
    const static int TD = 0xE0;
    const static int J = 0x3C;
    const static int JEQ = 0x30;
    const static int JGT = 0x34;
    const static int JLT = 0x38;
    const static int JSUB = 0x48;
    const static int RSUB = 0x4C;
    const static int ADDR = 0x90;
    const static int SUBR = 0x94;
    const static int MULR = 0x98;
    const static int DIVR = 0x9C;
    const static int COMPR = 0xA0;
    const static int SHIFTL = 0xA4;
    const static int SHIFTR = 0xA8;
    const static int RMO = 0xAC;
    const static int CLEAR = 0xB4;
};

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

class Register
{
private:
    int value;
    Name name;

public:
    Register(Name name)
    {
        this->name = name;
        this->value = 0;
    }

    int get() 
    {
        return this->value;
    }

    void set(int value)
    {
        this->value = value;
    }

    Name get_name()
    {
        return this->name;
    }
};

class FloatRegister
{
private:
    double value;
    Name name;

public:
    FloatRegister(Name name)
    {
        this->name = name;
        this->value = 0;
    }

    FloatRegister()
    {
        this->name = F;
        this->value = 0;
    }

    double get() 
    {
        return this->value;
    }

    void set(double value)
    {
        this->value = value;
    }

    Name get_name()
    {
        return this->name;
    }
};

class Device
{
private:
    int id;
    std::string name;
    std::unique_ptr<std::fstream> file;
    std::istream* in = nullptr;
    std::ostream* out = nullptr;

public:
    // file-backed device (read/write)
    Device(int id, const std::string& filename)
    {
        this->id = id;
        this->name = filename;
        this->file = std::unique_ptr<std::fstream>(new std::fstream());

        file->open(filename, std::ios::in | std::ios::out);
        if (file->is_open()) { in = file.get(); out = file.get(); }
    }

    // input-only (stdin)
    Device(int id, std::istream* input)
    {
        this->id = id;
        this->name = "in";
        this->in = input;
        this->out = nullptr;
    }

    // output-only (stdout/stderr)
    Device(int id, std::ostream* output)
    {
        this->id = id;
        this->name = "out";
        this->out = output;
    }

    ~Device()
    {
        if (file && file->is_open()) file->close();
    }

    bool test()
    {
        if (in) return true;
        if (out) return true;
        return file && file->is_open();
    }

    // return -1 on EOF/error
    int read_byte()
    {
        if (in)
        {
            char c;
            if (in->get(c)) return static_cast<unsigned char>(c);
            return -1;
        }
        return -1;
    }

    bool write_byte(char byte)
    {
        if (out)
        {
            out->put(byte);
            return static_cast<bool>(*out);
        }
        return false;
    }

    int get_id()
    {
        return this->id;
    }
};

class RAM 
{
private:
    std::vector<unsigned char> memory;

public:

    RAM() : memory(MAX_ADDRESS) {}

    unsigned char get_byte(unsigned int address)
    {
        if (address >= memory.size()) { std::cerr << "RAM access out of bounds at " << address << "\n"; return 0; }
        return this->memory[address];
    }

    void set_byte(unsigned int address, unsigned char value)
    {
        if (address >= memory.size()) { std::cerr << "RAM access out of bounds at " << address << "\n"; return; }
        this->memory[address] = value;
    }

    unsigned int get_word(int address)
    {
        if (address + 2 >= memory.size()) { std::cerr << "RAM access out of bounds at " << address << "\n"; return 0; }
        return (unsigned int)memory[address + 2] | ((unsigned int)memory[address + 1] << 8) | ((unsigned int)memory[address] << 16);
    }

    void set_word(unsigned int address, int value)
    {
        if (address + 2 >= memory.size()) { std::cerr << "RAM access out of bounds at " << address << "\n"; return; }
        memory[address] = (value >> 16) & 0xFF;
        memory[address + 1] = (value >> 8)  & 0xFF;
        memory[address + 2] = value & 0xFF;
    } 
};

class Machine
{
private:
    std::vector<Register> regs;
    FloatRegister float_reg;
    RAM ram;

    std::vector<Device*> devices;

    bool is_running;

public:
    Machine()
    {
        this->regs.push_back(Register(A));        
        this->regs.push_back(Register(X));            
        this->regs.push_back(Register(L));        
        this->regs.push_back(Register(B));        
        this->regs.push_back(Register(S));        
        this->regs.push_back(Register(T));        
        this->regs.push_back(Register(PC));        
        this->regs.push_back(Register(SW)); // 0 == equal, <0 = less than, >0 = greater than
        this->float_reg = FloatRegister(F);

        this->devices.push_back(new Device(0, &std::cin));
        this->devices.push_back(new Device(1, &std::cout));
        this->devices.push_back(new Device(2, &std::cerr));

        this->ram = RAM();

        this->is_running = false;
    }

    int set_reg(Name name, int value)
    {
        for (Register& r : regs)
        {
            if (r.get_name() == name)
            {
                r.set(value);
                return 0;
            }
        }
        return -1;
    }

    int get_reg(Name name)
    {
        for (Register& r : regs)
        {
            if (r.get_name() == name)
            {
                return r.get();
            }
        }
        return -1;
    }

    void set_mem(unsigned int address, int value)
    {
        ram.set_word(address, value);
    }

    int get_mem(unsigned int address, int value)
    {
        return ram.get_word(address);
    }

    void set_mem_byte(unsigned int address, int value)
    {
        ram.set_byte(address, value);
    }

    int get_mem_byte(unsigned int address, int value)
    {
        return ram.get_byte(address);
    }

    int write_device(int device_id, char byte)
    {
        for (Device* d : devices)
        {
            if (d->test() && d->get_id() == device_id && d->write_byte(byte)) return 0;
        }
        return -1;
    }

    unsigned char read_device(int device_id)
    {
        for (Device* d : devices)
        {
            if (d->test() && d->get_id() == device_id)
            {
                int byte = d->read_byte();
                if (byte != -1) return static_cast<unsigned char>(byte);
            }
        }
        return 0;
    }

    bool test_device(int device_id)
    {
        for (Device* d : devices)
        {
            if (d->get_id() == device_id && d->test()) return true;
        }
        return false;
    }

    bool open_device(int device_id, const std::string& filename)
    {
        for (Device* d : devices)
        {
            if (d->get_id() == device_id) return d->test();
        }

        devices.push_back(new Device(device_id, filename));
        return devices.back()->test();
    }
        
    ~Machine()
    {
        for (Device* d : devices) delete d;
    }

    enum exception
    {
        INVALID_OPCODE,
        INVALID_ADDRESSING,
        NOT_IMPLEMENTED,
        DIVIDE_BY_ZERO
    };

    void handle_exception(exception e, std::string info = "")
    {
        switch (e)
        {
            case INVALID_OPCODE:
                std::cerr << "Invalid opcode: " << info << "\n";
                break;
            case INVALID_ADDRESSING:
                std::cerr << "Bad address / register.\n";
                break;
            case NOT_IMPLEMENTED:
                std::cerr << info << " not implemented yet.\n";
                break;
            case DIVIDE_BY_ZERO:
                std::cerr << "Division by zero error.\n";
                break;
            default:
                std::cerr << "Unknown exception.\n";
                break;
        }

        exit(1);
    }

    unsigned char fetch_byte()
    {
        unsigned int pc = get_reg(PC);
        unsigned char byte = this->ram.get_byte(pc);
        set_reg(PC, pc + 1);
        return byte;
    }

    unsigned int inline handle_bp(int bp, unsigned char byte2, unsigned char byte3)
    {
        switch (bp)
        {
            case 0:
            {
                return byte3 + ((byte2 & 0x0F) << 8);
            }
            case 0b00100000:
            {
                return byte3 + ((byte2 & 0x0F) << 8) + get_reg(PC);
            }
            case 0b01000000:
            {
                return byte3 + ((byte2 & 0x0F) << 8) + get_reg(B);
            }
            case 0b01100000:
            {
                handle_exception(NOT_IMPLEMENTED, "Base + PC relative addressing");
                return 0;
            }
        }
        return 0;
    }

    void execute()
    {
        if (on_breakpoint(get_reg(PC))) 
        {
            std::cout << "Breakpoint hit at address 0x" << std::hex << get_reg(PC) << "\n";
            stop_requested = 1;
            return;
        } 

        unsigned char byte1 = fetch_byte();
        unsigned char byte2 = fetch_byte(); // no 1 byte instructions are supported yet
        unsigned char byte2_high = (byte2 & 0xF0) >> 4;
        unsigned char byte2_low = byte2 & 0x0F;

        unsigned char opcode = byte1 & 0xFC;
        switch (opcode)
        {
            case Opcode::CLEAR:
                if (set_reg(static_cast<Name>(byte2_high), 0) == -1) handle_exception(INVALID_ADDRESSING);
                break;
            case Opcode::RMO:
            {
                unsigned int value = get_reg(static_cast<Name>(byte2_high));
                if (value == -1) handle_exception(INVALID_ADDRESSING);
                set_reg(static_cast<Name>(byte2_low), value);
                break;
            }
            case Opcode::SHIFTR:
            {
                unsigned int value = get_reg(static_cast<Name>(byte2_high));
                if (value == -1) handle_exception(INVALID_ADDRESSING);
                set_reg(static_cast<Name>(byte2_high), value >> byte2_low);
                break;
            }
            case Opcode::SHIFTL:
            {
                unsigned int value = get_reg(static_cast<Name>(byte2_high));
                if (value == -1) handle_exception(INVALID_ADDRESSING);
                set_reg(static_cast<Name>(byte2_high), value << byte2_low);
                break;
            }
            case Opcode::COMPR:
            {
                int value1 = get_reg(static_cast<Name>(byte2_high));
                int value2 = get_reg(static_cast<Name>(byte2_low));
                if (value1 == -1 || value2 == -1) handle_exception(INVALID_ADDRESSING);
                if (value1 < value2) set_reg(SW, -1);
                else if (value1 == value2) set_reg(SW, 0);
                else set_reg(SW, 1);
                break;
            }
            case Opcode::DIVR:
            {
                int value1 = get_reg(static_cast<Name>(byte2_high));
                int value2 = get_reg(static_cast<Name>(byte2_low));
                if (value1 == -1 || value2 == -1) handle_exception(INVALID_ADDRESSING);
                if (value1 == 0) handle_exception(DIVIDE_BY_ZERO);
                set_reg(static_cast<Name>(byte2_low), value2 / value1);
                break;
            }
            case Opcode::MULR:
            {
                int value1 = get_reg(static_cast<Name>(byte2_high));
                int value2 = get_reg(static_cast<Name>(byte2_low));
                if (value1 == -1 || value2 == -1) handle_exception(INVALID_ADDRESSING);
                set_reg(static_cast<Name>(byte2_low), value2 * value1);
                break;
            }
            case Opcode::SUBR:
            {
                int value1 = get_reg(static_cast<Name>(byte2_high));
                int value2 = get_reg(static_cast<Name>(byte2_low));
                if (value1 == -1 || value2 == -1) handle_exception(INVALID_ADDRESSING);
                set_reg(static_cast<Name>(byte2_low), value2 - value1);
                break;
            }
            case Opcode::ADDR:
            {
                int value1 = get_reg(static_cast<Name>(byte2_high));
                int value2 = get_reg(static_cast<Name>(byte2_low));
                if (value1 == -1 || value2 == -1) handle_exception(INVALID_ADDRESSING);
                set_reg(static_cast<Name>(byte2_low), value2 + value1);
                break;
            }
        
            default: // format 3 or 4 or old sic
            {
                unsigned char ni = byte1 & 0b00000011;
                unsigned char x = byte2 & 0b10000000;
                unsigned char bp = byte2 & 0b01100000;
                unsigned char extended = byte2 & 0b00010000;

                unsigned char byte3 = fetch_byte();
                unsigned int effective_address = 0;
                unsigned int operand = 0;

                if (ni == 0)
                {
                    if (x) effective_address = byte3 + ((byte2 & 0x7F) << 8) + get_reg(X);
                    else  effective_address = byte3 + ((byte2 & 0x0F) << 8);
                    operand = ram.get_word(effective_address);

                    if (bp || extended) handle_exception(NOT_IMPLEMENTED, "old sic base / PC relative or format 4");
                }
                else
                {
                    if (extended)
                    {
                        unsigned char byte4 = fetch_byte();
                        if (x) effective_address = byte4 + (byte3 << 8) + ((byte2 & 0x0F) << 16) + get_reg(X);
                        else effective_address = byte4 + (byte3 << 8) + ((byte2 & 0x0F) << 16);
                        switch (ni)
                        {
                            case 3:
                            {
                                operand = ram.get_word(effective_address);
                                break;
                            }
                            case 2:
                            {
                                effective_address = ram.get_word(effective_address);
                                operand = ram.get_word(effective_address);
                                break;
                            }
                            case 1:
                            {
                                operand = effective_address;
                                break;
                            }
                        }
                        if (bp) handle_exception(NOT_IMPLEMENTED, "Base / PC relative addressing with format 4");
                    }
                    else
                    {
                        switch (ni)
                        {
                            case 3:
                            {
                                effective_address = handle_bp(bp, byte2, byte3);
                                if (x) effective_address += get_reg(X);
                                operand = ram.get_word(effective_address);
                                break;
                            }
                            case 2: // supporting LDA   @ADR, X --- LDA = ((ADR) + X) :D
                            {
                                effective_address = handle_bp(bp, byte2, byte3);
                                effective_address = ram.get_word(effective_address);
                                if (x) effective_address += get_reg(X);
                                operand = ram.get_word(effective_address);
                                break;
                            }
                            case 1:
                            {
                                effective_address = handle_bp(bp, byte2, byte3);
                                operand = effective_address;
                                break;
                            }
                        }
                    }
                }
                
                switch (opcode)
                {
                    case Opcode::LDA:
                    {
                        set_reg(A, operand);
                        break;
                    }
                    case Opcode::LDX:
                    {
                        set_reg(X, operand);
                        break;
                    }
                    case Opcode::LDL:
                    {
                        set_reg(L, operand);
                        break;
                    }
                    case Opcode::LDB:
                    {
                        set_reg(B, operand);
                        break;
                    }
                    case Opcode::LDT:
                    {
                        set_reg(T, operand);
                        break;
                    }
                    case Opcode::LDS:
                    {
                        set_reg(S, operand);
                        break;
                    }
                    case Opcode::LDCH:
                    {
                        set_reg(A, operand & 0xFF);
                        break;
                    }
                    case Opcode::STA:
                    {
                        ram.set_word(effective_address, get_reg(A));
                        break;
                    }
                    case Opcode::STX:
                    {
                        ram.set_word(effective_address, get_reg(X));
                        break;
                    }
                    case Opcode::STL:
                    {
                        ram.set_word(effective_address, get_reg(L));
                        break;
                    }
                    case Opcode::STB:
                    {
                        ram.set_word(effective_address, get_reg(B));
                        break;
                    }
                    case Opcode::STT:
                    {
                        ram.set_word(effective_address, get_reg(T));
                        break;
                    }
                    case Opcode::STS:
                    {
                        ram.set_word(effective_address, get_reg(S));
                        break;
                    }
                    case Opcode::STCH:
                    {
                        ram.set_byte(effective_address, static_cast<unsigned char>(get_reg(A) & 0xFF));
                        break;
                    }
                    case Opcode::OR:
                    {
                        int value = get_reg(A);
                        if (value == -1) handle_exception(INVALID_ADDRESSING);
                        value = value | operand;
                        set_reg(A, value);
                        break;
                    }
                    case Opcode::AND:
                    {
                        int value = get_reg(A);
                        if (value == -1) handle_exception(INVALID_ADDRESSING);
                        value = value & operand;
                        set_reg(A, value);
                        break;
                    }
                    case Opcode::ADD:
                    {
                        int value = get_reg(A);
                        if (value == -1) handle_exception(INVALID_ADDRESSING);
                        value = value + operand;
                        set_reg(A, value);
                        break;
                    }
                    case Opcode::SUB:
                    {
                        int value = get_reg(A);
                        if (value == -1) handle_exception(INVALID_ADDRESSING);
                        value = value - operand;
                        set_reg(A, value);
                        break;
                    }
                    case Opcode::MUL:
                    {
                        int value = get_reg(A);
                        if (value == -1) handle_exception(INVALID_ADDRESSING);
                        value = value * operand;
                        set_reg(A, value);
                        break;
                    }
                    case Opcode::DIV:
                    {
                        int value = get_reg(A);
                        if (value == -1) handle_exception(INVALID_ADDRESSING);
                        if (operand == 0) handle_exception(DIVIDE_BY_ZERO);
                        value = value / operand;
                        set_reg(A, value);
                        break;
                    }
                    case Opcode::COMP:
                    {
                        int value = get_reg(A);
                        if (value == -1) handle_exception(INVALID_ADDRESSING);
                        if (value < operand) set_reg(SW, -1);
                        else if (value == operand) set_reg(SW, 0);
                        else set_reg(SW, 1);
                        break;
                    }
                    
                    case Opcode::RD:
                    {
                        if (operand > 255) handle_exception(INVALID_ADDRESSING);
                        char name[7];
                        snprintf(name, sizeof(name), "%x%x.dev", (operand >> 4) & 0x0F, operand & 0x0F);
                        if (!test_device(operand)) open_device(operand, name);
                        unsigned char byte = read_device(operand);
                        set_reg(A, byte);
                        break;
                    }
                    case Opcode::WD:
                    {
                        if (operand > 255) handle_exception(INVALID_ADDRESSING);
                        char name[7];
                        snprintf(name, sizeof(name), "%x%x.dev", (operand >> 4) & 0x0F, operand & 0x0F);
                        if (!test_device(operand)) open_device(operand, name);
                        unsigned char byte = get_reg(A) & 0xFF;
                        write_device(operand, byte);
                        break;
                    }
                    case Opcode::TD:
                    {
                        if (operand > 255) handle_exception(INVALID_ADDRESSING);
                        char name[7];
                        snprintf(name, sizeof(name), "%x%x.dev", (operand >> 4) & 0x0F, operand & 0x0F);
                        if (!test_device(operand)) open_device(operand, name);
                        bool ready = test_device(operand);
                        set_reg(SW, ready ? 0 : 1); // zero = ready, non‑zero = not ready
                        break;
                    }
                    case Opcode::J:
                    {
                        if (extended && get_reg(PC) - 4 == effective_address)
                        {
                            std::cout << "Infinite loop detected at 0x" << std::hex << get_reg(PC) - 4 << ". Stopping execution.\n";
                            stop_requested = 1;
                        }
                        else if (!extended && get_reg(PC) - 3 == effective_address)
                        {
                            std::cout << "Infinite loop detected at 0x" << std::hex << get_reg(PC) - 3 << ". Stopping execution.\n";
                            stop_requested = 1;
                        }
                        set_reg(PC, effective_address);
                        break;
                    }
                    case Opcode::JEQ:
                    {
                        if (get_reg(SW) == 0) set_reg(PC, effective_address);
                        break;
                    }
                    case Opcode::JGT:
                    {
                        if (get_reg(SW) > 0) set_reg(PC, effective_address);
                        break;
                    }
                    case Opcode::JLT:
                    {
                        if (get_reg(SW) < 0) set_reg(PC, effective_address);
                        break;
                    }
                    case Opcode::JSUB:
                    {
                        set_reg(L, get_reg(PC));
                        set_reg(PC, effective_address);
                        break;
                    }
                    case Opcode::RSUB:
                    {
                        set_reg(PC, get_reg(L));
                        break;
                    }
                }
                
                break;
            }
        }
    }

    void execute_loop()
    {
        this->is_running = true;
        while (!stop_requested)
        {
            for (int i = 0; i < 10 && !stop_requested; i++) execute();
            usleep(sleep_time_us);
        }
        stop_requested = 0;
        this->is_running = false;
    }

    void show_ram(unsigned int start_address = 0, unsigned int end_address = 0xFF)
    {
        for (unsigned int addr = start_address; addr <= end_address; addr += 3)
        {
            unsigned int byte1 = ram.get_byte(addr);
            unsigned int byte2 = ram.get_byte(addr + 1);
            unsigned int byte3 = ram.get_byte(addr + 2);
            if ((addr - start_address) % 15 == 0) std::cout << std::hex << "0x" << std::left << std::setw(7) << addr;
            std::cout << std::hex << std::right << std::setw(2) << std::setfill('0') << byte1 << " " << std::right << std::setw(2) << byte2 << " " << std::right << std::setw(2) << byte3 << std::setfill(' ');
            if ((addr - start_address) % 15 == 12) std::cout << "\n";
            else std::cout << " ";
        }
        std::cout << "\n";
        return;
    }

    void dissasemble(unsigned int start_address = 0, unsigned int end_address = 0xFF)
    {
        unsigned int addr = start_address;
        while (addr <= end_address)
        {
            unsigned int orig_addr = addr;
            unsigned char byte1 = ram.get_byte(addr);
            unsigned char byte2 = ram.get_byte(addr + 1);

            unsigned char byte2_high = (byte2 & 0xF0) >> 4;
            unsigned char byte2_low = byte2 & 0x0F;

            unsigned char opcode = byte1 & 0xFC;

            std::cout << std::hex << "0x" << std::left << std::setw(7) << addr;

            switch (opcode)
            {
                case Opcode::CLEAR:
                    std::cout << "CLEAR ";
                    std::cout << static_cast<Name>(byte2_high) << "\n";
                    addr += 2;
                    break;
                case Opcode::RMO:
                    std::cout << "RMO ";
                    std::cout << static_cast<Name>(byte2_high) << ", " << static_cast<Name>(byte2_low) << "\n";
                    addr += 2;
                    break;
                case Opcode::SHIFTR:
                    std::cout << "SHIFTR ";
                    std::cout << static_cast<Name>(byte2_high) << ", " << std::dec << (int)byte2_low << "\n";
                    addr += 2;
                    break;
                case Opcode::SHIFTL:
                    std::cout << "SHIFTL ";
                    std::cout << static_cast<Name>(byte2_high) << ", " << std::dec << (int)byte2_low << "\n";
                    addr += 2;
                    break;
                case Opcode::COMPR:
                    std::cout << "COMPR ";
                    std::cout << static_cast<Name>(byte2_high) << ", " << static_cast<Name>(byte2_low) << "\n";
                    addr += 2;
                    break;
                case Opcode::DIVR:
                    std::cout << "DIVR ";
                    std::cout << static_cast<Name>(byte2_high) << ", " << static_cast<Name>(byte2_low) << "\n";
                    addr += 2;
                    break;
                case Opcode::MULR:
                    std::cout << "MULR ";
                    std::cout << static_cast<Name>(byte2_high) << ", " << static_cast<Name>(byte2_low) << "\n";
                    addr += 2;
                    break;
                case Opcode::SUBR:
                    std::cout << "SUBR ";
                    std::cout << static_cast<Name>(byte2_high) << ", " << static_cast<Name>(byte2_low) << "\n";
                    addr += 2;
                    break;
                case Opcode::ADDR:
                    std::cout << "ADDR ";
                    std::cout << static_cast<Name>(byte2_high) << ", " << static_cast<Name>(byte2_low) << "\n";
                    addr += 2;
                    break;

                default:
                {
                    unsigned char ni = byte1 & 0b00000011;
                    unsigned char x = byte2 & 0b10000000;
                    unsigned char bp = byte2 & 0b01100000;
                    unsigned char extended = byte2 & 0b00010000;

                    unsigned char byte3 = ram.get_byte(addr + 2);
                    unsigned int effective_address = 0;

                    unsigned char byte4 = 0;

                    if (extended)
                    {
                        byte4 = ram.get_byte(addr + 3);
                        std::cout << "+";
                        effective_address = byte4 + (byte3 << 8) + ((byte2 & 0x0F) << 16);
                        addr += 4;
                    }
                    else
                    {
                        effective_address = (byte3) + ((byte2 & 0x0F) << 8);
                        addr += 3;
                    }

                    switch (opcode)
                    {
                        case Opcode::LDA:
                            std::cout << "LDA ";
                            break;
                        case Opcode::LDX:
                            std::cout << "LDX ";
                            break;
                        case Opcode::LDL:
                            std::cout << "LDL ";
                            break;
                        case Opcode::LDB:
                            std::cout << "LDB ";
                            break;
                        case Opcode::LDT:
                            std::cout << "LDT ";
                            break;
                        case Opcode::LDS:
                            std::cout << "LDS ";
                            break;
                        case Opcode::LDCH:
                            std::cout << "LDCH ";
                            break;
                        case Opcode::STA:
                            std::cout << "STA ";
                            break;
                        case Opcode::STX:
                            std::cout << "STX ";
                            break;
                        case Opcode::STL:
                            std::cout << "STL ";
                            break;
                        case Opcode::STB:
                            std::cout << "STB ";
                            break;
                        case Opcode::STT:
                            std::cout << "STT ";
                            break;
                        case Opcode::STS:
                            std::cout << "STS ";
                            break;
                        case Opcode::STCH:
                            std::cout << "STCH ";
                            break;
                        case Opcode::OR:
                            std::cout << "OR ";
                            break;
                        case Opcode::AND:
                            std::cout << "AND ";
                            break;
                        case Opcode::ADD:
                            std::cout << "ADD ";
                            break;
                        case Opcode::SUB:
                            std::cout << "SUB ";
                            break;
                        case Opcode::MUL:
                            std::cout << "MUL ";
                            break;
                        case Opcode::DIV:
                            std::cout << "DIV ";
                            break;
                        case Opcode::COMP:
                            std::cout << "COMP ";
                            break;
                        case Opcode::RD:
                            std::cout << "RD ";
                            break;
                        case Opcode::WD:
                            std::cout << "WD ";
                            break;
                        case Opcode::TD:
                            std::cout << "TD ";
                            break;
                        case Opcode::J:
                            std::cout << "J ";
                            break;
                        case Opcode::JEQ:
                            std::cout << "JEQ ";
                            break;
                        case Opcode::JGT:
                            std::cout << "JGT ";
                            break;
                        case Opcode::JSUB:
                            std::cout << "JSUB ";
                            break;
                        case Opcode::JLT:
                            std::cout << "JLT ";
                            break;
                        case Opcode::RSUB:
                            std::cout << "RSUB ";
                            break;
                    }

                    switch (ni)
                    {
                        case 2:
                            std::cout << "@";
                            break;
                        case 1:
                            std::cout << "#";
                            break;

                        default:
                            break;
                    }

                    std::cout << std::hex << effective_address;

                    if (x) std::cout << ", X";

                    switch (bp)
                    {
                        case 0b00100000:
                            std::cout << " (PC relative)";
                            break;
                        case 0b01000000:
                            std::cout << " (Base relative)";
                            break;
                        default:
                            break;
                    }
                    break;
                }
            }

            if (get_reg(PC) == orig_addr) std::cout << "    <-- PC";
            std::cout << "\n";
        }
    }
};

inline std::ostream& operator<<(std::ostream& os, Name n)
{
    switch (n)
    {
        case A:  return os << 'A';
        case X:  return os << 'X';
        case L:  return os << 'L';
        case B:  return os << 'B';
        case S:  return os << 'S';
        case T:  return os << 'T';
        case F:  return os << 'F';
        case PC: return os << "PC";
        case SW: return os << "SW";
        default: return os << static_cast<int>(n);
    }
}

typedef void* (*action_fn)(const char**, Machine&);
struct action_entry 
{
    const char* name;
    action_fn func;
    const char* help;
};

void* exit(const char** argv, Machine& machine)
{
    exit(0);
    return nullptr;
}

void* start(const char** argv, Machine& machine)
{
    stop_requested = 0;
    machine.execute_loop();
    return nullptr;
}

void* stop(const char** argv, Machine& machine)
{
    stop_requested = 1;
    return nullptr;
}

void* step(const char** argv, Machine& machine)
{
    machine.execute();
    return nullptr;
}

void* set(const char** argv, Machine& machine)
{
    if (strcmp(argv[1], "mem") == 0)
    {
        unsigned int address = std::stoul(argv[2], nullptr, 0);
        int value = std::stoi(argv[3], nullptr, 0);
        machine.set_mem(address, value);
    }
    else if (strcmp(argv[1], "reg") == 0)
    {
        unsigned int name = std::stoul(argv[2], nullptr, 0);
        Name reg_name = static_cast<Name>(name);
        int value = std::stoi(argv[3], nullptr, 0);
        machine.set_reg(reg_name, value);
    }
    return nullptr;
}

void* get(const char** argv, Machine& machine)
{
    if (strcmp(argv[1], "mem") == 0)
    {
        unsigned int address = std::stoul(argv[2], nullptr, 0);
        int value = machine.get_mem(address, 0);
        std::cout << "Memory at 0x" << std::hex << address << " = 0x" << std::hex << value << "\n";
    }
    else if (strcmp(argv[1], "reg") == 0)
    {
        unsigned int name = std::stoul(argv[2], nullptr, 0);
        Name reg_name = static_cast<Name>(name);
        int value = machine.get_reg(reg_name);
        std::cout << "Register " << reg_name << " = 0x" << std::hex << value << "\n";
    }
    return nullptr;
}

void* show(const char** argv, Machine& machine)
{
    unsigned int start_address = machine.get_reg(PC);
    unsigned int end_address = start_address + 0xFF;
    if (argv[1][0] != '\0') start_address = std::stoul(argv[1], nullptr, 0);
    if (argv[2][0] != '\0') end_address = std::stoul(argv[2], nullptr, 0);
    else end_address = start_address + 0xFF;
    machine.show_ram(start_address, end_address);
    return nullptr;
}

void* dissasemble(const char** argv, Machine& machine)
{
    unsigned int start_address = machine.get_reg(PC);
    unsigned int end_address = start_address + 0xFF;
    if (argv[1][0] != '\0') start_address = std::stoul(argv[1], nullptr, 0);
    if (argv[2][0] != '\0') end_address = std::stoul(argv[2], nullptr, 0);
    else end_address = start_address + 0xFF;
    machine.dissasemble(start_address, end_address);
    return nullptr;
}

void* set_breakpoint(const char** argv, Machine& machine)
{
    unsigned int address = std::stoul(argv[1], nullptr, 0);
    add_breakpoint(address);
    std::cout << "Breakpoint set at address 0x" << std::hex << address << "\n";
    return nullptr;
}

void* load_file(const char** argv, Machine& machine)
{
    
    std::ifstream file(argv[1], std::ios::binary);
    if (!file)
    {
        std::cerr << "error opening file: " << argv[1] << "\n";
        return nullptr;
    }

    std::string line;
    std::getline(file, line);

    int i = 0;
    while (line[i++] != ' ');

    unsigned int address = std::stoul(line.substr(i, 6), nullptr, 16);
    unsigned int length = std::stoul(line.substr(i + 6, 6), nullptr, 16);
    std::cout << "loading program at address 0x" << std::hex << address << "\n";
    std::cout << "program length: 0x" << std::hex << length << " bytes\n";

    while (std::getline(file, line))
    {
        if (line[0] == 'E') break;
        if (line[0] != 'T') continue;

        unsigned int record_address = std::stoul(line.substr(1, 6), nullptr, 16);
        unsigned int record_length = std::stoul(line.substr(7, 2), nullptr, 16);

        for (unsigned int i = 0; i < record_length; i++)
        {
            unsigned int byte = std::stoul(line.substr(9 + i * 2, 2), nullptr, 16);
            machine.set_mem_byte(address + record_address + i, byte);
        }
    }

    return nullptr;
}

void* set_speed(const char** argv, Machine& machine)
{
    float khz = std::stof(argv[1], nullptr);
    sleep_time_us = 10000.0f / khz;
    std::cout << "execution speed set to " << khz << " kHz\n";
    return nullptr;
}

void* get_speed(const char** argv, Machine& machine)
{
    float khz = 10000.0f / sleep_time_us;
    std::cout << "current execution speed: " << khz << " kHz\n";
    return nullptr;
}

void* show_bps(const char** argv, Machine& machine)
{
    std::cout << "Breakpoints:\n";
    Breakpoint* current = first;
    while (current != nullptr)
    {
        std::cout << "0x" << std::hex << current->address << "\n";
        current = current->next;
    }
    return nullptr;
}

const struct action_entry actions[] =
{
    {"quit", exit, "Exit the simulator."},
    {"exit", exit, "Exit the simulator."},
    {"start", start, "Start the simulator."},
    {"stop", stop, "Stop the simulator."},
    {"step", step, "Execute one instruction."},
    {"set", set, "Set register/memory value. usage: set mem <address> <value> | set reg <regname> <value>"},
    {"get", get, "Get register/memory value. usage: get mem <address> | get reg <regname>"},
    {"show", show, "Show memory contents. usage: show <start_address> <end_address>"},
    {"dissasemble", dissasemble, "Dissasemble memory contents. usage: dissasemble <start_address> <end_address>"},
    {"setbp", set_breakpoint, "Set breakpoint at address. usage: setbp <address>"},
    {"load", load_file, "Load object file. usage: load <filename>"},
    {"setspeed", set_speed, "Set execution speed in kHz. usage: setspeed <kHz>"},
    {"getspeed", get_speed, "Get current execution speed."},
    {"showbps", show_bps, "Show all breakpoints."},
    { NULL, NULL, NULL }
};

action_fn find_action(const char* name) 
{
    int i = 0;
    while (actions[i].name != NULL)
    {
        if (!strcmp(actions[i].name, name)) return actions[i].func;
        i++;
    }
    return NULL;
}

void repl(Machine& machine)
{
    char line[256];

    char** argv = (char**)calloc(16, sizeof(char*));
    for (int i = 0; i < 16; i++)
    {
        argv[i] = (char*)calloc(32, sizeof(char));
    }
    while (1) 
    {
        std::cout << "\nsim> ";
        for (int i = 0; i < 16; i++) argv[i][0] = '\0';
        
        fgets(line, sizeof(line), stdin);
        char* token = strtok(line, " \n\t");
        size_t i = 0;
        while (token != NULL) 
        {
            strcpy(argv[i++], token);
            token = strtok(NULL, " \n\t");
        }

        action_fn action = find_action(argv[0]);
        if (action == NULL) 
        {
            printf("invalid command.\n\n");
        }
        else
        {
            const char** argv_c = (const char **)argv;
            action(argv_c, machine);
        }
    }
}

int main(int argc, const char* argv[])
{
    struct sigaction sa{};
    sa.sa_handler = singalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0; 
    sigaction(SIGINT, &sa, nullptr);
    
    Machine machine;

    repl(machine);

    //machine.set_mem(0x0, 0x028102); // LDA #0x102
    //machine.set_mem(0x3, 0x3C0006); // JSUB 0x6

    //machine.dissasemble(0, 15);


    return 0;
}