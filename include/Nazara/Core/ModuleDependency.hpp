// Copyright (C) 2017 Jérôme Leclercq
// This file is part of the "Nazara Engine - Core module"
// For conditions of distribution and use, see copyright notice in Config.hpp

#pragma once

#ifndef NAZARA_MODULEDEPENDENCY_HPP
#define NAZARA_MODULEDEPENDENCY_HPP

#include <Nazara/Prerequesites.hpp>
#include <Nazara/Core/Initializer.hpp>

namespace Nz
{
	template<typename... Args>
	class ModuleDependency : Initializer<Args...>
	{
		public:
			ModuleDependency() = default;
			ModuleDependency(const ModuleDependency&) = default;
			ModuleDependency(ModuleDependency&&) = default;
			~ModuleDependency() = default;

			ModuleDependency& operator=(const ModuleDependency&) = default;
			ModuleDependency& operator=(ModuleDependency&&) = default;
	};
}

#include <Nazara/Core/ModuleDependency.inl>

#endif // NAZARA_MODULEDEPENDENCY_HPP
