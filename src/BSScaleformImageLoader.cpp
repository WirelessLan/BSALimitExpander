#include "BSScaleformImageLoader.h"

#include "xbyak/xbyak.h"

namespace BSScaleformImageLoader {
	void Hooks_LoadProtocolImage() {
		struct asm_code : Xbyak::CodeGenerator {
			asm_code(std::uintptr_t a_jmpTarget) {
				Xbyak::Label jmpLabel;

				cmp(rcx, 0x00);
				jne("JMP");

				xor_(al, al);
				ret();

				L("JMP");
				jmp(ptr[rip + jmpLabel]);

				L(jmpLabel);
				dq(a_jmpTarget);
			}
		};

		REL::Relocation<std::uintptr_t> hookTarget(REL::Offset(0x211450C));
		REL::Relocation<std::uintptr_t> jmpTarget(REL::Offset(0x22E5AD0));
		asm_code p{ jmpTarget.address() };
		auto& trampoline = F4SE::GetTrampoline();
		void* codeBuf = trampoline.allocate(p);
		trampoline.write_branch<5>(hookTarget.address(), codeBuf);
	}
}
