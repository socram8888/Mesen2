#pragma once
#include "stdafx.h"
#include "Debugger/DebugTypes.h"
#include "Debugger/IDebugger.h"

class PceConsole;
class Disassembler;
class Debugger;
class CallstackManager;
class MemoryAccessCounter;
class CodeDataLogger;
class EmuSettings;
class ScriptManager;
class BreakpointManager;
class IAssembler;
class BaseEventManager;
class PceTraceLogger;
class PcePpuTools;

class Emulator;
class PceCpu;
class PcePpu;
class PceMemoryManager;
class DummyPceCpu;

enum class MemoryOperationType;

class PceDebugger final : public IDebugger
{
	Debugger* _debugger;
	Emulator* _emu;
	Disassembler* _disassembler;
	MemoryAccessCounter* _memoryAccessCounter;
	EmuSettings* _settings;

	PceConsole* _console;
	PceCpu* _cpu;
	PcePpu* _ppu;
	PceMemoryManager* _memoryManager;

	unique_ptr<CodeDataLogger> _codeDataLogger;
	//unique_ptr<BaseEventManager> _eventManager;
	//unique_ptr<IAssembler> _assembler;
	unique_ptr<CallstackManager> _callstackManager;
	unique_ptr<BreakpointManager> _breakpointManager;
	unique_ptr<PceTraceLogger> _traceLogger;
	unique_ptr<StepRequest> _step;
	unique_ptr<PcePpuTools> _ppuTools;

	bool _enableBreakOnUninitRead = false;
	uint8_t _prevOpCode = 0xFF;
	uint32_t _prevProgramCounter = 0;

	unique_ptr<DummyPceCpu> _dummyCpu;

	bool IsRegister(MemoryOperationInfo& op);

public:
	PceDebugger(Debugger* debugger);

	void Reset() override;

	void ProcessInstruction();
	void ProcessRead(uint32_t addr, uint8_t value, MemoryOperationType type);
	void ProcessWrite(uint32_t addr, uint8_t value, MemoryOperationType type);
	void ProcessInterrupt(uint32_t originalPc, uint32_t currentPc, bool forNmi) override;
	void ProcessPpuRead(uint16_t addr, uint8_t value, MemoryType memoryType);
	void ProcessPpuWrite(uint16_t addr, uint8_t value, MemoryType memoryType);
	void ProcessPpuCycle();

	void Run() override;
	void Step(int32_t stepCount, StepType type) override;

	DebuggerFeatures GetSupportedFeatures() override;
	void SetProgramCounter(uint32_t addr) override;
	uint32_t GetProgramCounter(bool getInstPc) override;

	BreakpointManager* GetBreakpointManager() override;
	ITraceLogger* GetTraceLogger() override;
	PpuTools* GetPpuTools() override;
	void SaveRomToDisk(string filename, bool saveAsIps, CdlStripOption stripOption);
	CallstackManager* GetCallstackManager() override;
	IAssembler* GetAssembler() override;
	BaseEventManager* GetEventManager() override;
	CodeDataLogger* GetCodeDataLogger() override;

	BaseState& GetState() override;
	void GetPpuState(BaseState& state) override;
	void SetPpuState(BaseState& state) override;
};