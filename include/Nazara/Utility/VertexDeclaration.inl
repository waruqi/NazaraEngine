// Copyright (C) 2017 Jérôme Leclercq
// This file is part of the "Nazara Engine - Utility module"
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <memory>
#include <Nazara/Utility/Debug.hpp>
#include <Nazara/Utility/Algorithm.hpp>

namespace Nz
{
	template<typename... Args>
	VertexDeclarationRef VertexDeclaration::New(Args&&... args)
	{
		return std::make_shared<VertexDeclaration>(std::forward<Args>(args)...);
	}

	template<typename T> 
	bool VertexDeclaration::HasComponentOfType(VertexComponent component) const
	{
		bool enabled;
		Nz::ComponentType type;

		GetComponent(component, &enabled, &type, nullptr);

		return enabled && GetComponentTypeOf<T>() == type;
	}
}

#include <Nazara/Utility/DebugOff.hpp>
