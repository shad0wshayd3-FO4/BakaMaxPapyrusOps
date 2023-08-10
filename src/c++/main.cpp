class MaxPapyrusOpsPerFrame
{
public:
	static void Install()
	{
		struct PapyrusOpsPerFrame : Xbyak::CodeGenerator
		{
			PapyrusOpsPerFrame(std::uintptr_t a_begin, std::uintptr_t a_end)
			{
				inc(r14d);
				cmp(r14d, MaxOpsPerFrame);
				jp("Loop");
				mov(rcx, a_end);
				jmp(rcx);
				L("Loop");
				mov(rcx, a_begin);
				jmp(rcx);
			}
		};

		REL::Relocation<std::uintptr_t> target{ REL::ID(614585), 0x4F0 };
		REL::Relocation<std::uintptr_t> loopBeg{ REL::ID(614585), 0x0A0 };
		REL::Relocation<std::uintptr_t> loopEnd{ REL::ID(614585), 0x4FD };

		auto code = PapyrusOpsPerFrame(loopBeg.address(), loopEnd.address());
		REL::safe_fill(target.address(), REL::NOP, 0x0D);

		auto& trampoline = F4SE::GetTrampoline();
		auto result = trampoline.allocate(code);
		trampoline.write_branch<5>(target.address(), reinterpret_cast<std::uintptr_t>(result));
	}

	template<typename T>
	static void Update(T a_value)
	{
		MaxOpsPerFrame = static_cast<std::int32_t>(a_value);
	}

private:
	inline static std::int32_t MaxOpsPerFrame{ 100 };
};

namespace
{
	void InitializeLog()
	{
		auto path = logger::log_directory();
		if (!path)
		{
			stl::report_and_fail("Failed to find standard logging directory"sv);
		}

		*path /= fmt::format(FMT_STRING("{:s}.log"sv), Version::PROJECT);
		auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);

		auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));
		auto lvl = *Settings::General::EnableDebugLogging
		               ? spdlog::level::trace
		               : spdlog::level::info;

		log->set_level(lvl);
		log->flush_on(lvl);

		spdlog::set_default_logger(std::move(log));
		spdlog::set_pattern("[%m/%d/%Y - %T] [%^%l%$] %v"s);

		logger::info(FMT_STRING("{:s} v{:s}"sv), Version::PROJECT, Version::NAME);
	}
}

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Query(const F4SE::QueryInterface* a_F4SE, F4SE::PluginInfo* a_info)
{
	a_info->infoVersion = F4SE::PluginInfo::kVersion;
	a_info->name = Version::PROJECT.data();
	a_info->version = Version::MAJOR;

	const auto rtv = a_F4SE->RuntimeVersion();
	if (rtv < F4SE::RUNTIME_LATEST)
	{
		stl::report_and_fail(
			fmt::format(
				FMT_STRING("{:s} does not support runtime v{:s}."sv),
				Version::PROJECT,
				rtv.string()));
	}

	return true;
}

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Load(const F4SE::LoadInterface* a_F4SE)
{
	Settings::Load();
	InitializeLog();

	logger::info(FMT_STRING("{:s} loaded."sv), Version::PROJECT);
	logger::debug("Debug logging enabled."sv);

	F4SE::Init(a_F4SE);
	F4SE::AllocTrampoline(1 << 6);

	MaxPapyrusOpsPerFrame::Update(*Settings::Tweaks::MaxPapyrusOpsPerFrame);
	MaxPapyrusOpsPerFrame::Install();

	return true;
}
