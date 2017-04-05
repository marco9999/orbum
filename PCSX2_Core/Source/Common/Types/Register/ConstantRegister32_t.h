#pragma once

#include "Common/Global/Globals.h"
#include "Common/Types/Register/Register32_t.h"

/*
ConstantRegister32_t is used as a register with a constant value. Any writes are discarded, and any reads are the value set at creation.
By default set to all 0.
*/

class ConstantRegister32_t : public Register32_t
{
public:
	ConstantRegister32_t(const u32 W = 0);

	/*
	Overwritten write functions that discard any value.
	*/
	void writeByte(const System_t context, size_t arrayIndex, u8 value) override;
	void writeHword(const System_t context, size_t arrayIndex, u16 value) override;
	void writeWord(const System_t context, u32 value) override;

	/*
	Initalise register (set to do nothing).
	*/
	void initalise() override;
};
