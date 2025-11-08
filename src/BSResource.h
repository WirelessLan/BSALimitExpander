#pragma once

namespace  BSResource {
	struct ID {
		std::uint32_t file;
		std::uint32_t ext;
		std::uint32_t dir;

		bool operator==(const ID& other) const {
			return file == other.file && ext == other.ext && dir == other.dir;
		}

		static ID& GenerateID(ID& id, const char* path) {
			using func_t = ID&(*)(ID&, const char*);
			const REL::Relocation<func_t> func(REL::Offset(0x1B6F0E0));
			return func(id, path);
		}
	};

	void InsertGeneralArchiveIndex(const ID& a_id, std::uint32_t a_archIdx);
	std::uint16_t FindGeneralArchiveIndex(const ID& a_id);

	void InsertTextureArchiveIndex(const ID& a_id, std::uint32_t a_archIdx);
	std::uint16_t FindTextureArchiveIndex(const ID& a_id);

	namespace Archive2 {
		void Hooks_Index_AddDataFile();
		void Hooks_Index_AddDataFromReader();
	}

	namespace SDirectory2 {
		void Hooks_Retrieve();
		void Hooks_ReplicateDirTo();
	}
}

struct BSResourceIDHash {
	std::size_t operator()(const BSResource::ID& id) const {
		return ((std::hash<std::uint32_t>()(id.file) ^
			    (std::hash<std::uint32_t>()(id.ext) << 1)) >> 1) ^
		        (std::hash<std::uint32_t>()(id.dir) << 1);
	}
};
