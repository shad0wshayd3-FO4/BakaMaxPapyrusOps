class Config
{
public:
	class Fixes
	{
	public:
		inline static DKUtil::Alias::Boolean bFixToggleScriptsCommand{ "bFixToggleScriptsCommand", "Fixes" };
		inline static DKUtil::Alias::Boolean bFixScriptPageAllocation{ "bFixScriptPageAllocation", "Fixes" };
	};

	class Tweaks
	{
	public:
		inline static DKUtil::Alias::Integer iMaxPapyrusOpsPerFrame{ "iMaxPapyrusOpsPerFrame", "Tweaks" };
	};

	static void Load()
	{
		static auto Config = COMPILE_PROXY("BakaMaxPapyrusOps.ini");
		Config.Bind(Fixes::bFixToggleScriptsCommand, true);
		Config.Bind(Fixes::bFixScriptPageAllocation, true);
		Config.Bind(Tweaks::iMaxPapyrusOpsPerFrame, 500);
		Config.Load();
	}
};

class Fixes
{
public:
	static void Install()
	{
		if (*Config::Fixes::bFixScriptPageAllocation)
		{
			FixScriptPageAllocation::Install();
		}

		if (*Config::Fixes::bFixToggleScriptsCommand)
		{
			FixToggleScriptsCommand::Install();
		}
	}

private:
	class FixScriptPageAllocation
	{
	public:
		static void Install()
		{
			static REL::Relocation<std::uintptr_t> target{ RE::BSScript::SimpleAllocMemoryPagePolicy::VTABLE[0] };
			_GetLargestAvailablePage = target.write_vfunc(0x04, GetLargestAvailablePage);
		}

	private:
		static RE::BSScript::SimpleAllocMemoryPagePolicy::AllocationStatus GetLargestAvailablePage(
			RE::BSScript::SimpleAllocMemoryPagePolicy* a_this,
			RE::BSTSmartPointer<RE::BSScript::MemoryPage, RE::BSTSmartPointerAutoPtr>& a_newPage)
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

		inline static REL::Relocation<decltype(&GetLargestAvailablePage)> _GetLargestAvailablePage;
	};

	class FixToggleScriptsCommand
	{
	public:
		static void Install()
		{
			hkEndSaveLoad<2228095, 0x030>::Install();
			hkEndSaveLoad<2228097, 0x69E>::Install();
			hkEndSaveLoad<2228945, 0x30B>::Install();
			hkBSSpinLock_Lock::Install();

			hkFreeze<2251333, 0x119>::Install();
			hkFreeze<2251336, 0x370>::Install();
			hkFreeze<2251364, 0x382>::Install();
		}

	private:
		template <std::uintptr_t ID, std::ptrdiff_t OFF>
		class hkEndSaveLoad
		{
		public:
			static void Install()
			{
				static REL::Relocation<std::uintptr_t> target{ REL::ID(ID), OFF };
				auto& trampoline = F4SE::GetTrampoline();
				trampoline.write_call<5>(target.address(), EndSaveLoad);
			}

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
		};

		class hkBSSpinLock_Lock
		{
		public:
			static void Install()
			{
				{
					static REL::Relocation<std::uintptr_t> target{ REL::ID(2251339), 0x797 };
					auto& trampoline = F4SE::GetTrampoline();
					trampoline.write_call<5>(target.address(), BSSpinLock_Lock);
				}

				{
					static REL::Relocation<std::uintptr_t> target{ REL::ID(2251339), 0x79C };
					REL::safe_fill(target.address(), REL::NOP, 0x08);
				}
			}

		private:
			static void BSSpinLock_Lock(void*, void*)
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
		};

		template <std::uintptr_t ID, std::ptrdiff_t OFF>
		class hkFreeze
		{
		public:
			static void Install()
			{
				static REL::Relocation<std::uintptr_t> target{ REL::ID(ID), OFF };
				auto& trampoline = F4SE::GetTrampoline();
				_Freeze = trampoline.write_call<5>(target.address(), Freeze);
			}

		private:
			static void Freeze(RE::GameVM* a_this, bool a_freeze)
			{
				if (RE::Script::GetProcessScripts())
				{
					return _Freeze(a_this, a_freeze);
				}
			}

			inline static REL::Relocation<decltype(&Freeze)> _Freeze;
		};
	};
};

class Tweaks
{
public:
	static void Install()
	{
		if (*Config::Tweaks::iMaxPapyrusOpsPerFrame > 0)
		{
			MaxPapyrusOpsPerFrame::Update(*Config::Tweaks::iMaxPapyrusOpsPerFrame);
			MaxPapyrusOpsPerFrame::Install();
		}
	}

private:
	class MaxPapyrusOpsPerFrame
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
			static REL::Relocation<std::uintptr_t> tgtBeg{ REL::ID(2315660), 0x0A0 };
			static REL::Relocation<std::uintptr_t> tgtEnd{ REL::ID(2315660), 0xA77 };

			auto code = PapyrusOpsPerFrame(tgtBeg.address(), tgtEnd.address());
			REL::safe_fill(target.address(), REL::NOP, 0x13);

			auto& trampoline = F4SE::GetTrampoline();
			auto result = trampoline.allocate(code);
			trampoline.write_branch<5>(target.address(), reinterpret_cast<std::uintptr_t>(result));
		}

		template <typename T>
		static void Update(T a_value)
		{
			MaxOpsPerFrame = static_cast<std::int32_t>(a_value);
		}

	private:
		inline static std::int32_t MaxOpsPerFrame{ 100 };
	};
};

DLLEXPORT bool F4SEAPI F4SEPlugin_Load(const F4SE::LoadInterface* a_F4SE)
{
#ifndef NDEBUG
	MessageBoxA(NULL, "Loaded. You can now attach the debugger or continue execution.", Plugin::NAME.data(), NULL);
#endif

	F4SE::Init(a_F4SE);
	F4SE::AllocTrampoline(1 << 8);

	DKUtil::Logger::Init(Plugin::NAME, std::to_string(Plugin::Version));
	INFO("{} v{} loaded."sv, Plugin::NAME, Plugin::Version);

	Config::Load();

	Fixes::Install();
	Tweaks::Install();

	return true;
}
