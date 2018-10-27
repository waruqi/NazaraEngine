// Copyright (C) 2017 Jérôme Leclercq
// This file is part of the "Nazara Engine - Utility module"
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <memory>
#include <Nazara/Utility/Debug.hpp>

namespace Nz
{
	template<typename... Args>
	ImageRef Image::New(Args&&... args)
	{
		return std::make_shared<Image>(std::forward<Args>(args)...);
	}
}

#include <Nazara/Utility/DebugOff.hpp>
