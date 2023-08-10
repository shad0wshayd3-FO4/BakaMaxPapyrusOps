#pragma once

namespace Settings
{
	namespace
	{
		using ISetting = AutoTOML::ISetting;
		using bSetting = AutoTOML::bSetting;
		using iSetting = AutoTOML::iSetting;
	}

	namespace General
	{
		inline bSetting EnableDebugLogging{ "General"s, "EnableDebugLogging"s, false };
	}

	namespace Fixes
	{
		inline bSetting FixScriptPageAllocation{ "Tweaks"s, "FixScriptPageAllocation"s, true, true };
	}

	namespace Tweaks
	{
		inline iSetting MaxPapyrusOpsPerFrame{ "Tweaks"s, "MaxPapyrusOpsPerFrame"s, 100, true };
	}

	inline void Load()
	{
		try
		{
			const auto table = toml::parse_file(
				fmt::format(FMT_STRING("Data/F4SE/Plugins/{:s}.toml"sv), Version::PROJECT));
			for (const auto& setting : ISetting::get_settings())
			{
				setting->load(table);
			}
		}
		catch (const toml::parse_error& e)
		{
			std::ostringstream ss;
			ss
				<< "Error parsing file \'" << *e.source().path << "\':\n"
				<< '\t' << e.description() << '\n'
				<< "\t\t(" << e.source().begin << ')';
			logger::error(FMT_STRING("{:s}"sv), ss.str());
			stl::report_and_fail("Failed to load settings."sv);
		}
		catch (const std::exception& e)
		{
			stl::report_and_fail(e.what());
		}
		catch (...)
		{
			stl::report_and_fail("Unknown failure."sv);
		}
	}
}
