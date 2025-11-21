#include <iostream>
#include <vector>

#define MAX_ADDRESS 1000000

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

class Machine
{
private:
    std::vector<Register> regs;
    FloatRegister float_reg;

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
        this->regs.push_back(Register(SW));
        this->float_reg = FloatRegister(F);
    }
};

class RAM 
{
private:
    std::vector<unsigned char> memory;

public:

    RAM() {}

    unsigned char get_byte(unsigned int address)
    {
        return this->memory[address];
    }

    void set_byte(unsigned int address, unsigned char value)
    {
        this->memory[address] = value;
    }

    unsigned int get_word(unsigned int address)
    {
        return (this->memory[address] | (this->memory[address] << 8) | (this->memory[address] << 16));
    }

    void set_word(unsigned int address, unsigned int value)
    {
        this->memory[address] = value & 0xFF;
        this->memory[address + 1] = value & 0xFF00;
        this->memory[address + 2] = value & 0xFF0000;
    } 
};

int main()
{

}