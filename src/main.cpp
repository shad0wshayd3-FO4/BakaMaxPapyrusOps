namespace Config
{
	namespace Tweaks
	{
		inline static REX::INI::I32 iMaxPapyrusOpsPerFrame{ "Tweaks", "iMaxPapyrusOpsPerFrame", 500 };
	}

	static void Load()
	{
		const auto ini = REX::INI::SettingStore::GetSingleton();
		ini->Init(
			"Data/F4SE/plugins/BakaMaxPapyrusOps.ini",
			"Data/F4SE/plugins/BakaMaxPapyrusOpsCustom.ini");
		ini->Load();
	}
}

namespace Hooks
{
	class hkGetLargestAvailablePage :
		public REX::Singleton<hkGetLargestAvailablePage>
	{
	private:
		static RE::BSScript::SimpleAllocMemoryPagePolicy::AllocationStatus GetLargestAvailablePage(RE::BSScript::SimpleAllocMemoryPagePolicy* a_this, RE::BSTSmartPointer<RE::BSScript::MemoryPage, RE::BSTSmartPointerAutoPtr>& a_newPage)
		{
			const RE::BSAutoLock lock{ a_this->dataLock };

			auto maxPageSize = a_this->maxAllocatedMemory - a_this->currentMemorySize;
			auto currentMemorySize = a_this->currentMemorySize;
			if (maxPageSize < 0)
			{
				a_this->currentMemorySize = a_this->maxAllocatedMemory;
			}

			auto result = _GetLargestAvailablePage(a_this, a_newPage);
			if (maxPageSize < 0)
			{
				a_this->currentMemorySize = currentMemorySize;
			}

			return result;
		}

		inline static REL::HookVFT _GetLargestAvailablePage{ RE::BSScript::SimpleAllocMemoryPagePolicy::VTABLE[0], 0x04, GetLargestAvailablePage };
	};

	class hkEndSaveLoad :
		public REX::Singleton<hkEndSaveLoad>
	{
	private:
		static void EndSaveLoad(RE::GameVM* a_this)
		{
			if (a_this->saveLoadInterface)
			{
				a_this->saveLoadInterface->CleanupLoad();
				a_this->saveLoadInterface->CleanupSave();
			}

			a_this->RegisterForAllGameEvents();
			a_this->saveLoad = false;

			a_this->handlePolicy.DropSaveLoadRemapData();
			a_this->objectBindPolicy.EndSaveLoad();
			a_this->handlePolicy.UpdatePersistence();

			if (RE::Script::GetProcessScripts())
			{
				const RE::BSAutoLock lock{ a_this->freezeLock };
				a_this->frozen = false;
			}
		}

		inline static REL::Hook _EndSaveLoad0{ REL::ID(2228095), 0x030, EndSaveLoad };
		inline static REL::Hook _EndSaveLoad1{ REL::ID(2228097), 0x6A3, EndSaveLoad };
		inline static REL::Hook _EndSaveLoad2{ REL::ID(2228945), 0x310, EndSaveLoad };
	};

	class hkLock :
		public REX::Singleton<hkLock>
	{
	public:
		static void Install()
		{
			static REL::Relocation<std::uintptr_t> target{ REL::ID(2251339), 0x79C };
			target.write_fill(REL::NOP, 0x08);
		}

	private:
		static void Lock(void*, void*)
		{
			auto GameVM = RE::GameVM::GetSingleton();
			if (!GameVM)
			{
				return;
			}

			if (RE::Script::GetProcessScripts())
			{
				const RE::BSAutoLock lock{ GameVM->freezeLock };
				GameVM->frozen = false;
			}
		}

		inline static REL::Hook _Lock0{ REL::ID(2251339), 0x797, Lock };
	};

	class hkFreeze :
		public REX::Singleton<hkFreeze>
	{
	private:
		static void Freeze(RE::GameVM* a_this, bool a_freeze)
		{
			if (RE::Script::GetProcessScripts())
			{
				return _Freeze0(a_this, a_freeze);
			}
		}

		inline static REL::Hook _Freeze0{ REL::ID(2251333), 0x119, Freeze };
		inline static REL::Hook _Freeze1{ REL::ID(2251336), 0x370, Freeze };
		inline static REL::Hook _Freeze2{ REL::ID(2251364), 0x382, Freeze };
	};

	class hkMaxPapyrusOpsPerFrame
	{
	public:
		static void Install()
		{
			struct PapyrusOpsPerFrame : Xbyak::CodeGenerator
			{
				PapyrusOpsPerFrame(std::uintptr_t a_begin, std::uintptr_t a_end)
				{
					inc(r12d);
					mov(r8d, 0x1570);
					cmp(r12d, MaxOpsPerFrame);
					jb("Loop");
					mov(rcx, a_end);
					jmp(rcx);
					L("Loop");
					mov(rcx, a_begin);
					jmp(rcx);
				}
			};

			static REL::Relocation<std::uintptr_t> target{ REL::ID(2315660), 0xA64 };
			static REL::Relocation<std::uintptr_t> loopin{ REL::ID(2315660), 0x0A0 };
			static REL::Relocation<std::uintptr_t> loopex{ REL::ID(2315660), 0xA77 };

			auto code = PapyrusOpsPerFrame(loopin.address(), loopex.address()); 
			target.write_fill(REL::NOP, 0x13);

			auto& trampoline = REL::GetTrampoline();
			auto  result = trampoline.allocate(code);
			target.write_jmp<5>(result);
		}

		template <typename T>
		static void Update(T a_value)
		{
			MaxOpsPerFrame = static_cast<std::int32_t>(a_value);
		}

	private:
		inline static std::int32_t MaxOpsPerFrame{ 100 };
	};

	static void Install()
	{
		Config::Load();
		if (Config::Tweaks::iMaxPapyrusOpsPerFrame > 0)
		{
			hkMaxPapyrusOpsPerFrame::Update(Config::Tweaks::iMaxPapyrusOpsPerFrame.GetValue());
			hkMaxPapyrusOpsPerFrame::Install();
		}

		hkLock::Install();
	}
};

namespace
{
	void MessageCallback(F4SE::MessagingInterface::Message* a_msg)
	{
		switch (a_msg->type)
		{
		case F4SE::MessagingInterface::kPostLoad:
			Hooks::Install();
			break;
		default:
			break;
		}
	}
}

F4SE_PLUGIN_LOAD(const F4SE::LoadInterface* a_f4se)
{
	F4SE::Init(a_f4se, { .trampoline = true, .trampolineSize = 128 });
	F4SE::GetMessagingInterface()->RegisterListener(MessageCallback);
	return true;
}
