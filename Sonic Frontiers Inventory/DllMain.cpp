#include "Pch.h"
#include <rangers-api/bootstrap.h>
#include "Localization.h"
#include "UIInventory.h"
#pragma pointers_to_members(full_generality)

using namespace mods::inventory;
using namespace mods::inventory::ui;

static hh::fnd::Reference<hh::fnd::ResourceLoader> resourceLoader;

static void* baseAddress;

typedef void (*InitializeCockpitPtr)(app::game::GameModeStage::State::RebuildLevel* self, app::game::GameModeStage* gameMode);

InitializeCockpitPtr pInitializeCockpit;

void InitializeCockpitDetour(app::game::GameModeStage::State::RebuildLevel* self, app::game::GameModeStage* gameMode) {
	//pInitializeCockpit(self, gameMode);

	auto uiInventory = UIInventory::GetClass().Instantiate<UIInventory>(self->GetAllocator());

	gameMode->gameManager->RegisterNamedObject(uiInventory, nullptr, false, nullptr, nullptr);
}

typedef uint64_t (*GameModeBootInitPtr)(void* gameModeBoot);

GameModeBootInitPtr pGameModeBootInit;

uint64_t GameModeBootInitDetour(void* gameModeBoot) {
	auto res = pGameModeBootInit(gameModeBoot);

	//auto allocators = hh::fnd::GetAllocatorSystem();
	auto allocator = app::fnd::AppHeapManager::GetResidentAllocator();
	resourceLoader = new (allocator) hh::fnd::ResourceLoader(allocator);
	resourceLoader->LoadPackfile("mods/inventory/inventory.pac", true);

	return res;
}

BOOL WINAPI DllMain(_In_ HINSTANCE hInstance, _In_ DWORD reason, _In_ LPVOID reserved)
{
	switch (reason)
	{
		case DLL_PROCESS_ATTACH:
			{
				baseAddress = GetModuleHandle(nullptr);

				rangersapi::bootstrap::SetBaseAddress(baseAddress);

				pInitializeCockpit = reinterpret_cast<InitializeCockpitPtr>(reinterpret_cast<size_t>(baseAddress) + 0x7D0F2B0);
				pGameModeBootInit = reinterpret_cast<GameModeBootInitPtr>(reinterpret_cast<size_t>(baseAddress) + 0x76832E0);
			}

			DetourTransactionBegin();
			DetourUpdateThread(GetCurrentThread());
			DetourAttach(&(PVOID&)pInitializeCockpit, &InitializeCockpitDetour);
			DetourAttach(&(PVOID&)pGameModeBootInit, &GameModeBootInitDetour);
			DetourTransactionCommit();
			break;
		case DLL_PROCESS_DETACH:
			DetourTransactionBegin();
			DetourUpdateThread(GetCurrentThread());
			DetourDetach(&(PVOID&)pInitializeCockpit, &InitializeCockpitDetour);
			DetourDetach(&(PVOID&)pGameModeBootInit, &GameModeBootInitDetour);
			DetourTransactionCommit();
			break;
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
			break;
	}

	return TRUE;
}
