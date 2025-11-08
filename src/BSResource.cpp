#include "BSResource.h"

#include <xbyak/xbyak.h>

#include "ankerl/unordered_dense.h"

constexpr std::uint32_t MaxArchiveCount = 65535;

namespace BSResource {
	RE::BSTSmartPointer<RE::BSResource::Stream> g_dataFiles[MaxArchiveCount];
	RE::BSTSmartPointer<RE::BSResource::AsyncStream> g_asyncDataFiles[MaxArchiveCount];
	RE::BSResource::ID g_dataFileNameIDs[MaxArchiveCount];

	ankerl::unordered_dense::map<ID, std::uint16_t, BSResourceIDHash> g_generalIndexMap;
	RE::BSReadWriteLock g_generalIndexLock;

	ankerl::unordered_dense::map<ID, std::uint16_t, BSResourceIDHash> g_textureIndexMap;
	RE::BSReadWriteLock g_textureIndexLock;

	void InsertGeneralArchiveIndex(const ID& a_id, std::uint32_t a_archIdx) {
		RE::BSAutoWriteLock lock(g_generalIndexLock);
		g_generalIndexMap[a_id] = static_cast<std::uint16_t>(a_archIdx);
	}

	std::uint16_t FindGeneralArchiveIndex(const ID& a_id) {
		RE::BSAutoReadLock lock(g_generalIndexLock);
		auto it = g_generalIndexMap.find(a_id);
		if (it == g_generalIndexMap.end()) {
			return static_cast<std::uint16_t>(-1);
		}
		return it->second;
	}

	void InsertTextureArchiveIndex(const ID& a_id, std::uint32_t a_archIdx) {
		RE::BSAutoWriteLock lock(g_textureIndexLock);
		g_textureIndexMap[a_id] = static_cast<std::uint16_t>(a_archIdx);
	}

	std::uint16_t FindTextureArchiveIndex(const ID& a_id) {
		RE::BSAutoReadLock lock(g_textureIndexLock);
		auto it = g_textureIndexMap.find(a_id);
		if (it == g_textureIndexMap.end()) {
			return static_cast<std::uint16_t>(-1);
		}
		return it->second;
	}

	namespace Archive2 {
		void Func_125260(RE::BSTSmartPointer<RE::BSResource::Stream>& a_stream, RE::BSTSmartPointer<RE::BSResource::Stream>& a_res) {
			using func_t = decltype(&Func_125260);
			REL::Relocation<func_t> func(REL::Offset(0x125260));
			func(a_stream, a_res);
		}

		RE::BSTSmallIndexScatterTable<RE::BSResource::ID, RE::BSResource::Archive2::Index::NameIDAccess>::entry_type* Func_1B16AB0(std::uint64_t a_arg1) {
			using func_t = decltype(&Func_1B16AB0);
			REL::Relocation<func_t> func(REL::Offset(0x1B16AB0));
			return func(a_arg1);
		}

		bool Func_1B772F0(RE::BSTSmallIndexScatterTable<RE::BSResource::ID, RE::BSResource::Archive2::Index::NameIDAccess>& a_nameTable, std::uint32_t a_index, RE::BSResource::ID*& a_id) {
			using func_t = decltype(&Func_1B772F0);
			REL::Relocation<func_t> func(REL::Offset(0x1B772F0));
			return func(a_nameTable, a_index, a_id);
		}

		void Func_1B7B740(RE::BSTSmallIndexScatterTable<RE::BSResource::ID, RE::BSResource::Archive2::Index::NameIDAccess>& a_nameTable, RE::BSResource::ID*& a_id) {
			using func_t = decltype(&Func_1B7B740);
			REL::Relocation<func_t> func(REL::Offset(0x1B7B740));
			func(a_nameTable, a_id);
		}

		void AddDataFile(RE::BSResource::Archive2::Index& a_self, RE::BSTSmartPointer<RE::BSResource::Stream>& a_stream, RE::BSResource::ID& a_id, std::uint32_t a_index) {
			Func_125260(g_dataFiles[a_index], a_stream);
			a_stream->DoCreateAsync(g_asyncDataFiles[a_index]);

			if (a_self.dataFileCount != a_index)
				return;

			g_dataFileNameIDs[a_index] = a_id;
			RE::BSResource::ID* p_id = g_dataFileNameIDs;

			if (a_self.nameTable.table == REL::Relocation<RE::BSTSmallIndexScatterTable<RE::BSResource::ID, RE::BSResource::Archive2::Index::NameIDAccess>::entry_type*>(REL::Offset(0x3845978)).get()) {
				a_self.nameTable.avail = 2;
				a_self.nameTable.table = Func_1B16AB0(2);
			}
			else {
				bool result = Func_1B772F0(a_self.nameTable, a_index, p_id);
				if (!result)
					Func_1B7B740(a_self.nameTable, p_id);
			}

			Func_1B772F0(a_self.nameTable, a_index, p_id);
			a_self.dataFileCount++;
		}

		void Hooks_Index_AddDataFile() {
			struct asm_code : Xbyak::CodeGenerator {
				asm_code(std::uintptr_t a_targetAddr) {
					Xbyak::Label retLabel;

					mov(ptr[rsp + 0x10], rbx);
					jmp(ptr[rip + retLabel]);

					L(retLabel);
					dq(a_targetAddr + 0x05);
				}
			};

			REL::Relocation<std::uintptr_t> target(REL::Offset(0x1B767F0));
			asm_code p{ target.address() };
			auto& trampoline = F4SE::GetTrampoline();
			[[maybe_unused]] void* codeBuf = trampoline.allocate(p);
			trampoline.write_branch<5>(target.address(), AddDataFile);
		}

		void Hooks_Index_AddDataFromReader() {
			struct asm_code : Xbyak::CodeGenerator {
				asm_code(std::uintptr_t a_targetAddr, std::uintptr_t a_funcAddr) {
					Xbyak::Label retnLabel;
					Xbyak::Label funcLabel;

					mov(ptr[rsp + 0x4C], dil);

					push(rax);
					push(rcx);
					push(rdx);
					sub(rsp, 0x18);

					lea(rcx, ptr[rsp + 0x40 + 0x8 + 0x8 + 0x8 + 0x18]);
					mov(edx, edi);
					
					call(ptr[rip + funcLabel]);	

					add(rsp, 0x18);
					pop(rdx);
					pop(rcx);
					pop(rax);

					jmp(ptr[rip + retnLabel]);

					L(retnLabel);
					dq(a_targetAddr + 0x05);

					L(funcLabel);
					dq(a_funcAddr);
				}
			};

			REL::Relocation<std::uintptr_t> target(REL::Offset(0x1B76AF7));
			asm_code p{ target.address(), (std::uintptr_t)InsertGeneralArchiveIndex };
			auto& trampoline = F4SE::GetTrampoline();
			void* codeBuf = trampoline.allocate(p);
			trampoline.write_branch<5>(target.address(), codeBuf);
		}
	}

	namespace SDirectory2 {
		void Hooks_Retrieve() {
			{
				struct asm_code : Xbyak::CodeGenerator {
					asm_code(std::uintptr_t a_target, std::uintptr_t a_funcAddr) {
						Xbyak::Label retnLabel;
						Xbyak::Label funcLabel;

						push(rcx);
						sub(rsp, 0x10);

						lea(rcx, ptr[rbp + 0x00000148]);
						call(ptr[rip + funcLabel]);

						add(rsp, 0x10);
						pop(rcx);

						cmp(eax, 0xFFFF);
						jne("RET");
						movzx(eax, byte[rbp + 0x00000154]);

						L("RET");
						jmp(ptr[rip + retnLabel]);

						L(retnLabel);
						dq(a_target + 0x7);

						L(funcLabel);
						dq(a_funcAddr);
					}
				};

				REL::Relocation<std::uintptr_t> target(REL::Offset(0x1B71BB5));
				asm_code p{ target.address(), (std::uintptr_t)FindGeneralArchiveIndex };
				auto& trampoline = F4SE::GetTrampoline();
				void* codeBuf = trampoline.allocate(p);
				trampoline.write_branch<6>(target.address(), codeBuf);
			}
			{
				struct asm_code : Xbyak::CodeGenerator {
					asm_code(std::uintptr_t a_target) {
						Xbyak::Label retnLabel;

						mov(rcx, (std::uintptr_t)g_dataFiles);
						mov(rdx, ptr[rcx + rax * 8]);

						jmp(ptr[rip + retnLabel]);

						L(retnLabel);
						dq(a_target + 0x5);
					}
				};

				REL::Relocation<std::uintptr_t> target(REL::Offset(0x1B71BD6));
				asm_code p{ target.address() };
				auto& trampoline = F4SE::GetTrampoline();
				void* codeBuf = trampoline.allocate(p);
				trampoline.write_branch<5>(target.address(), codeBuf);
			}
			{
				struct asm_code : Xbyak::CodeGenerator {
					asm_code(std::uintptr_t a_target, std::uintptr_t a_funcAddr) {
						Xbyak::Label retnLabel;
						Xbyak::Label funcLabel;

						push(rcx);
						sub(rsp, 0x10);

						lea(rcx, ptr[rbp + 0x00000148]);
						call(ptr[rip + funcLabel]);

						add(rsp, 0x10);
						pop(rcx);

						cmp(eax, 0xFFFF);
						jne("RET");
						movzx(eax, byte[rbp + 0x00000154]);

						L("RET");
						jmp(ptr[rip + retnLabel]);

						L(retnLabel);
						dq(a_target + 0x7);

						L(funcLabel);
						dq(a_funcAddr);
					}
				};

				REL::Relocation<std::uintptr_t> target(REL::Offset(0x1B71C3F));
				asm_code p{ target.address(), (std::uintptr_t)FindGeneralArchiveIndex };
				auto& trampoline = F4SE::GetTrampoline();
				void* codeBuf = trampoline.allocate(p);
				trampoline.write_branch<6>(target.address(), codeBuf);
			}
			{
				struct asm_code : Xbyak::CodeGenerator {
					asm_code(std::uintptr_t a_target, std::uintptr_t a_funcAddr) {
						Xbyak::Label retnLabel;
						Xbyak::Label funcLabel;

						push(rcx);
						sub(rsp, 0x10);

						lea(rcx, ptr[rsi + 0x00000148]);
						call(ptr[rip + funcLabel]);

						add(rsp, 0x10);
						pop(rcx);

						cmp(eax, 0xFFFF);
						jne("RET");
						movzx(eax, byte[rsi + 0x00000154]);

						L("RET");
						jmp(ptr[rip + retnLabel]);

						L(retnLabel);
						dq(a_target + 0x7);

						L(funcLabel);
						dq(a_funcAddr);
					}
				};

				REL::Relocation<std::uintptr_t> target(REL::Offset(0x1B7288C));
				asm_code p{ target.address(), (std::uintptr_t)FindGeneralArchiveIndex };
				auto& trampoline = F4SE::GetTrampoline();
				void* codeBuf = trampoline.allocate(p);
				trampoline.write_branch<6>(target.address(), codeBuf);
			}
			{
				struct asm_code : Xbyak::CodeGenerator {
					asm_code(std::uintptr_t a_target) {
						Xbyak::Label retnLabel;

						mov(rcx, (std::uintptr_t)g_asyncDataFiles);
						mov(rdx, ptr[rcx + rax * 8]);

						jmp(ptr[rip + retnLabel]);

						L(retnLabel);
						dq(a_target + 0x8);
					}
				};

				REL::Relocation<std::uintptr_t> target(REL::Offset(0x1B728B5));
				asm_code p{ target.address() };
				auto& trampoline = F4SE::GetTrampoline();
				void* codeBuf = trampoline.allocate(p);
				trampoline.write_branch<5>(target.address(), codeBuf);
			}
		}

		void InsertReplicatedGeneralID(const ID& a_id, std::uint32_t a_repDir) {
			std::uint16_t index = FindGeneralArchiveIndex(a_id);
			if (index == static_cast<std::uint16_t>(-1)) {
				return;
			}

			ID repId = a_id;
			repId.dir = a_repDir;
			InsertGeneralArchiveIndex(repId, index);
		}

		void Hooks_ReplicateDirTo() {
			struct asm_code : Xbyak::CodeGenerator {
				asm_code(std::uintptr_t a_targetAddr, std::uintptr_t a_funcAddr) {
					Xbyak::Label retnLabel;
					Xbyak::Label funcLabel;

					push(rax);
					push(rcx);
					sub(rsp, 0x20);

					lea(rcx, ptr[rdi]);
					mov(edx, ebx);
					call(ptr[rip + funcLabel]);

					add(rsp, 0x20);
					pop(rcx);
					pop(rax);

					L("RET");
					mov(ptr[rdi + 0x08], ebx);
					mov(ptr[rbp - 0x48], ebx);
					jmp(ptr[rip + retnLabel]);

					L(retnLabel);
					dq(a_targetAddr + 0x06);

					L(funcLabel);
					dq(a_funcAddr);
				}
			};

			REL::Relocation<std::uintptr_t> target(REL::Offset(0x1B7249D));
			asm_code p{ target.address(), (std::uintptr_t)InsertReplicatedGeneralID };
			auto& trampoline = F4SE::GetTrampoline();
			void* codeBuf = trampoline.allocate(p);
			trampoline.write_branch<6>(target.address(), codeBuf);
		}
	}
}
