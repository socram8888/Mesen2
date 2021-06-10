#pragma once
#include "stdafx.h"
#include "Debugger/DebugTypes.h"
#include "Debugger/IEventManager.h"
#include "Utilities/SimpleLock.h"

enum class DebugEventType;
struct DebugEventInfo;
class GbCpu;
class GbPpu;
class Debugger;

struct GbEventViewerConfig : public BaseEventViewerConfig
{
	EventViewerCategoryCfg Irq;
	EventViewerCategoryCfg MarkedBreakpoints;

	EventViewerCategoryCfg PpuRegisterReads;
	EventViewerCategoryCfg PpuRegisterCgramWrites;
	EventViewerCategoryCfg PpuRegisterVramWrites;
	EventViewerCategoryCfg PpuRegisterOamWrites;
	EventViewerCategoryCfg PpuRegisterBgScrollWrites;
	EventViewerCategoryCfg PpuRegisterWindowWrites;
	EventViewerCategoryCfg PpuRegisterOtherWrites;

	EventViewerCategoryCfg ApuRegisterReads;
	EventViewerCategoryCfg ApuRegisterWrites;
	EventViewerCategoryCfg CpuRegisterReads;
	EventViewerCategoryCfg CpuRegisterWrites;

	bool ShowPreviousFrameEvents;
};

class GbEventManager final : public IEventManager
{
private:
	static constexpr int ScanlineWidth = 456*2;
	static constexpr int ScreenHeight = 154;
	static constexpr int VBlankScanline = 144;

	GbEventViewerConfig _config;

	GbPpu* _ppu;
	GbCpu* _cpu;
	Debugger* _debugger;

	vector<DebugEventInfo> _debugEvents;
	vector<DebugEventInfo> _prevDebugEvents;
	vector<DebugEventInfo> _sentEvents;

	vector<DebugEventInfo> _snapshot;
	int16_t _snapshotScanline = -1;
	uint16_t _snapshotCycle = 0;
	SimpleLock _lock;

	uint32_t _scanlineCount = GbEventManager::ScreenHeight;
	uint16_t* _ppuBuffer = nullptr;

	void DrawEvent(DebugEventInfo& evt, bool drawBackground, uint32_t* buffer);
	void FilterEvents();

public:
	GbEventManager(Debugger* debugger, GbCpu* cpu, GbPpu* ppu);
	~GbEventManager();

	void AddEvent(DebugEventType type, MemoryOperationInfo& operation, int32_t breakpointId = -1);
	void AddEvent(DebugEventType type);

	void GetEvents(DebugEventInfo* eventArray, uint32_t& maxEventCount);
	uint32_t GetEventCount();
	void ClearFrameEvents();

	EventViewerCategoryCfg GetEventConfig(DebugEventInfo& evt);

	uint32_t TakeEventSnapshot();
	void GetDisplayBuffer(uint32_t* buffer, uint32_t bufferSize);
	DebugEventInfo GetEvent(uint16_t scanline, uint16_t cycle);

	FrameInfo GetDisplayBufferSize() override;
	void SetConfiguration(BaseEventViewerConfig& config) override;
};
