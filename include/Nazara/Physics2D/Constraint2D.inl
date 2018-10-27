// Copyright (C) 2017 Jérôme Leclercq
// This file is part of the "Nazara Engine - Physics 2D module"
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <Nazara/Physics2D/Constraint2D.hpp>
#include <memory>
#include <Nazara/Physics2D/Debug.hpp>

namespace Nz
{
	template<typename... Args>
	DampedSpringConstraint2DRef DampedSpringConstraint2D::New(Args&&... args)
	{
		return std::make_shared<DampedSpringConstraint2D>(std::forward<Args>(args)...);
	}

	template<typename... Args>
	DampedRotarySpringConstraint2DRef DampedRotarySpringConstraint2D::New(Args&&... args)
	{
		return std::make_shared<DampedRotarySpringConstraint2D>(std::forward<Args>(args)...);
	}

	template<typename... Args>
	GearConstraint2DRef GearConstraint2D::New(Args&&... args)
	{
		return std::make_shared<GearConstraint2D>(std::forward<Args>(args)...);
	}

	template<typename... Args>
	MotorConstraint2DRef MotorConstraint2D::New(Args&&... args)
	{
		return std::make_shared<MotorConstraint2D>(std::forward<Args>(args)...);
	}

	template<typename... Args>
	PinConstraint2DRef PinConstraint2D::New(Args&&... args)
	{
		return std::make_shared<PinConstraint2D>(std::forward<Args>(args)...);
	}

	template<typename... Args>
	PivotConstraint2DRef PivotConstraint2D::New(Args&&... args)
	{
		return std::make_shared<PivotConstraint2D>(std::forward<Args>(args)...);
	}

	template<typename... Args>
	RatchetConstraint2DRef RatchetConstraint2D::New(Args&&... args)
	{
		return std::make_shared<RatchetConstraint2D>(std::forward<Args>(args)...);
	}

	template<typename... Args>
	RotaryLimitConstraint2DRef RotaryLimitConstraint2D::New(Args&&... args)
	{
		return std::make_shared<RotaryLimitConstraint2D>(std::forward<Args>(args)...);
	}

	template<typename... Args>
	SlideConstraint2DRef SlideConstraint2D::New(Args&&... args)
	{
		return std::make_shared<SlideConstraint2D>(std::forward<Args>(args)...);
	}
}

#include <Nazara/Physics2D/DebugOff.hpp>
