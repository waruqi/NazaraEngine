// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "Nazara Engine - ChipmunkPhysics2D module"
// For conditions of distribution and use, see copyright notice in Config.hpp

#pragma once

#ifndef NAZARA_CHIPMUNKPHYSICS2D_CHIPMUNKRIGIDBODY2D_HPP
#define NAZARA_CHIPMUNKPHYSICS2D_CHIPMUNKRIGIDBODY2D_HPP

#include <NazaraUtils/Prerequisites.hpp>
#include <Nazara/ChipmunkPhysics2D/ChipmunkCollider2D.hpp>
#include <Nazara/ChipmunkPhysics2D/Config.hpp>
#include <Nazara/Core/Enums.hpp>
#include <Nazara/Math/Angle.hpp>
#include <Nazara/Math/Rect.hpp>
#include <NazaraUtils/Signal.hpp>
#include <functional>
#include <limits>

struct cpBody;

namespace Nz
{
	class ChipmunkArbiter2D;
	class ChipmunkPhysWorld2D;

	class NAZARA_CHIPMUNKPHYSICS2D_API ChipmunkRigidBody2D
	{
		public:
			struct DynamicSettings;
			struct StaticSettings;

			using VelocityFunc = std::function<void(ChipmunkRigidBody2D& body2D, const Vector2f& gravity, float damping, float deltaTime)>;

			inline ChipmunkRigidBody2D(ChipmunkPhysWorld2D& world, const DynamicSettings& settings);
			inline ChipmunkRigidBody2D(ChipmunkPhysWorld2D& world, const StaticSettings& settings);
			ChipmunkRigidBody2D(const ChipmunkRigidBody2D& object);
			ChipmunkRigidBody2D(ChipmunkRigidBody2D&& object) noexcept;
			inline ~ChipmunkRigidBody2D();

			inline void AddForce(const Vector2f& force, CoordSys coordSys = CoordSys::Global);
			void AddForce(const Vector2f& force, const Vector2f& point, CoordSys coordSys = CoordSys::Global);
			inline void AddImpulse(const Vector2f& impulse, CoordSys coordSys = CoordSys::Global);
			void AddImpulse(const Vector2f& impulse, const Vector2f& point, CoordSys coordSys = CoordSys::Global);
			void AddTorque(const RadianAnglef& torque);

			bool ClosestPointQuery(const Vector2f& position, Vector2f* closestPoint = nullptr, float* closestDistance = nullptr) const;

			void EnableSimulation(bool simulation);

			void ForEachArbiter(const FunctionRef<void(ChipmunkArbiter2D& /*arbiter*/)>& callback);
			void ForceSleep();

			Rectf GetAABB() const;
			RadianAnglef GetAngularVelocity() const;
			float GetElasticity(std::size_t shapeIndex = 0) const;
			float GetFriction(std::size_t shapeIndex = 0) const;
			inline const std::shared_ptr<ChipmunkCollider2D>& GetGeom() const;
			inline cpBody* GetHandle() const;
			inline float GetMass() const;
			Vector2f GetMassCenter(CoordSys coordSys = CoordSys::Local) const;
			float GetMomentOfInertia() const;
			Vector2f GetPosition() const;
			inline const Vector2f& GetPositionOffset() const;
			RadianAnglef GetRotation() const;
			inline std::size_t GetShapeCount() const;
			inline std::size_t GetShapeIndex(cpShape* shape) const;
			Vector2f GetSurfaceVelocity(std::size_t shapeIndex = 0) const;
			inline void* GetUserdata() const;
			Vector2f GetVelocity() const;
			inline const VelocityFunc& GetVelocityFunction() const;
			inline ChipmunkPhysWorld2D* GetWorld() const;

			inline bool IsKinematic() const;
			inline bool IsSimulationEnabled() const;
			bool IsSleeping() const;
			inline bool IsStatic() const;

			void ResetVelocityFunction();

			void SetAngularVelocity(const RadianAnglef& angularVelocity);
			void SetElasticity(float elasticity);
			void SetElasticity(std::size_t shapeIndex, float elasticity);
			void SetFriction(float friction);
			void SetFriction(std::size_t shapeIndex, float friction);
			void SetGeom(std::shared_ptr<ChipmunkCollider2D> geom, bool recomputeMoment = true, bool recomputeMassCenter = true);
			void SetMass(float mass, bool recomputeMoment = true);
			void SetMassCenter(const Vector2f& center, CoordSys coordSys = CoordSys::Local);
			void SetMomentOfInertia(float moment);
			void SetPosition(const Vector2f& position);
			void SetPositionOffset(const Vector2f& offset);
			void SetRotation(const RadianAnglef& rotation);
			void SetSurfaceVelocity(const Vector2f& surfaceVelocity);
			void SetSurfaceVelocity(std::size_t shapeIndex, const Vector2f& surfaceVelocity);
			void SetStatic(bool setStaticBody = true);
			inline void SetUserdata(void* ud);
			void SetVelocity(const Vector2f& velocity);
			void SetVelocityFunction(VelocityFunc velocityFunc);

			void TeleportTo(const Vector2f& position, const RadianAnglef& rotation);

			void UpdateVelocity(const Vector2f& gravity, float damping, float deltaTime);

			void Wakeup();

			ChipmunkRigidBody2D& operator=(const ChipmunkRigidBody2D& object);
			ChipmunkRigidBody2D& operator=(ChipmunkRigidBody2D&& object);

			NazaraSignal(OnRigidBody2DMove, ChipmunkRigidBody2D* /*oldPointer*/, ChipmunkRigidBody2D* /*newPointer*/);
			NazaraSignal(OnRigidBody2DRelease, ChipmunkRigidBody2D* /*rigidBody*/);

			static constexpr std::size_t InvalidShapeIndex = std::numeric_limits<std::size_t>::max();

			struct CommonSettings
			{
				std::shared_ptr<ChipmunkCollider2D> geom;
				RadianAnglef rotation = RadianAnglef::Zero();
				Vector2f position = Vector2f::Zero();
				bool initiallySleeping = false;
				bool isSimulationEnabled = true;
			};

			struct DynamicSettings : CommonSettings
			{
				DynamicSettings() = default;
				DynamicSettings(std::shared_ptr<ChipmunkCollider2D> collider, float mass_) :
				mass(mass_)
				{
					geom = std::move(collider);
				}

				RadianAnglef angularVelocity = RadianAnglef::Zero();
				Vector2f linearVelocity = Vector2f::Zero();
				float gravityFactor = 1.f;
				float mass = 1.f;
			};

			struct StaticSettings : CommonSettings
			{
				StaticSettings() = default;
				StaticSettings(std::shared_ptr<ChipmunkCollider2D> collider)
				{
					geom = std::move(collider);
				}
			};

		protected:
			ChipmunkRigidBody2D() = default;
			void Create(ChipmunkPhysWorld2D& world, const DynamicSettings& settings);
			void Create(ChipmunkPhysWorld2D& world, const StaticSettings& settings);
			void Destroy();

		private:
			void RegisterToSpace();
			void UnregisterFromSpace();

			static void CopyBodyData(cpBody* from, cpBody* to);
			static void CopyShapeData(cpShape* from, cpShape* to);

			std::vector<cpShape*> m_shapes;
			std::shared_ptr<ChipmunkCollider2D> m_geom;
			MovablePtr<cpBody> m_handle;
			MovablePtr<ChipmunkPhysWorld2D> m_world;
			MovablePtr<void> m_userData;
			Vector2f m_positionOffset;
			VelocityFunc m_velocityFunc;
			bool m_isRegistered;
			bool m_isSimulationEnabled;
			bool m_isStatic;
			float m_gravityFactor;
			float m_mass;
	};
}

#include <Nazara/ChipmunkPhysics2D/ChipmunkRigidBody2D.inl>

#endif // NAZARA_CHIPMUNKPHYSICS2D_CHIPMUNKRIGIDBODY2D_HPP
