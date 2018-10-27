// Copyright (C) 2017 Jérôme Leclercq
// This file is part of the "Nazara Engine - Graphics module"
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <memory>
#include <Nazara/Utility/Debug.hpp>

namespace Nz
{
	template<typename... Args>
	ParticleDeclarationRef ParticleDeclaration::New(Args&&... args)
	{
		return std::make_shared<ParticleDeclaration>(std::forward<Args>(args)...);
	}
}

#include <Nazara/Utility/DebugOff.hpp>
