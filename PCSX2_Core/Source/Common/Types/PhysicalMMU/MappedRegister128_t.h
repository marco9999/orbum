#pragma once

#include <memory>

#include "Common/Global/Globals.h"
#include "Common/Interfaces/PhysicalMapped.h"
#include "Common/Types/Registers/Register128_t.h"

/*
A transition layer, allowing a register to be mapped into the PS2 physical memory space.
Ie: translates from byte index accesses into size index accesses.
If the access size is smaller than the register size, but aligned on a boundary, then the appropriate array index will be accessed in the register.
For example, for a 32-bit register mapped @ 0x1F801470 accessed by readHword(0x1F801472) will return Register32->UH[1].
If the access is not aligned on the appropriate boundary, then a runtime_error is thrown.
If the access size is larger than the register size, a runtime_error is thrown.
TODO: look into size conditions - the EE manual mentions "... only accessible by word ...", but this allows any size.
*/
class MappedRegister128_t : public PhysicalMapped
{
public:
	explicit MappedRegister128_t(const u32& physicalAddress, const std::shared_ptr<Register128_t> & register128);
	virtual ~MappedRegister128_t();

	u8 readByte(u32 storageIndex) override;
	void writeByte(u32 storageIndex, u8 value) override;
	u16 readHword(u32 storageIndex) override;
	void writeHword(u32 storageIndex, u16 value) override;
	u32 readWord(u32 storageIndex) override;
	void writeWord(u32 storageIndex, u32 value) override;
	u64 readDword(u32 storageIndex) override;
	void writeDword(u32 storageIndex, u64 value) override;
	u128 readQword(u32 storageIndex) override;
	void writeQword(u32 storageIndex, u128 value) override;

	/*
	Gets the storage length, needed by the Physical MMU handler in order to map it.
	*/
	size_t getSize() override;

	/*
	Get the storage mnemonic, used for debug.
	*/
	const char * getMnemonic() const override;

private:
	/*
	The underlying register this class maps to.
	*/
	const std::shared_ptr<Register128_t> mRegister128;
};