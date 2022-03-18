#include "spammers.h"

void spammers::clan_tag()
{
	auto apply = [](const char* tag) -> void
	{
		using Fn = int(__fastcall*)(const char*, const char*);
		static auto fn = reinterpret_cast<Fn>(util::FindSignature(crypt_str("engine.dll"), crypt_str("53 56 57 8B DA 8B F9 FF 15")));

		fn(tag, tag);
	};

	static auto removed = false;

	if (!g_cfg.misc.clantag_spammer && !removed)
	{
		removed = true;
		apply(crypt_str(""));
		return;
	}

	if (g_cfg.misc.clantag_spammer)
	{
		auto nci = m_engine()->GetNetChannelInfo();

		if (!nci)
			return;

		static auto time = 1;

		auto ticks = TIME_TO_TICKS(nci->GetAvgLatency(FLOW_OUTGOING)) + (float)m_globals()->m_tickcount;
		auto intervals = 0.3f / m_globals()->m_intervalpertick;

		auto main_time = (int)(ticks / intervals) % 47;

		if (main_time != time && !m_clientstate()->iChokedCommands)
		{
			auto tag = crypt_str("");

			switch (main_time)
			{
			case 0:
				tag = crypt_str(" _ ");
				break;
			case 1:
				tag = crypt_str(" - ");
				break;
			case 2:
				tag = crypt_str(" _ ");
				break;
			case 3:
				tag = crypt_str(" _ ");
				break;
			case 4:
				tag = crypt_str(" S ");
				break;
			case 5:
				tag = crypt_str(" S_ ");
				break;
			case 6:
				tag = crypt_str(" S- ");
				break;
			case 7:
				tag = crypt_str(" S_ ");
				break;
			case 8:
				tag = crypt_str(" S_ ");
			case 9:
				tag = crypt_str(" Sq ");
				break;
			case 10:
				tag = crypt_str(" Sq_ ");
				break;
			case 11:
				tag = crypt_str(" Sq- ");
				break;
			case 12:
				tag = crypt_str(" Sq_ ");
				break;
			case 13:
				tag = crypt_str(" Sq- ");
			case 14:
				tag = crypt_str(" Squ ");
				break;
			case 15:
				tag = crypt_str(" Squ_ ");
				break;
			case 16:
				tag = crypt_str(" Squ- ");
				break;
			case 17:
				tag = crypt_str(" Squ_ ");
				break;
			case 18:
				tag = crypt_str(" Squ- ");
			case 19:
				tag = crypt_str(" Squa ");
				break;
			case 20:
				tag = crypt_str(" Squa_ ");
				break;
			case 21:
				tag = crypt_str(" Squa- ");
				break;
			case 22:
				tag = crypt_str(" Squa_ ");
				break;
			case 23:
				tag = crypt_str(" Squa- ");
				break;
			case 24:
				tag = crypt_str(" Squar ");
				break;
			case 25:
				tag = crypt_str(" Squar_ ");
				break;
			case 26:
				tag = crypt_str(" Squar- ");
				break;
			case 27:
				tag = crypt_str(" Squar_ ");
				break;
			case 28:
				tag = crypt_str(" Squar- ");
				break;
			case 29:
				tag = crypt_str(" Squar_ ");
				break;
			case 30:
				tag = crypt_str(" Square ");
				break;
			case 31:
				tag = crypt_str(" Squared ");
				break;
			case 32:
				tag = crypt_str(" Squaredh ");
				break;
			case 33:
				tag = crypt_str(" Squaredha ");
				break;
			case 34:
				tag = crypt_str(" Squaredhac ");
				break;
			case 35:
				tag = crypt_str(" Squaredhack ");
				break;
			case 36:
				tag = crypt_str(" Squaredhac_ ");
				break;
			case 37:
				tag = crypt_str(" Squaredha- ");
				break;
			case 38:
				tag = crypt_str(" Squaredh_ ");
				break;
			case 39:
				tag = crypt_str(" Squared- ");
				break;
			case 40:
				tag = crypt_str(" Square_ ");
				break;
			case 41:
				tag = crypt_str(" Squar- ");
				break;
			case 42:
				tag = crypt_str(" Squa_ ");
			case 43:
				break;
			case 44:
				tag = crypt_str(" Squ_ ");
				break;
			case 45:
				tag = crypt_str(" Sq- ");
				break;
			case 46:
				tag = crypt_str(" S_ ");
				break;
			case 47:
				tag = crypt_str(" - ");
			}

			apply(tag);
			time = main_time;
		}

		removed = false;
	}
}