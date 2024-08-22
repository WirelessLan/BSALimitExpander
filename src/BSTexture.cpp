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
				if (ch == '\\')
					*writePtr = '/';
				else if (ch >= 'A' && ch <= 'Z')
					*writePtr = ch + 32;
				else
					*writePtr = ch;
				readPtr++;
				writePtr++;
			}
			*writePtr = '\0';

			const char* prefix1 = "data/textures/";
			size_t prefix1Len = std::strlen(prefix1);
			const char* prefix2 = "textures/";
			size_t prefix2Len = std::strlen(prefix2);

			readPtr = outputPath;

			if (strncmp(readPtr, prefix1, prefix1Len) == 0) {
				readPtr += std::strlen("data/");
			}
			else if (strncmp(readPtr, prefix2, prefix2Len) == 0) {
				return;
			}
			else {
				char tempPath[MAX_PATH];
				strcpy_s(tempPath, outputPath);
				strcpy_s(outputPath, MAX_PATH, prefix2);
				strcat_s(outputPath, MAX_PATH, tempPath);
				return;
			}

			writePtr = outputPath;
			while (*readPtr)
				*writePtr++ = *readPtr++;
			*writePtr = '\0';
		}

		std::uint16_t FindArchiveIndexByTextureRequest(const TextureRequest& a_request) {
			if (!a_request.texturePath.empty()) {
				char processedPath[MAX_PATH];
				ProcessPath(a_request.texturePath.c_str(), processedPath);

				BSResource::ID id;
				BSResource::ID::GenerateID(id, processedPath);

				return BSResource::FindArchiveIndex(id);
			}

			if (a_request.niTexture && a_request.niTexture->rendererTexture && a_request.niTexture->rendererTexture->data) {
				auto data = a_request.niTexture->rendererTexture->data;
				return (data->dataFileHighIndex << 8) | data->dataFileIndex;
			}

			RE::BSResource::ID id = a_request.header.nameID;
			return BSResource::FindArchiveIndex(*reinterpret_cast<BSResource::ID*>(&id));
		}

		void Hooks_ProcessEvent() {
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

					lea(rcx, ptr[r14]);
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
			asm_code p{ target.address(), (std::uintptr_t)FindArchiveIndexByTextureRequest };
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

					lea(rcx, ptr[r15]);
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
			asm_code p{ target.address(), (std::uintptr_t)FindArchiveIndexByTextureRequest };
			auto& trampoline = F4SE::GetTrampoline();
			void* codeBuf = trampoline.allocate(p);
			trampoline.write_branch<5>(target.address(), codeBuf);
		}

		void Hooks_BSGraphics_Renderer_CreateStreamingTexture() {
			struct asm_code : Xbyak::CodeGenerator {
				asm_code(std::uintptr_t a_target, std::uintptr_t a_funcAddr) {
					Xbyak::Label retnLabel;
					Xbyak::Label funcLabel;

					push(rax);
					push(rcx);
					sub(rsp, 0x8);

					lea(rcx, ptr[rsi]);
					call(ptr[rip + funcLabel]);

					mov(edx, eax);

					add(rsp, 0x8);
					pop(rcx);
					pop(rax);

					cmp(edx, 0xFFFF);
					je("RET");

					mov(byte[rax + 0x4], dl);
					mov(byte[rax + 0x7], dh);

					L("RET");
					movzx(edx, byte[rcx + 0x3C]);
					mov(ptr[rax + 0x06], dl);
					jmp(ptr[rip + retnLabel]);

					L(retnLabel);
					dq(a_target + 0xD);

					L(funcLabel);
					dq(a_funcAddr);
				}
			};

			REL::Relocation<std::uintptr_t> target(REL::Offset(0x1D0ED1B));
			asm_code p{ target.address(), (std::uintptr_t)BSResource::FindArchiveIndex };
			auto& trampoline = F4SE::GetTrampoline();
			void* codeBuf = trampoline.allocate(p);
			trampoline.write_branch<5>(target.address(), codeBuf);
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
