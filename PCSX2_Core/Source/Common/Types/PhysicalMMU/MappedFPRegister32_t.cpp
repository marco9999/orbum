#include "stdafx.h"

#include <memory>

#include "Common/Global/Globals.h"
#include "Common/Types/PhysicalMMU/MappedFPRegister32_t.h"

MappedFPRegister32_t::MappedFPRegister32_t(const u32& physicalAddress, const std::shared_ptr<FPRegister32_t> & fpRegister32) :
	PhysicalMapped(physicalAddress),
	mFPRegister32(fpRegister32)
{
}

MappedFPRegister32_t::~MappedFPRegister32_t()
{
}

size_t MappedFPRegister32_t::getSize()
{
	return Constants::NUMBER_BYTES_IN_QWORD;
}

const char* MappedFPRegister32_t::getMnemonic() const
{
	return mFPRegister32->getMnemonic();
}

u8 MappedFPRegister32_t::readByte(u32 storageIndex)
{
	throw std::runtime_error("Tried to access MappedFPRegister32_t with smaller than allowed size. Not allowed.");
}

void MappedFPRegister32_t::writeByte(u32 storageIndex, u8 value)
{
	throw std::runtime_error("Tried to access MappedFPRegister32_t with smaller than allowed size. Not allowed.");
}

u16 MappedFPRegister32_t::readHword(u32 storageIndex)
{
	throw std::runtime_error("Tried to access MappedFPRegister32_t with smaller than allowed size. Not allowed.");
}

void MappedFPRegister32_t::writeHword(u32 storageIndex, u16 value)
{
	throw std::runtime_error("Tried to access MappedFPRegister32_t with smaller than allowed size. Not allowed.");
}

u32 MappedFPRegister32_t::readWord(u32 storageIndex)
{
	if (storageIndex % Constants::NUMBER_BYTES_IN_WORD != 0)
		throw std::runtime_error("Tried to access MappedFPRegister32_t with unaligned index. Not allowed.");
	return mFPRegister32->readWord();
}

void MappedFPRegister32_t::writeWord(u32 storageIndex, u32 value)
{
	if (storageIndex % Constants::NUMBER_BYTES_IN_WORD != 0)
		throw std::runtime_error("Tried to access MappedFPRegister32_t with unaligned index. Not allowed.");
	mFPRegister32->writeWord(value);
}

u64 MappedFPRegister32_t::readDword(u32 storageIndex)
{
	throw std::runtime_error("Tried to access MappedFPRegister32_t with larger than allowed size. Not allowed.");
}

void MappedFPRegister32_t::writeDword(u32 storageIndex, u64 value)
{
	throw std::runtime_error("Tried to access MappedFPRegister32_t with larger than allowed size. Not allowed.");
}

u128 MappedFPRegister32_t::readQword(u32 storageIndex)
{
	throw std::runtime_error("Tried to access MappedFPRegister32_t with larger than allowed size. Not allowed.");
}

void MappedFPRegister32_t::writeQword(u32 storageIndex, u128 value)
{
	throw std::runtime_error("Tried to access MappedFPRegister32_t with larger than allowed size. Not allowed.");
}
