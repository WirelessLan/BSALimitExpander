#pragma once

namespace RE {
	namespace BSGraphics {
		class Texture {
		public:
			struct Data {
				std::uint32_t refCount;			// 00
				std::uint8_t dataFileIndex;		// 04
				std::uint8_t chunkCount;		// 05
				std::uint8_t chunkOffsetOrType;	// 06
				std::uint8_t dataFileHighIndex;	// 07
			};

			std::uint64_t unk00;
			std::uint64_t unk08;
			std::uint64_t unk10;
			Data* data;
		};
	}
}

namespace BSTextureIndex {
	void Overwrites_Instruction();
	void Hooks_Index_AddDataFile();
}

namespace BSTextureStreamer {
	class TextureRequest {
	public:
		RE::BSResource::Archive2::Index::EntryHeader header;	// 00
		std::uint64_t unk10[(0x78 - 0x10) >> 3];				// 10
		RE::BSFixedString unk78;								// 78
		std::uint64_t unk80[(0xC8 - 0x80) >> 3];				// 80
		RE::NiTexture* niTexture;								// C8
		RE::BSFixedString texturePath;							// D0
		std::uint64_t unkD8[(0x110 - 0xD8) >> 3];				// D8
	};
	static_assert(sizeof(TextureRequest) == 0x110);

	namespace Manager {
		void Hooks_ProcessEvent();
		void Hooks_LoadChunks();
		void Hooks_StartStreamingChunks();
		void Hooks_DecompressStreamedLoad();
		void Hooks_BSGraphics_Renderer_CreateStreamingTexture();
		void Hooks_BSGraphics_CreateStreamingDDSTexture();
		void Hooks_ThreadProc();
	}
}
