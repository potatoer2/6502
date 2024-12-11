#include "6502.h"
using namespace Instructions;



int main()
{
	Mem mem;
	CPU cpu;
	cpu.Reset(mem);

	//
	mem[0xFFFC] = INS_LDA_IM;
	mem[0xFFFD] = 0x42;
	//

	cpu.Execute( 3 , mem);
	return 0;
}