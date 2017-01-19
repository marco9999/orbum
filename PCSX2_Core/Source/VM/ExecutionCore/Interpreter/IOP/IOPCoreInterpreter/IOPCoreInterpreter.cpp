#include "stdafx.h"

#include "Common/Types/Context_t.h"
#include "Common/Types/Registers/MIPS/PCRegister32_t.h"
#include "Common/Tables/IOPCoreInstructionTable/IOPCoreInstructionTable.h"
#include "Common/Types/MIPSInstruction/MIPSInstruction_t.h"
#include "Common/Types/MIPSInstructionInfo/MIPSInstructionInfo_t.h"
#include "Common/Types/MIPSBranchDelay/MIPSBranchDelay_t.h"
#include "Common/Util/MathUtil/MathUtil.h"

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
#include "PS2Resources/IOP/IOPCore/Types/IOPCoreCOP0_t.h"
#include "PS2Resources/IOP/IOPCore/Types/IOPCoreCOP0Registers_t.h"

IOPCoreInterpreter::IOPCoreInterpreter(VMMain * vmMain) :
	VMExecutionCoreComponent(vmMain),
	mExceptionHandler(std::make_unique<IOPCoreExceptionHandler>(vmMain)),
	mMMUHandler(std::make_unique<IOPCoreMMUHandler>(vmMain)),
	mInstructionInfo(nullptr)
{
}

void IOPCoreInterpreter::initalise()
{
	// A Reset is done by raising a Reset exception.
	mExceptionHandler->handleException(IOPCoreException_t::EX_RESET);
}

s64 IOPCoreInterpreter::executionStep(const ClockSource_t & clockSource)
{
	auto& IOPCore = getResources()->IOP->IOPCore;

	// Check if any external interrupts are pending and queue exception if there are.
	handleInterruptCheck();

	// Check the exception queue to see if any are queued up - handle them first before executing an instruction (since the PC will change). 
	if (IOPCore->Exceptions->hasExceptionOccurred())
		mExceptionHandler->handleException(IOPCore->Exceptions->getException());

	// Check if in a branch delay slot - function will set the PC automatically to the correct location.
	IOPCore->R3000->BD->handleBranchDelay();

	// Increment PC.
	IOPCore->R3000->PC->setPCValueNext();

	// Set the instruction holder to the instruction at the previous PC.
	const u32 pcValue = IOPCore->R3000->PC->readWord(Context_t::IOP) - Constants::SIZE_MIPS_INSTRUCTION;
	const u32 instructionValue = mMMUHandler->readWord(pcValue); // TODO: Add error checking for address bus error.
	mInstruction.setInstructionValue(instructionValue);

	// Get the instruction details.
	mInstructionInfo = IOPCoreInstructionTable::getInstructionInfo(mInstruction);

#if defined(BUILD_DEBUG)
	static u64 DEBUG_LOOP_BREAKPOINT = 0x10000028e000; // 1b42ae
	static u32 DEBUG_PC_BREAKPOINT = 0x0;
	static u32 DEBUG_INST_VAL_BREAKPOINT = 0x42000010; // COP0 RFE

	if (DEBUG_LOOP_COUNTER >= DEBUG_LOOP_BREAKPOINT)
	{
		// Debug print details.
		logDebug("IOPCore cycle 0x%llX: "
			"PC = 0x%08X, BD = %d, IEc = %d, "
			"Instruction = %s",
			DEBUG_LOOP_COUNTER,
			pcValue, IOPCore->R3000->BD->isInBranchDelay(), !IOPCore->COP0->Status->isInterruptsMasked(),
			(instructionValue == 0) ? "SLL (NOP)" : mInstructionInfo->mMnemonic);
	}

	if (pcValue == DEBUG_PC_BREAKPOINT)
	{
		logDebug("IOPCore Breakpoint hit @ cycle = 0x%llX.", DEBUG_LOOP_COUNTER);
	}
#endif

	// Run the instruction, which is based on the implementation index.
	(this->*IOP_INSTRUCTION_TABLE[mInstructionInfo->mImplementationIndex])();

#if defined(BUILD_DEBUG)
	// Debug increment loop counter.
	DEBUG_LOOP_COUNTER++;
#endif

	// Return the number of cycles the instruction took to complete.
	return mInstructionInfo->mCycles;
}

void IOPCoreInterpreter::handleInterruptCheck() const
{
	auto& COP0 = getResources()->IOP->IOPCore->COP0;

	// Interrupt exceptions are only taken when conditions are correct.
	if (!COP0->Status->isInterruptsMasked())
	{
		u32 ipCause = COP0->Cause->getFieldValue(IOPCoreCOP0Register_Cause_t::Fields::IP);
		u32 imStatus = COP0->Status->getFieldValue(IOPCoreCOP0Register_Status_t::Fields::IM);
		if (ipCause & imStatus)
		{
#if defined(BUILD_DEBUG)
			auto& IOPCore = getResources()->IOP->IOPCore;
			logDebug("IOP interrupt exception occurred @ cycle = 0x%llX, PC = 0x%08X, BD = %d.", 
				DEBUG_LOOP_COUNTER, IOPCore->R3000->PC->readWord(Context_t::IOP), IOPCore->R3000->BD->isInBranchDelay());
#endif
			auto& Exceptions = getResources()->IOP->IOPCore->Exceptions;
			Exceptions->setException(IOPCoreException_t::EX_INTERRUPT);
		}
	}
}

bool IOPCoreInterpreter::checkCOP0Usable() const
{
	if (!getResources()->IOP->IOPCore->COP0->isCoprocessorUsable())
	{
		// Coprocessor was not usable. Raise an exception.
		auto& COP0 = getResources()->IOP->IOPCore->COP0;
		auto& Exceptions = getResources()->IOP->IOPCore->Exceptions;
		COP0->Cause->setFieldValue(IOPCoreCOP0Register_Cause_t::Fields::CE, 0);
		Exceptions->setException(IOPCoreException_t::EX_COPROCESSOR_UNUSABLE);
		return false;
	}

	// Coprocessor is usable, proceed.
	return true;
}

bool IOPCoreInterpreter::checkNoMMUError() const
{
	if (mMMUHandler->hasExceptionOccurred())
	{
		// Something went wrong in the MMU.
		auto& Exceptions = getResources()->IOP->IOPCore->Exceptions;
		Exceptions->setException(mMMUHandler->getException());
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
		auto& Exceptions = getResources()->IOP->IOPCore->Exceptions;
		Exceptions->setException(IOPCoreException_t::EX_OVERFLOW);
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

