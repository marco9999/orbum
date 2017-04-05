﻿#pragma once

#include <string>

#include "Common/Global/Globals.h"
#include "Common/Types/System_t.h"

/*
Register type and size definitions.
Register8, Register16, Register32, Register64 and Register128 define the base register sizes used thoughout the emulator.

These registers are implemented (at core) as a union of the unsigned sums (to the register size) of these types.
However, they should be accessed by the read/write functions instead to account for overriden functionality.

TODO: Check byte order / endianess - it is assumed within the emulator that, for example in a 128-bit register:
 Register128_t.UW[0] = bits 0-31.    Least significant bits
 Register128_t.UW[1] = bits 32-63.              |
 Register128_t.UW[2] = bits 64-95.              ↓
 Register128_t.UW[3] = bits 96-127.  Most significant bits

The primary difference between a Register type and Memory type is that registers are accessed by the size index, not by the byte index.
For example, a writeWordU(0, value) will write to the first 0-31 bits, while writeWord(1, value) will write to bits 32-63.
See the Memory type for the counter-example.
*/
class Register8_t
{
public:
	// Initialise union with 0 value.
	explicit Register8_t();
	explicit Register8_t(const char * mnemonic);

#if defined(BUILD_DEBUG)
	explicit Register8_t(const char * mnemonic, bool debugReads, bool debugWrites); // Turn on/off debugging functionality.
	bool mDebugReads, mDebugWrites;
#endif

	virtual ~Register8_t() = default;

	/*
	Read/write functions to access the register, with subclassed functionality. 
	TODO: can get small speedup by separating the VM context into own direct function ( readTypeRaw() / writeTypeRaw() ) if needed.
	*/
	virtual u8 readByte(const System_t context);
	virtual void writeByte(const System_t context, u8 value);

	/*
	Gets the mnemonic of this register. Used for debug/logging.
	*/
	const char * getMnemonic() const;

	/*
	Initalise register (set back to default value).
	*/
	virtual void initalise();

protected:
	/*
	Underlying storage for register.
	*/
	union
	{
		u8 UB;
	};

private:
	/*
	The mnemonic.
	*/
	std::string mMnemonic;
};