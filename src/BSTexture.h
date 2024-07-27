#pragma once

namespace BSTextureIndex {
	namespace Manager {
		void Hooks_ProcessEvent();
		void Hooks_DecompressStreamedLoad();
		void Hooks_StartStreamingChunks();
	}

	class Index {
		std::uint64_t	unk00;
		void* unk08;
		std::uint32_t	unk10;
		std::uint32_t	unk14;
		std::uint64_t	unk18;
		RE::BSTArray<RE::BSTSmartPointer<RE::BSResource::Stream>>	unk20;
		RE::BSResource::ID	unk38[256];
	};

	void Overwrites_Instruction();
	void Hooks_Index_AddDataFile();
}

namespace BSTextureStreamer {
	namespace Manager {
		void Hooks_ProcessEvent();
		//void Hooks_DecompressStreamedLoad();
		void Hooks_StartStreamingChunks();
		void Hooks_BSGraphics_Renderer_CreateStreamingTexture();
		void Hooks_BSGraphics_CreateStreamingDDSTexture();
		//void Hooks_1CB9AC2();
		//void Hooks_ThreadProc();
	}
}
