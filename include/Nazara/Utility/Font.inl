// Copyright (C) 2017 Jérôme Leclercq
// This file is part of the "Nazara Engine - Utility module"
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <memory>
#include <Nazara/Utility/Debug.hpp>

namespace Nz
{
	template<typename... Args>
	FontRef Font::New(Args&&... args)
	{
		std::unique_ptr<Font> object(new Font(std::forward<Args>(args)...));
		object->SetPersistent(false);

		return object.release();
	}
}

#include <Nazara/Utility/DebugOff.hpp>
