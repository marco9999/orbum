#include "stdafx.h"

#include "Common/Global/Globals.h"
#include "Common/Types/Register/ConstantRegister16_t.h"

ConstantRegister16_t::ConstantRegister16_t(const u16 H)
{
	UH = H;
}

void ConstantRegister16_t::writeByte(const System_t context, size_t arrayIndex, u8 value)
{
	// Do not write value (zero register).
}

void ConstantRegister16_t::writeHword(const System_t context, u16 value)
{
	// Do not write value (zero register).
}

void ConstantRegister16_t::initalise()
{
}