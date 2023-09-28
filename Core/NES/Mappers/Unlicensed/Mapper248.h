#pragma once
#include "pch.h"
#include "NES/BaseMapper.h"
#include "Utilities/Serializer.h"

class Mapper248 : public BaseMapper
{
protected:
	uint8_t _prgReg = 0;

	uint16_t RegisterStartAddress() override { return 0xC000; }
	uint16_t RegisterEndAddress() override { return 0xFFFF; }
	uint16_t GetPrgPageSize() override { return 0x4000; }
	uint16_t GetChrPageSize() override { return 0x2000; }

	void InitMapper() override
	{
		SelectPrgPage(0, 0);
		SelectPrgPage(1, -1);
		SetNametables(0, 1, 1, 0);
	}

	void WriteRegister(uint16_t addr, uint8_t value) override
	{
		switch(addr & 0xE000) {
			case 0xC000:
				break;
			case 0xE000:
				_prgReg = (_prgReg >> 1 | value << 4) & 0x1F;
				SelectPrgPage(0, _prgReg);
				break;
		}
	}

	void Serialize(Serializer& s) override
	{
		BaseMapper::Serialize(s);
		SV(_prgReg);
	}
};
