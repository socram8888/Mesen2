#include "stdafx.h"
#include "Shared/ShortcutKeyHandler.h"
#include "Shared/EmuSettings.h"
#include "Shared/KeyManager.h"
#include "Shared/Video/VideoDecoder.h"
#include "Shared/Emulator.h"
#include "Shared/RewindManager.h"
#include "Shared/NotificationManager.h"
#include "Shared/SaveStateManager.h"
#include "Shared/Movies/MovieManager.h"
#include "Netplay/GameClient.h"

ShortcutKeyHandler::ShortcutKeyHandler(shared_ptr<Emulator> emu)
{
	_emu = emu;
	_keySetIndex = 0;
	_isKeyUp = false;
	_repeatStarted = false;

	_stopThread = false;
	_thread = std::thread([=]() {
		while(!_stopThread) {
			ProcessKeys();
			std::this_thread::sleep_for(std::chrono::duration<int, std::milli>(50));
		}
	});
}

ShortcutKeyHandler::~ShortcutKeyHandler()
{
	_stopThread = true;
	_thread.join();
}

bool ShortcutKeyHandler::IsKeyPressed(EmulatorShortcut shortcut)
{
	KeyCombination keyComb = _emu->GetSettings()->GetShortcutKey(shortcut, _keySetIndex);
	vector<KeyCombination> supersets = _emu->GetSettings()->GetShortcutSupersets(shortcut, _keySetIndex);
	for(KeyCombination &superset : supersets) {
		if(IsKeyPressed(superset)) {
			//A superset is pressed, ignore this subset
			return false;
		}
	}

	//No supersets are pressed, check if all matching keys are pressed
	return IsKeyPressed(keyComb);
}

bool ShortcutKeyHandler::IsKeyPressed(KeyCombination comb)
{
	int keyCount = (comb.Key1 ? 1 : 0) + (comb.Key2 ? 1 : 0) + (comb.Key3 ? 1 : 0);

	if(keyCount == 0 || _pressedKeys.empty()) {
		return false;
	}

	return IsKeyPressed(comb.Key1) &&
		(comb.Key2 == 0 || IsKeyPressed(comb.Key2)) &&
		(comb.Key3 == 0 || IsKeyPressed(comb.Key3));
}

bool ShortcutKeyHandler::IsKeyPressed(uint32_t keyCode)
{
	return KeyManager::IsKeyPressed(keyCode);
}

bool ShortcutKeyHandler::DetectKeyPress(EmulatorShortcut shortcut)
{
	if(IsKeyPressed(shortcut)) {
		bool newlyPressed = _prevKeysDown[_keySetIndex].find((uint32_t)shortcut) == _prevKeysDown[_keySetIndex].end();
		_keysDown[_keySetIndex].emplace((uint32_t)shortcut);

		if(newlyPressed && !_isKeyUp) {
			return true;
		}
	}
	return false;
}

bool ShortcutKeyHandler::DetectKeyRelease(EmulatorShortcut shortcut)
{
	if(!IsKeyPressed(shortcut)) {
		if(_prevKeysDown[_keySetIndex].find((uint32_t)shortcut) != _prevKeysDown[_keySetIndex].end()) {
			return true;
		}
	}
	return false;
}

void ShortcutKeyHandler::ProcessRunSingleFrame()
{
	shared_ptr<Timer> timer = _runSingleFrameRepeatTimer;
	if(!timer) {
		timer.reset(new Timer());
		_runSingleFrameRepeatTimer = timer;
	}
	timer->Reset();

	_emu->PauseOnNextFrame();
}

bool ShortcutKeyHandler::IsShortcutAllowed(EmulatorShortcut shortcut)
{
	bool isRunning = _emu->IsRunning();
	bool isNetplayClient = GameClient::Connected();
	bool isMoviePlaying = _emu->GetMovieManager()->Playing();
	bool isMovieRecording = _emu->GetMovieManager()->Recording();
	bool isMovieActive = isMoviePlaying || isMovieRecording;

	switch(shortcut) {
		case EmulatorShortcut::ToggleRewind:
		case EmulatorShortcut::Rewind:
		case EmulatorShortcut::RewindTenSecs:
		case EmulatorShortcut::RewindOneMin:
			return isRunning && !isNetplayClient && !isMovieRecording;

		case EmulatorShortcut::IncreaseSpeed:
		case EmulatorShortcut::DecreaseSpeed:
		case EmulatorShortcut::MaxSpeed:
			return !isNetplayClient;

		case EmulatorShortcut::Reset:
		case EmulatorShortcut::PowerCycle:
		case EmulatorShortcut::ReloadRom:
			return isRunning && !isNetplayClient && !isMoviePlaying;

		case EmulatorShortcut::PowerOff:
			return isRunning && !isNetplayClient;

		case EmulatorShortcut::TakeScreenshot:
			return isRunning;

		case EmulatorShortcut::ToggleCheats:
			return !isNetplayClient && !isMovieActive;
	}

	return true;
}

void ShortcutKeyHandler::ProcessShortcutPressed(EmulatorShortcut shortcut)
{
	EmuSettings* settings = _emu->GetSettings();

	switch(shortcut) {
		case EmulatorShortcut::Pause:
			if(_emu->IsPaused()) {
				_emu->Resume();
			} else {
				_emu->Pause();
			}
			break;
		
		case EmulatorShortcut::Reset: _emu->Reset(); break;
		case EmulatorShortcut::PowerCycle: _emu->PowerCycle(); break;
		case EmulatorShortcut::ReloadRom: _emu->ReloadRom(false); break;
		case EmulatorShortcut::PowerOff: _emu->Stop(true); break;

		case EmulatorShortcut::FastForward: settings->SetFlag(EmulationFlags::Turbo); break;
		case EmulatorShortcut::ToggleFastForward:
			if(settings->CheckFlag(EmulationFlags::Turbo)) {
				settings->ClearFlag(EmulationFlags::Turbo);
			} else {
				settings->SetFlag(EmulationFlags::Turbo);
			}
			break;

		case EmulatorShortcut::SelectSaveSlot1: case EmulatorShortcut::SelectSaveSlot2: case EmulatorShortcut::SelectSaveSlot3: case EmulatorShortcut::SelectSaveSlot4: case EmulatorShortcut::SelectSaveSlot5:
		case EmulatorShortcut::SelectSaveSlot6: case EmulatorShortcut::SelectSaveSlot7: case EmulatorShortcut::SelectSaveSlot8: case EmulatorShortcut::SelectSaveSlot9: case EmulatorShortcut::SelectSaveSlot10:
			_emu->GetSaveStateManager()->SelectSaveSlot((int)shortcut - (int)EmulatorShortcut::SelectSaveSlot1 + 1);
			break;

		case EmulatorShortcut::SaveStateSlot1: case EmulatorShortcut::SaveStateSlot2: case EmulatorShortcut::SaveStateSlot3: case EmulatorShortcut::SaveStateSlot4: case EmulatorShortcut::SaveStateSlot5:
		case EmulatorShortcut::SaveStateSlot6: case EmulatorShortcut::SaveStateSlot7: case EmulatorShortcut::SaveStateSlot8: case EmulatorShortcut::SaveStateSlot9: case EmulatorShortcut::SaveStateSlot10:
			_emu->GetSaveStateManager()->SaveState((int)shortcut - (int)EmulatorShortcut::SaveStateSlot1 + 1);
			break;
		
		case EmulatorShortcut::LoadStateSlot1: case EmulatorShortcut::LoadStateSlot2: case EmulatorShortcut::LoadStateSlot3: case EmulatorShortcut::LoadStateSlot4: case EmulatorShortcut::LoadStateSlot5:
		case EmulatorShortcut::LoadStateSlot6: case EmulatorShortcut::LoadStateSlot7: case EmulatorShortcut::LoadStateSlot8: case EmulatorShortcut::LoadStateSlot9: case EmulatorShortcut::LoadStateSlot10:
			_emu->GetSaveStateManager()->LoadState((int)shortcut - (int)EmulatorShortcut::LoadStateSlot1 + 1);
			break;

		case EmulatorShortcut::MoveToNextStateSlot: _emu->GetSaveStateManager()->MoveToNextSlot(); break;
		case EmulatorShortcut::MoveToPreviousStateSlot: _emu->GetSaveStateManager()->MoveToPreviousSlot(); break;
		case EmulatorShortcut::SaveState: _emu->GetSaveStateManager()->SaveState(); break;
		case EmulatorShortcut::LoadState: _emu->GetSaveStateManager()->LoadState(); break;

		case EmulatorShortcut::RunSingleFrame: ProcessRunSingleFrame(); break;
		
		case EmulatorShortcut::ToggleRewind:
			if(_emu->GetRewindManager()->IsRewinding()) {
				_emu->GetRewindManager()->StopRewinding();
			} else {
				_emu->GetRewindManager()->StartRewinding();
			}
			break;

		case EmulatorShortcut::Rewind: _emu->GetRewindManager()->StartRewinding(); break;
		case EmulatorShortcut::RewindTenSecs: _emu->GetRewindManager()->RewindSeconds(10); break;
		case EmulatorShortcut::RewindOneMin: _emu->GetRewindManager()->RewindSeconds(60); break;
		
		default:
			//Anything else is managed by the UI
			break;
	}
}

void ShortcutKeyHandler::ProcessShortcutReleased(EmulatorShortcut shortcut)
{
	EmuSettings* settings = _emu->GetSettings();
	switch(shortcut) {
		case EmulatorShortcut::FastForward: settings->ClearFlag(EmulationFlags::Turbo); break;
		case EmulatorShortcut::Rewind: _emu->GetRewindManager()->StopRewinding(); break;
		
		case EmulatorShortcut::RunSingleFrame:
			_runSingleFrameRepeatTimer.reset();
			_repeatStarted = false;
			break;
	}
}

void ShortcutKeyHandler::CheckMappedKeys()
{
	EmuSettings* settings = _emu->GetSettings();

	//Let the UI handle these shortcuts
	for(uint64_t i = 0; i < (uint64_t)EmulatorShortcut::ShortcutCount; i++) {
		EmulatorShortcut shortcut = (EmulatorShortcut)i;
		if(DetectKeyPress(shortcut)) {
			if(!IsShortcutAllowed(shortcut)) {
				continue;
			}

			ExecuteShortcutParams params = {};
			params.Shortcut = shortcut;
			_emu->GetNotificationManager()->SendNotification(ConsoleNotificationType::ExecuteShortcut, &params);

			ProcessShortcutPressed(shortcut);
		} else if(DetectKeyRelease(shortcut)) {
			ExecuteShortcutParams params = {};
			params.Shortcut = shortcut;
			_emu->GetNotificationManager()->SendNotification(ConsoleNotificationType::ReleaseShortcut, &params);

			ProcessShortcutReleased(shortcut);
		}
	}
}

void ShortcutKeyHandler::ProcessKeys()
{
	if(!_emu->GetSettings()->IsInputEnabled()) {
		return;
	}

	auto lock = _lock.AcquireSafe();
	KeyManager::RefreshKeyState();

	_pressedKeys = KeyManager::GetPressedKeys();
	_isKeyUp = _pressedKeys.size() < _lastPressedKeys.size();

	bool noChange = false;
	if(_pressedKeys.size() == _lastPressedKeys.size()) {
		noChange = true;
		for(size_t i = 0; i < _pressedKeys.size(); i++) {
			if(_pressedKeys[i] != _lastPressedKeys[i]) {
				noChange = false;
				break;
			}
		}
	}

	if(!noChange) {
		//Only run this if the keys have changed
		for(int i = 0; i < 2; i++) {
			_keysDown[i].clear();
			_keySetIndex = i;
			CheckMappedKeys();
			_prevKeysDown[i] = _keysDown[i];
		}

		_lastPressedKeys = _pressedKeys;
	}

	shared_ptr<Timer> timer = _runSingleFrameRepeatTimer;
	if(timer) {
		double elapsedMs = timer->GetElapsedMS();
		if((_repeatStarted && elapsedMs >= 50) || (!_repeatStarted && elapsedMs >= 500)) {
			//Over 500ms has elapsed since the key was first pressed, or over 50ms since repeat mode started (20fps)
			//In this case, run another frame and pause again.
			_repeatStarted = true;
			ProcessRunSingleFrame();
		}
	}
}