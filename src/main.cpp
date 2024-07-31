#include "BSResource.h"
#include "BSTexture.h"
#include "BSScaleformImageLoader.h"

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
	F4SE::AllocTrampoline(static_cast<size_t>(1) << 12u);

	BSResource::Archive2::Hooks_Index_AddDataFile();
	BSResource::Archive2::Hooks_Index_AddDataFromReader();

	BSResource::SDirectory2::Hooks_Retrieve();
	BSResource::SDirectory2::Hooks_ReplicateDirTo();

	BSTextureIndex::Hooks_Index_AddDataFile();
	BSTextureIndex::Overwrites_Instruction();

	BSTextureStreamer::Manager::Hooks_ProcessEvent();
	BSTextureStreamer::Manager::Hooks_LoadChunks();
	BSTextureStreamer::Manager::Hooks_StartStreamingChunks();
	BSTextureStreamer::Manager::Hooks_BSGraphics_Renderer_CreateStreamingTexture();
	BSTextureStreamer::Manager::Hooks_BSGraphics_CreateStreamingDDSTexture();
	BSTextureStreamer::Manager::Hooks_ThreadProc();

	BSScaleformImageLoader::Hooks_LoadProtocolImage();

	return true;
}
