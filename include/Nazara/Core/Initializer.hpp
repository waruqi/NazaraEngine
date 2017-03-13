// Copyright (C) 2017 Jérôme Leclercq
// This file is part of the "Nazara Engine - Core module"
// For conditions of distribution and use, see copyright notice in Config.hpp

#pragma once

#ifndef NAZARA_INITIALIZER_HPP
#define NAZARA_INITIALIZER_HPP

#include <Nazara/Prerequesites.hpp>

namespace Nz
{
	template<typename... Args>
	class Initializer
	{
		public:
			Initializer(bool initialize = true);
			Initializer(const Initializer&);
			Initializer(Initializer&&);
			~Initializer();

			bool Initialize();
			bool IsInitialized() const;
			void Uninitialize();

			explicit operator bool() const;

			Initializer& operator=(const Initializer&);
			Initializer& operator=(Initializer&&);

		private:
			bool m_initialized;
	};
}

#include <Nazara/Core/Initializer.inl>

#endif // NAZARA_INITIALIZER_HPP
