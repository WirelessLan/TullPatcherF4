#include "ArmorAddons.h"
#include "Armors.h"
#include "Cells.h"
#include "CombatStyles.h"
#include "Containers.h"
#include "CObjs.h"
#include "DefaultObjectManagers.h"
#include "FormLists.h"
#include "ImageSpaceAdapters.h"
#include "Ingestibles.h"
#include "Keywords.h"
#include "LeveledLists.h"
#include "Locations.h"
#include "MusicTypes.h"
#include "NPCs.h"
#include "ObjectModifications.h"
#include "Outfits.h"
#include "Quests.h"
#include "Races.h"
#include "Weapons.h"
#include "Worldspaces.h"

void Patch() {
	auto patchStart = std::chrono::high_resolution_clock::now();

	ArmorAddons::Patch();
	Armors::Patch();
	Cells::Patch();
	CombatStyles::Patch();
	Containers::Patch();
	CObjs::Patch();
	DefaultObjectManagers::Patch();
	FormLists::Patch();
	ImageSpaceAdapters::Patch();
	Ingestibles::Patch();
	Keywords::Patch();
	LeveledLists::Patch();
	Locations::Patch();
	MusicTypes::Patch();
	ObjectModifications::Patch();
	Outfits::Patch();
	Quests::Patch();
	Races::Patch();
	Weapons::Patch();
	WorldSpaces::Patch();

	auto patchEnd = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> patchDuration = patchEnd - patchStart;

	logger::info("Patch execution time: {} seconds", patchDuration.count());
}

void OnF4SEMessage(F4SE::MessagingInterface::Message* msg) {
	switch (msg->type) {
	case F4SE::MessagingInterface::kGameDataReady:
		Patch();
		break;
	}
}

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Query(const F4SE::QueryInterface * a_f4se, F4SE::PluginInfo * a_info) {
#ifndef NDEBUG
	auto sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
#else
	auto path = logger::log_directory();
	if (!path) {
		return false;
	}

	*path /= fmt::format("{}.log", Version::PROJECT);
	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
#endif

	auto log = std::make_shared<spdlog::logger>("Global Log"s, std::move(sink));

#ifndef NDEBUG
	log->set_level(spdlog::level::trace);
#else
	log->set_level(spdlog::level::info);
	log->flush_on(spdlog::level::trace);
#endif

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("[%^%l%$] %v"s);

	logger::info("{} v{}", Version::PROJECT, Version::NAME);

	a_info->infoVersion = F4SE::PluginInfo::kVersion;
	a_info->name = Version::PROJECT.data();
	a_info->version = Version::MAJOR;

	if (a_f4se->IsEditor()) {
		logger::critical("loaded in editor");
		return false;
	}

	const auto ver = a_f4se->RuntimeVersion();
	if (ver < F4SE::RUNTIME_1_10_162) {
		logger::critical("unsupported runtime v{}", ver.string());
		return false;
	}

	return true;
}

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Load(const F4SE::LoadInterface * a_f4se) {
	F4SE::Init(a_f4se);

	auto readConfigStart = std::chrono::high_resolution_clock::now();

	ArmorAddons::ReadConfigs();
	Armors::ReadConfigs();
	Cells::ReadConfigs();
	CombatStyles::ReadConfigs();
	Containers::ReadConfigs();
	CObjs::ReadConfigs();
	DefaultObjectManagers::ReadConfigs();
	FormLists::ReadConfigs();
	ImageSpaceAdapters::ReadConfigs();
	Ingestibles::ReadConfigs();
	Keywords::ReadConfigs();
	LeveledLists::ReadConfigs();
	Locations::ReadConfigs();
	MusicTypes::ReadConfigs();
	NPCs::ReadConfigs();
	ObjectModifications::ReadConfigs();
	Outfits::ReadConfigs();
	Quests::ReadConfigs();
	Races::ReadConfigs();
	Weapons::ReadConfigs();
	WorldSpaces::ReadConfigs();

	auto readConfigEnd = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> readConfigDuration = readConfigEnd - readConfigStart;

	logger::info("ReadConfig execution time: {} seconds", readConfigDuration.count());

	NPCs::Install();

	const F4SE::MessagingInterface* message = F4SE::GetMessagingInterface();
	if (message)
		message->RegisterListener(OnF4SEMessage);

	return true;
}
