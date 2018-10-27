// Copyright (C) 2017 Jérôme Leclercq
// This file is part of the "Nazara Engine - Utility module"
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <Nazara/Utility/Mesh.hpp>
#include <memory>
#include <Nazara/Utility/Debug.hpp>

namespace Nz
{
	Mesh::Mesh() :
	m_materialData(1),
	m_aabbUpdated(false),
	m_isValid(false)
	{
	}

	Mesh::~Mesh()
	{
		OnMeshRelease(this);

		Destroy();
	}

	template<typename... Args>
	MeshRef Mesh::New(Args&&... args)
	{
		return std::make_shared<Mesh>(std::forward<Args>(args)...);
	}
}

#include <Nazara/Utility/DebugOff.hpp>
