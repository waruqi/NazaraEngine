// Copyright (C) 2017 Jérôme Leclercq
// This file is part of the "Nazara Engine - Utility module"
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <memory>
#include <Nazara/Utility/Debug.hpp>

namespace Nz
{
	template<typename... Args>
	StaticMeshRef StaticMesh::New(Args&&... args)
	{
		return std::make_shared<StaticMesh>(std::forward<Args>(args)...);
	}
}

#include <Nazara/Utility/DebugOff.hpp>
