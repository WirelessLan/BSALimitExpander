#include "BSTexture.h"

#include <xbyak/xbyak.h>

#include "BSResource.h"

#define	MAX_SIZE	65535

namespace BSTextureIndex {
	RE::BSResource::ID g_dataFileNameIDs[MAX_SIZE];

	void Overwrites_Instruction() {
		REL::Relocation<std::uintptr_t> target(REL::Offset(0x1CB89A6));
		unsigned char buf[] = { 0x45, 0x8B, 0xCC , 0x90 };	// movzx r9d,r12l -> mov r9d, r12d; nop;
		REL::safe_write(target.address(), buf, sizeof(buf));
	}

	void Hooks_Index_AddDataFile() {
		struct asm_code : Xbyak::CodeGenerator {
			asm_code(std::uintptr_t a_target) {
				Xbyak::Label retnLabel;

				mov(rbx, (std::uintptr_t)g_dataFileNameIDs);

				mov(eax, ptr[rsi + 0x8]);
				lea(rdx, ptr[rdi + rdi * 0x2]);
				mov(ptr[rbx + rdx * 0x4 + 0x8], eax);
				mov(ecx, ptr[rsi + 0x4]);
				mov(eax, ptr[rsi]);
				shl(rcx, 0x20);
				or_(rcx, rax);
				mov(ptr[rbx + rdx * 0x4], ecx);
				shr(rcx, 0x20);
				mov(ptr[rbx + rdx * 0x4 + 0x4], ecx);

				jmp(ptr[rip + retnLabel]);

				L(retnLabel);
				dq(a_target + 0x23);
			}
		};

		REL::Relocation<std::uintptr_t> target(REL::Offset(0x1CB4087));
		asm_code p{ target.address() };
		auto& trampoline = F4SE::GetTrampoline();
		void* codeBuf = trampoline.allocate(p);
		trampoline.write_branch<5>(target.address(), codeBuf);
	}
}

namespace BSTextureStreamer {
	namespace Manager {
		const char* texturesPrefix = "textures/";
		const char* dataPrefix = "data/";
		const size_t texturesPrefixLength = std::char_traits<char>::length(texturesPrefix);
		const size_t dataPrefixLength = std::char_traits<char>::length(dataPrefix);

		std::string ProcessPath(const char* path) {
			std::string result(path);
			size_t pathLength = result.length();

			for (size_t i = 0; i < pathLength; ++i) {
				if (result[i] == '\\')
					result[i] = '/';
				result[i] = static_cast<char>(tolower(result[i]));
			}

			if (result.compare(0, dataPrefixLength, dataPrefix) == 0)
				result.erase(0, dataPrefixLength);

			if (result.compare(0, texturesPrefixLength, texturesPrefix) == 0)
				result.erase(0, texturesPrefixLength);

			return result;
		}

		std::unordered_map<std::string, std::uint16_t> g_pathIndexMap;

		void InsertPathIndex(const char* a_path, std::uint32_t a_archIdx) {
			std::string path = ProcessPath(a_path);
			g_pathIndexMap[path] = static_cast<std::uint16_t>(a_archIdx);
		}

		std::uint16_t FindPathIndex(RE::BSFixedString a_path) {
			std::string path = ProcessPath(a_path.c_str());
			auto it = g_pathIndexMap.find(path.c_str());
			if (it == g_pathIndexMap.end())
				return static_cast<std::uint16_t>(-1);
			return it->second;
		}

		void Hooks_ProcessEvent() {
			{
				struct asm_code : Xbyak::CodeGenerator {
					asm_code(std::uintptr_t a_target, std::uintptr_t a_funcAddr) {
						Xbyak::Label retnLabel;
						Xbyak::Label funcLabel;

						mov(ptr[rsp + 0x5C], r12b);

						push(rcx);
						push(rdx);
						sub(rsp, 0x18);

						lea(rcx, ptr[rsp + 0x50 + 0x8 + 0x8 + 0x18]);
						mov(edx, r12d);

						call(ptr[rip + funcLabel]);

						add(rsp, 0x18);
						pop(rdx);
						pop(rcx);

						jmp(ptr[rip + retnLabel]);

						L(retnLabel);
						dq(a_target + 0x5);

						L(funcLabel);
						dq(a_funcAddr);
					}
				};

				REL::Relocation<std::uintptr_t> target(REL::Offset(0x1CB87F5));
				asm_code p{ target.address(), (std::uintptr_t)BSResource::InsertArchiveIndex };
				auto& trampoline = F4SE::GetTrampoline();
				void* codeBuf = trampoline.allocate(p);
				trampoline.write_branch<5>(target.address(), codeBuf);
			}
			{
				struct asm_code : Xbyak::CodeGenerator {
					asm_code(std::uintptr_t a_target, std::uintptr_t a_funcAddr) {
						Xbyak::Label retnLabel;
						Xbyak::Label funcLabel;

						lea(rcx, ptr[rbp - 0x38]);
						mov(rdx, rax);

						push(rcx);
						push(rdx);
						sub(rsp, 0x18);

						mov(rcx, rdx);
						mov(edx, r12d);

						call(ptr[rip + funcLabel]);

						add(rsp, 0x18);
						pop(rdx);
						pop(rcx);

						jmp(ptr[rip + retnLabel]);

						L(retnLabel);
						dq(a_target + 0x7);

						L(funcLabel);
						dq(a_funcAddr);
					}
				};

				REL::Relocation<std::uintptr_t> target(REL::Offset(0x1CB8804));
				asm_code p{ target.address(), (std::uintptr_t)InsertPathIndex };
				auto& trampoline = F4SE::GetTrampoline();
				void* codeBuf = trampoline.allocate(p);
				trampoline.write_branch<6>(target.address(), codeBuf);
			}
		}

		void Hooks_LoadChunks() {
			struct asm_code : Xbyak::CodeGenerator {
				asm_code(std::uintptr_t a_target, std::uintptr_t a_funcAddr) {
					Xbyak::Label retnLabel;
					Xbyak::Label funcLabel;

					push(rax);
					push(rcx);
					push(rdx);
					push(r10);
					push(r11);

					lea(rcx, ptr[rdx]);
					call(ptr[rip + funcLabel]);

					mov(ebx, eax);

					pop(r11);
					pop(r10);
					pop(rdx);
					pop(rcx);
					pop(rax);

					cmp(ebx, 0xFFFF);
					jne("RET");
					movzx(ebx, byte[rdx + 0x0C]);

					L("RET");
					movzx(edi, byte[rdx + 0x0D]);
					jmp(ptr[rip + retnLabel]);

					L(retnLabel);
					dq(a_target + 0x8);

					L(funcLabel);
					dq(a_funcAddr);
				}
			};

			REL::Relocation<std::uintptr_t> target(REL::Offset(0x1CB7C07));
			asm_code p{ target.address(), (std::uintptr_t)BSResource::FindArchiveIndex };
			auto& trampoline = F4SE::GetTrampoline();
			void* codeBuf = trampoline.allocate(p);
			trampoline.write_branch<6>(target.address(), codeBuf);
		}

		void Hooks_StartStreamingChunks() {
			struct asm_code : Xbyak::CodeGenerator {
				asm_code(std::uintptr_t a_target, std::uintptr_t a_funcAddr) {
					Xbyak::Label retnLabel;
					Xbyak::Label funcLabel;

					push(rcx);
					sub(rsp, 0x10);

					lea(rcx, ptr[r14+0xD0]);
					call(ptr[rip + funcLabel]);

					mov(edx, eax);

					add(rsp, 0x10);
					pop(rcx);

					cmp(edx, 0xFFFF);
					jne("RET");
					movzx(edx, byte[r14 + 0x0C]);

					L("RET");
					jmp(ptr[rip + retnLabel]);

					L(retnLabel);
					dq(a_target + 0x5);

					L(funcLabel);
					dq(a_funcAddr);
				}
			};

			REL::Relocation<std::uintptr_t> target(REL::Offset(0x1CBA3FD));
			asm_code p{ target.address(), (std::uintptr_t)FindPathIndex };
			auto& trampoline = F4SE::GetTrampoline();
			void* codeBuf = trampoline.allocate(p);
			trampoline.write_branch<5>(target.address(), codeBuf);
		}

		void Hooks_BSGraphics_Renderer_CreateStreamingTexture() {
			struct asm_code : Xbyak::CodeGenerator {
				asm_code(std::uintptr_t a_target, std::uintptr_t a_funcAddr) {
					Xbyak::Label retnLabel;
					Xbyak::Label funcLabel;

					lea(rcx, ptr[rsi]);
					call(ptr[rip + funcLabel]);

					mov(ebx, eax);

					cmp(ebx, 0xFFFF);
					jne("RET");
					movzx(ebx, byte[rsi + 0x0C]);

					L("RET");
					mov(rcx, rsi);
					jmp(ptr[rip + retnLabel]);

					L(retnLabel);
					dq(a_target + 0x7);

					L(funcLabel);
					dq(a_funcAddr);
				}
			};

			REL::Relocation<std::uintptr_t> target(REL::Offset(0x1D0ECE3));
			asm_code p{ target.address(), (std::uintptr_t)BSResource::FindArchiveIndex };
			auto& trampoline = F4SE::GetTrampoline();
			void* codeBuf = trampoline.allocate(p);
			trampoline.write_branch<6>(target.address(), codeBuf);
		}

		void Hooks_BSGraphics_CreateStreamingDDSTexture() {
			struct asm_code : Xbyak::CodeGenerator {
				asm_code(std::uintptr_t a_target, std::uintptr_t a_funcAddr) {
					Xbyak::Label retnLabel;
					Xbyak::Label funcLabel;

					push(rcx);

					lea(rcx, ptr[rsi]);
					call(ptr[rip + funcLabel]);

					pop(rcx);

					cmp(eax, 0xFFFF);
					jne("RET");
					movzx(eax, byte[rsi + 0x0C]);

					L("RET");
					mov(ptr[r14 + 0x12], ax);
					jmp(ptr[rip + retnLabel]);

					L(retnLabel);
					dq(a_target + 0x5);

					L(funcLabel);
					dq(a_funcAddr);
				}
			};

			REL::Relocation<std::uintptr_t> target(REL::Offset(0x1D37BF5));
			asm_code p{ target.address(), (std::uintptr_t)BSResource::FindArchiveIndex };
			auto& trampoline = F4SE::GetTrampoline();
			void* codeBuf = trampoline.allocate(p);
			trampoline.write_branch<5>(target.address(), codeBuf);
		}

		void Hooks_ThreadProc() {
			struct asm_code : Xbyak::CodeGenerator {
				asm_code(std::uintptr_t a_target, std::uintptr_t a_funcAddr) {
					Xbyak::Label retnLabel;
					Xbyak::Label funcLabel;

					lea(rcx, ptr[r14]);
					call(ptr[rip + funcLabel]);

					mov(ecx, eax);

					cmp(ecx, 0xFFFF);
					jne("RET");
					movzx(ecx, byte[r14 + 0x0C]);

					L("RET");
					jmp(ptr[rip + retnLabel]);

					L(retnLabel);
					dq(a_target + 0x5);

					L(funcLabel);
					dq(a_funcAddr);
				}
			};

			REL::Relocation<std::uintptr_t> target(REL::Offset(0x1CBAACF));
			asm_code p{ target.address(), (std::uintptr_t)BSResource::FindArchiveIndex };
			auto& trampoline = F4SE::GetTrampoline();
			void* codeBuf = trampoline.allocate(p);
			trampoline.write_branch<5>(target.address(), codeBuf);
		}
	}
}
