#pragma once
#include "pch.h"
#include "NES/BaseMapper.h"
#include "Utilities/Serializer.h"

class Mapper248 : public BaseMapper
{
private:
	unique_ptr<FlashSST39SF040> _flash;
	vector<uint8_t> _orgPrgRom;

protected:
	uint8_t _prgBank = 0;

	uint16_t RegisterStartAddress() override { return 0x8000; }
	uint16_t RegisterEndAddress() override { return 0xFFFF; }
	uint16_t GetPrgPageSize() override { return 0x4000; }
	uint16_t GetChrPageSize() override { return 0x2000; }
	bool AllowRegisterRead() override { return true; }

	void InitMapper() override
	{
		_flash.reset(new FlashSST39SF040(_console, _prgRom, _prgSize));
		_orgPrgRom = vector<uint8_t>(_prgRom, _prgRom + _prgSize);
		ApplySaveData();

		SelectPrgPage(0, 0);
		SelectPrgPage(1, -1);

		if(GetMirroringType() != MirroringType::FourScreens) {
			// Diagonal mirroring unless four screen
			SetNametables(0, 1, 1, 0);
		}
	}

	uint8_t ReadRegister(uint16_t addr) override
	{
		int16_t value = _flash->Read(addr);
		if(value >= 0) {
			return (uint8_t)value;
		}

		return BaseMapper::InternalReadRam(addr);
	}

	void WriteRegister(uint16_t addr, uint8_t value) override
	{
		if(addr < 0xC000) {
			_flash->Write((addr & 0x3FFF) | (_prgBank << 14), value);
		} else if(addr >= 0xE000) {
			_prgBank = (_prgBank >> 1 | value << 4) & 0x1F;
			SelectPrgPage(0, _prgBank);
		}
	}

	void Serialize(Serializer& s) override
	{
		BaseMapper::Serialize(s);
		SV(_prgBank);

		if(s.IsSaving()) {
			vector<uint8_t> prgRom = vector<uint8_t>(_prgRom, _prgRom + _prgSize);
			vector<uint8_t> ipsData = IpsPatcher::CreatePatch(_orgPrgRom, prgRom);
			SVVector(ipsData);
		} else {
			vector<uint8_t> ipsData;
			SVVector(ipsData);

			vector<uint8_t> patchedPrgRom;
			if(IpsPatcher::PatchBuffer(ipsData, _orgPrgRom, patchedPrgRom)) {
				memcpy(_prgRom, patchedPrgRom.data(), _prgSize);
			}
		}
	}

	void ApplySaveData()
	{
		if(_console->GetNesConfig().DisableFlashSaves) {
			return;
		}

		//Apply save data (saved as an IPS file), if found
		vector<uint8_t> ipsData = _emu->GetBatteryManager()->LoadBattery(".ips");
		if(!ipsData.empty()) {
			vector<uint8_t> patchedPrgRom;
			if(IpsPatcher::PatchBuffer(ipsData, _orgPrgRom, patchedPrgRom)) {
				memcpy(_prgRom, patchedPrgRom.data(), _prgSize);
			}
		}
	}

	void SaveBattery() override
	{
		if(_console->GetNesConfig().DisableFlashSaves) {
			return;
		}

		vector<uint8_t> prgRom = vector<uint8_t>(_prgRom, _prgRom + _prgSize);
		vector<uint8_t> ipsData = IpsPatcher::CreatePatch(_orgPrgRom, prgRom);
		if(ipsData.size() > 8) {
			_emu->GetBatteryManager()->SaveBattery(".ips", ipsData.data(), (uint32_t)ipsData.size());
		}
	}
};
