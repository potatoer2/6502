#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <cstdint>

// 6502 proccesor emulation,
// some procceses are skiped for simplification issue

using Byte = uint8_t;
using Word = uint16_t;
using u32 = uint32_t;

namespace Instructions {
    static constexpr Byte
        INS_LDA_IM = 0xA9,
        INS_LDA_ZP = 0xA5,
        INS_LDA_ZPX = 0xB5,
        INS_ADC = 0x69,
        INS_AND = 0x29,
        INS_BCC = 0x90,
        INS_BCS = 0xB0,
        INS_BEQ = 0xF0,
        INS_BMI = 0x30,
        INS_BNE = 0xD0,
        INS_BPL = 0x10,
        INS_BVC = 0x50,
        INS_BVS = 0x70,
        INS_CLC = 0x18,
        INS_CLD = 0xD8,
        INS_CLI = 0x58,
        INS_CLV = 0xB8,
        INS_CMP = 0xC9,
        INS_DEC = 0xC6,
        INS_INC = 0xE6,
        INS_JSR = 0x20,
        INS_JMP = 0x4C,
        INS_RTS = 0x60,
        INS_PHA = 0x48,
        INS_PLA = 0x68,
        INS_NOP = 0xEA;
}

struct Mem {
    static constexpr u32 MAX_MEM = 1024 * 64;
    Byte Data[MAX_MEM];

    void Initialise() {
        for (u32 i = 0; i < MAX_MEM; i++) {
            Data[i] = 0; // SET MEMORY TO 0
        }
    }

    Byte operator[](u32 Address) const {
        return Data[Address];
    }

    Byte& operator[](u32 Address) {
        return Data[Address];
    }

    void WriteWord(u32& Cycle, Word DataToWrite, u32 Adress) {
        Data[Adress] = DataToWrite & 0xFF;
        Data[Adress + 1] = (DataToWrite >> 8);
        Cycle -= 2;
    }
};

using namespace Instructions;



struct CPU {
    Word PC; // program counter
    Word SP; // stack pointer

    Byte A, X, Y; // regs

    Byte C : 1; // carry
    Byte Z : 1; // zero
    Byte I : 1; // IR
    Byte D : 1; // decimal
    Byte B : 1; // break
    Byte V : 1; // overflow
    Byte N : 1; // negative


    void modifySP()
    {
        if (SP < 0x0100) {
            SP = 0x01FF;
        }
        else if (SP > 0x01FF) {
            SP = 0x0100;
        }
    }

    Byte FetchByte(u32& Cycles, Mem& memory) {
        Byte Data = memory[PC];
        PC++;
        Cycles--;
        return Data;
    }

    Word FetchWord(u32& Cycles, Mem& memory) {
        Word Data = memory[PC];
        PC++;
        Data |= (memory[PC] << 8);
        PC++;
        Cycles -= 2;
        return Data;
    }

    Byte ReadByte(u32& Cycles, Mem& memory, Byte Adress) {
        Byte Data = memory[Adress];
        Cycles--;
        return Data;
    }

    void Reset(Mem& memory) {
        PC = 0xFFFC;
        SP = 0x0100;
        C = Z = I = D = B = V = N = 0;
        A = X = Y = 0;
        memory.Initialise();
    }

    Word PullWord(u32& Cycles, Mem& memory) {
        Byte LowByte = memory[SP];
        SP++;
        modifySP();
        Byte HighByte = memory[SP];
        SP++;
        modifySP();
        Cycles -= 2;

        return static_cast<Word>(HighByte) << 8 | LowByte;
    }

    void ADCSetStatus(Byte Value) {
        u32 Result = A + Value + C;
        C = (Result > 0xFF);
        Z = ((Result & 0xFF) == 0);
        N = ((Result & 0x80) != 0);
        V = (~(A ^ Value) & (A ^ Result) & 0x80) != 0;
        A = Result & 0xFF;
    }

    void ExecuteBranch(u32& Cycles, Mem& memory, bool Condition) {
        Byte Offset = FetchByte(Cycles, memory);
        if (Condition) {
            Word TargetPC = PC + static_cast<int8_t>(Offset);
            Cycles--;
            if ((PC & 0xFF00) != (TargetPC & 0xFF00)) {
                Cycles--;
            }
            PC = TargetPC;
        }
    }

    void LDASetStatus() {
        Z = (A == 0);
        N = (A & 0b10000000) > 0;
    }

    void AndSetStatus() {
        LDASetStatus();
    }

   
    void Execute(u32 Cycles, Mem& memory) {
        while (Cycles > 0) {
            Byte Instruction = FetchByte(Cycles, memory);
            switch (Instruction) {
            case INS_LDA_IM:
                A = FetchByte(Cycles, memory);
                LDASetStatus();
                break;
            case INS_LDA_ZP:
                A = ReadByte(Cycles, memory, FetchByte(Cycles, memory));
                LDASetStatus();
                break;
            case INS_LDA_ZPX:
                A = ReadByte(Cycles, memory, FetchByte(Cycles, memory) + X);
                Cycles--;
                LDASetStatus();
                break;
            case INS_JSR:
                memory.WriteWord(Cycles, PC - 1, SP - 2);
                SP -= 2;
                modifySP();
                PC = FetchWord(Cycles, memory);
                break;
            case INS_JMP:
                PC = FetchWord(Cycles, memory);
                Cycles--;
                break;
            case INS_ADC:
                ADCSetStatus(FetchByte(Cycles, memory));
                break;
            case INS_AND:
                A = A & FetchByte(Cycles, memory);
                AndSetStatus();
                break;
            case INS_BCC:
                ExecuteBranch(Cycles, memory, !C);
                break;
            case INS_BCS:
                ExecuteBranch(Cycles, memory, C);
                break;
            case INS_BEQ:
                ExecuteBranch(Cycles, memory, Z);
                break;
            case INS_BMI:
                ExecuteBranch(Cycles, memory, N);
                break;
            case INS_BNE:
                ExecuteBranch(Cycles, memory, !Z);
                break;
            case INS_BPL:
                ExecuteBranch(Cycles, memory, !N);
                break;
            case INS_BVC:
                ExecuteBranch(Cycles, memory, !V);
                break;
            case INS_BVS:
                ExecuteBranch(Cycles, memory, V);
                break;
            case INS_CLC:
                C = 0;
                break;
            case INS_CLD:
                D = 0;
                break;
            case INS_CLI:
                I = 0;
                break;
            case INS_CLV:
                V = 0;
                break;
            case INS_CMP:
                Byte Value = FetchByte(Cycles, memory);
                C = (A >= Value);
                Z = (A == Value);
                N = ((A - Value) & 0b10000000) > 0;
                break;
            case INS_DEC:
                Byte Address = FetchByte(Cycles, memory);
                memory[Address] = memory[Address] - 1;
                Z = (memory[Address] == 0);
                N = (memory[Address] & 0b10000000) > 0;
                Cycles -= 3;

                break;
            case INS_INC:
                memory[PC - 1] = FetchByte(Cycles, memory) + 1;
                Z = (memory[PC - 1] == 0);
                N = (memory[PC - 1] & 0b10000000) > 0;
                Cycles -= 3;
                break;
            case INS_NOP:
                PC++;
                break;
            case INS_PHA:
                SP--;
                modifySP();
                memory[SP] = A;
                PC++;
                Cycles -= 3;
                break;
            case INS_PLA:
                A = memory[SP];
                SP++;
                modifySP();
                PC++;
                Cycles -= 3;
                break;
            case INS_RTS:
                PC = PullWord(Cycles, memory) + 1;
                Cycles--;
                break;
            default:
                printf("Instruction not handled %d", Instruction);
                break;
            }
        }
    }
};
