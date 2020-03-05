// Copyright (C) 2020 Jérôme Leclercq
// This file is part of the "Nazara Engine - Core module"
// For conditions of distribution and use, see copyright notice in Config.hpp

#pragma once

#ifndef NAZARA_LOG_HPP
#define NAZARA_LOG_HPP

#include <Nazara/Prerequisites.hpp>
#include <Nazara/Core/Signal.hpp>
#include <Nazara/Core/String.hpp>

#ifdef NAZARA_DEBUG
	#define NazaraDebug(txt) NazaraNotice(txt)
#else
	#define NazaraDebug(txt)
#endif

#define NazaraNotice(txt) Nz::Log::Write(txt)

namespace Nz
{
	class AbstractLogger;

	class NAZARA_CORE_API Log
	{
		friend class Core;

		public:
			static void Enable(bool enable);

			static AbstractLogger* GetLogger();

			static bool IsEnabled();

			static void SetLogger(AbstractLogger* logger);

			static void Write(const String& string);
			static void WriteError(ErrorType type, const String& error, unsigned int line = 0, const char* file = nullptr, const char* function = nullptr);

			NazaraStaticSignal(OnLogWrite, const String& /*string*/);
			NazaraStaticSignal(OnLogWriteError, ErrorType /*type*/, const String& /*error*/, unsigned int /*line*/, const char* /*file*/, const char* /*function*/);

		private:
			static bool Initialize();
			static void Uninitialize();

			static AbstractLogger* s_logger;
			static bool s_enabled;
	};
}

#endif // NAZARA_LOGGER_HPP
