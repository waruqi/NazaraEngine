// Copyright (C) 2017 Jérôme Leclercq
// This file is part of the "Nazara Engine - Physics 3D module"
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <memory>
#include <Nazara/Physics3D/Debug.hpp>

namespace Nz
{
	template<typename... Args>
	BoxCollider3DRef BoxCollider3D::New(Args&&... args)
	{
		return std::make_shared<BoxCollider3D>(std::forward<Args>(args)...);
	}

	template<typename... Args>
	CapsuleCollider3DRef CapsuleCollider3D::New(Args&&... args)
	{
		return std::make_shared<CapsuleCollider3D>(std::forward<Args>(args)...);
	}

	template<typename... Args>
	CompoundCollider3DRef CompoundCollider3D::New(Args&&... args)
	{
		return std::make_shared<CompoundCollider3D>(std::forward<Args>(args)...);
	}

	template<typename... Args>
	ConeCollider3DRef ConeCollider3D::New(Args&&... args)
	{
		return std::make_shared<ConeCollider3D>(std::forward<Args>(args)...);
	}

	template<typename... Args>
	ConvexCollider3DRef ConvexCollider3D::New(Args&&... args)
	{
		return std::make_shared<ConvexCollider3D>(std::forward<Args>(args)...);
	}

	template<typename... Args>
	CylinderCollider3DRef CylinderCollider3D::New(Args&&... args)
	{
		return std::make_shared<CylinderCollider3D>(std::forward<Args>(args)...);
	}

	template<typename... Args>
	NullCollider3DRef NullCollider3D::New(Args&&... args)
	{
		return std::make_shared<NullCollider3D>(std::forward<Args>(args)...);
	}

	template<typename... Args>
	SphereCollider3DRef SphereCollider3D::New(Args&&... args)
	{
		return std::make_shared<SphereCollider3D>(std::forward<Args>(args)...);
	}
}

#include <Nazara/Physics3D/DebugOff.hpp>
