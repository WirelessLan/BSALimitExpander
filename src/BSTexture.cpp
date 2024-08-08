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
		void ProcessPath(const char* inputPath, char* outputPath) {
			const char* readPtr = inputPath;
			char* writePtr = outputPath;

			while (*readPtr) {
				char ch = *readPtr;
				if (ch == '\\') {
					*writePtr = '/';
				}
				else if (ch >= 'A' && ch <= 'Z') {
					*writePtr = ch + 32;
				}
				else {
					*writePtr = ch;
				}
				readPtr++;
				writePtr++;
			}
			*writePtr = '\0';

			constexpr const char* prefix1 = "data/textures/";
			constexpr size_t prefix1Len = 14;
			constexpr const char* prefix2 = "textures/";
			constexpr size_t prefix2Len = 9;

			readPtr = outputPath;

			if (strncmp(readPtr, prefix1, prefix1Len) == 0) {
				readPtr += prefix1Len;
			}
			else if (strncmp(readPtr, prefix2, prefix2Len) == 0) {
				readPtr += prefix2Len;
			}

			if (readPtr != outputPath) {
				writePtr = outputPath;
				while (*readPtr) {
					*writePtr++ = *readPtr++;
				}
				*writePtr = '\0';
			}
		}

		std::unordered_map<std::string, std::uint16_t> g_pathIndexMap;

		void InsertPathIndex(const char* a_path, std::uint32_t a_archIdx) {
			char path[MAX_PATH];
			ProcessPath(a_path, path);
			g_pathIndexMap[path] = static_cast<std::uint16_t>(a_archIdx);
		}

		std::uint16_t FindPathIndex(const RE::BSFixedString& a_path) {
			if (a_path.empty())
				return static_cast<std::uint16_t>(-1);

			char path[MAX_PATH];
			ProcessPath(a_path.c_str(), path);
			auto it = g_pathIndexMap.find(path);
			if (it != g_pathIndexMap.end())
				return it->second;

			char fullTexturePath[MAX_PATH];
			std::snprintf(fullTexturePath, sizeof(fullTexturePath), "textures/%s", path);

			BSResource::ID id;
			BSResource::ID::GenerateID(id, fullTexturePath);
			std::uint16_t index = BSResource::FindArchiveIndex(id);
			if (index != static_cast<std::uint16_t>(-1))
				g_pathIndexMap[path] = index;
			return index;
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
						sub(rsp, 0x10);

						mov(rcx, rdx);
						mov(edx, r12d);

						call(ptr[rip + funcLabel]);

						add(rsp, 0x10);
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
					sub(rsp, 0x8);

					lea(rcx, ptr[rdx]);
					call(ptr[rip + funcLabel]);

					mov(ebx, eax);

					add(rsp, 0x8);
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
					sub(rsp, 0x18);

					lea(rcx, ptr[r14 + 0xD0]);
					call(ptr[rip + funcLabel]);

					mov(edx, eax);

					add(rsp, 0x18);
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

		void Hooks_DecompressStreamedLoad() {
			struct asm_code : Xbyak::CodeGenerator {
				asm_code(std::uintptr_t a_target, std::uintptr_t a_funcAddr) {
					Xbyak::Label retnLabel;
					Xbyak::Label funcLabel;

					push(rax);
					sub(rsp, 0x18);

					lea(rcx, ptr[r15 + 0xD0]);
					call(ptr[rip + funcLabel]);

					mov(ecx, eax);

					add(rsp, 0x18);
					pop(rax);

					cmp(ecx, 0xFFFF);
					jne("RET");
					movzx(ecx, byte[r15 + 0x0C]);

					L("RET");
					jmp(ptr[rip + retnLabel]);

					L(retnLabel);
					dq(a_target + 0x5);

					L(funcLabel);
					dq(a_funcAddr);
				}
			};

			REL::Relocation<std::uintptr_t> target(REL::Offset(0x1CB6262));
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

					sub(rsp, 0x8);

					lea(rcx, ptr[rsi]);
					call(ptr[rip + funcLabel]);

					mov(ebx, eax);

					add(rsp, 0x8);

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
					sub(rsp, 0x8);

					lea(rcx, ptr[rsi]);
					call(ptr[rip + funcLabel]);

					add(rsp, 0x8);
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

					sub(rsp, 0x8);

					lea(rcx, ptr[r14]);
					call(ptr[rip + funcLabel]);

					mov(ecx, eax);

					add(rsp, 0x8);

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
