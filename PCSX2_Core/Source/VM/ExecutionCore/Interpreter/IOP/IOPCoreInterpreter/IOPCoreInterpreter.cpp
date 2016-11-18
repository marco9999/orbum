#include "stdafx.h"

#include "VM/ExecutionCore/Interpreter/IOP/IOPCoreInterpreter/IOPCoreInterpreter.h"
#include "VM/VMMain.h"
#include "VM/ExecutionCore/Interpreter/IOP/IOPCoreInterpreter/IOPCoreMMUHandler/IOPCoreMMUHandler.h"
#include "VM/ExecutionCore/Interpreter/IOP/IOPCoreInterpreter/IOPCoreExceptionHandler/IOPCoreExceptionHandler.h"
#include "PS2Resources/PS2Resources_t.h"
#include "PS2Resources/IOP/IOP_t.h"
#include "PS2Resources/IOP/IOPCore/IOPCore_t.h"
#include "PS2Resources/IOP/IOPCore/Types/IOPCoreExceptions_t.h"
#include "PS2Resources/IOP/IOPCore/Types/IOPCoreException_t.h"
#include "PS2Resources/IOP/IOPCore/Types/IOPCoreR3000_t.h"
#include "Common/Types/Registers/PCRegister32_t.h"
#include "Common/Tables/IOPCoreInstructionTable/IOPCoreInstructionTable.h"
#include "Common/Types/MIPSInstruction/MIPSInstruction_t.h"
#include "Common/Types/MIPSInstructionInfo/MIPSInstructionInfo_t.h"
#include "Common/Util/MathUtil/MathUtil.h"

IOPCoreInterpreter::IOPCoreInterpreter(VMMain * vmMain) :
	VMExecutionCoreComponent(vmMain),
	mClockSources{ ClockSource_t::IOP },
	mExceptionHandler(std::make_unique<IOPCoreExceptionHandler>(vmMain)),
	mMMUHandler(std::make_unique<IOPCoreMMUHandler>(vmMain)),
	mInstructionInfo(nullptr)
{
}

const std::vector<ClockSource_t>& IOPCoreInterpreter::getClockSources()
{
	return mClockSources;
}

void IOPCoreInterpreter::initalise()
{
	// A Reset is done by raising a Reset exception.
	mExceptionHandler->handleException(IOPCoreException_t(IOPCoreException_t::ExType::EX_RESET));
}

s64 IOPCoreInterpreter::executionStep(const ClockSource_t & clockSource)
{
	// Check the exception queue to see if any are queued up - handle them first before executing an instruction (since the PC will change). 
	mExceptionHandler->checkExceptionState();

	// Check if in a branch delay slot - function will set the PC automatically to the correct location.
	checkBranchDelaySlot();

	// Perform instruction related activities (such as execute instruction, increment PC and update timers).
	s64 cycles = executeInstruction();

#if defined(BUILD_DEBUG)
	// Debug increment loop counter.
	DEBUG_LOOP_COUNTER++;
#endif

	// Return the number of cycles, based upon how long the instruction would have taken to complete.
	return cycles;
}

void IOPCoreInterpreter::checkBranchDelaySlot() const
{
	auto& R3000 = getVM()->getResources()->IOP->IOPCore->R3000;
	if (R3000->mIsInBranchDelay)
	{
		if (R3000->mBranchDelayCycles == 0)
		{
			R3000->PC->setPCValueAbsolute(R3000->mBranchDelayPCTarget);
			R3000->mIsInBranchDelay = false;
		}
		else
			R3000->mBranchDelayCycles--;
	}
}

s64 IOPCoreInterpreter::executeInstruction()
{
	auto& IOPCore = getVM()->getResources()->IOP->IOPCore;

	// Set the instruction holder to the instruction at the current PC.
	const u32 & instructionValue = mMMUHandler->readWordU(IOPCore->R3000->PC->getPCValue()); // TODO: Add error checking for address bus error.
	mInstruction.setInstructionValue(instructionValue);

	// Get the instruction details
	mInstructionInfo = IOPCoreInstructionTable::getInstructionInfo(mInstruction);

#if (0) // defined(BUILD_DEBUG)
	static u64 DEBUG_LOOP_BREAKPOINT = 0x36a8c; //0x2aa05;
	static u32 DEBUG_PC_BREAKPOINT = 0xBFC4a39c;

	if (DEBUG_LOOP_COUNTER >= DEBUG_LOOP_BREAKPOINT)
	{
		// Debug print details.
		logDebug("(%s, %d) IOPCore loop 0x%llX: "
			"PC = 0x%08X, BD = %d, "
			"Instruction = %s",
			__FILENAME__, __LINE__, DEBUG_LOOP_COUNTER,
			IOPCore->R3000->PC->getPCValue(), IOPCore->R3000->mIsInBranchDelay, 
			(instructionValue == 0) ? "SLL (NOP)" : mInstructionInfo->mMnemonic);

		if (IOPCore->R3000->PC->getPCValue() == DEBUG_PC_BREAKPOINT)
		{
			logDebug("(%s, %d) Breakpoint hit.", __FILENAME__, __LINE__);
		}
	}

	if (IOPCore->R3000->PC->getPCValue() == 0x00000890)
	{
		logDebug("(%s, %d) Stop condition hit.", __FILENAME__, __LINE__);
	}
#endif

	// Run the instruction, which is based on the implementation index.
 	(this->*IOP_INSTRUCTION_TABLE[mInstructionInfo->mImplementationIndex])();

	// Increment PC.
	IOPCore->R3000->PC->setPCValueNext();

	// Return the number of cycles the instruction took to complete.
	return mInstructionInfo->mCycles;
}

bool IOPCoreInterpreter::checkNoMMUError() const
{
	if (mMMUHandler->hasExceptionOccurred())
	{
		// Something went wrong in the MMU.
		auto& Exceptions = getVM()->getResources()->IOP->IOPCore->Exceptions;
		Exceptions->setException(mMMUHandler->getExceptionInfo());
		return false;
	}

	// MMU was ok.
	return true;
}

bool IOPCoreInterpreter::checkNoOverOrUnderflow32(const s32& x, const s32& y) const
{
	if (MathUtil::testOverOrUnderflow32(x, y))
	{
		// Over/Under flow occured.
		auto& Exceptions = getVM()->getResources()->IOP->IOPCore->Exceptions;
		Exceptions->setException(IOPCoreException_t(IOPCoreException_t::ExType::EX_OVERFLOW));
		return false;
	}

	// No error occured.
	return true;
}

void IOPCoreInterpreter::INSTRUCTION_UNKNOWN()
{
	// Unknown instruction, log if debug is enabled.
#if defined(BUILD_DEBUG)
	logDebug("(%s, %d) Unknown R3000 instruction encountered! (Lookup: %s -> %s)",
		__FILENAME__, __LINE__, mInstructionInfo->mBaseClass, mInstructionInfo->mMnemonic);
#endif
}

