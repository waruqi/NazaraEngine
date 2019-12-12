// Copyright (C) 2017 Jérôme Leclercq
// This file is part of the "Nazara Engine - Utility module"
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <Nazara/Utility/Node.hpp>
#include <Nazara/Utility/Debug.hpp>

namespace Nz
{
	Node::Node() :
	m_rotation(Quaternionf::Identity()),
	m_position(Vector3f::Zero()),
	m_scale(Vector3f(1.f, 1.f, 1.f)),
	m_parent(nullptr),
	m_derivedUpdated(false),
	m_transformMatrixUpdated(false)
	{
	}

	Node::Node(const Node& node) :
	m_rotation(node.m_rotation),
	m_position(node.m_position),
	m_scale(node.m_scale),
	m_parent(nullptr),
	m_derivedUpdated(false),
	m_transformMatrixUpdated(false)
	{
		SetParent(node.m_parent, false);
	}

	Node::~Node()
	{
		OnNodeRelease(this);

		for (Node* child : m_childs)
		{
			// child->SetParent(nullptr); serait problématique car elle nous appellerait
			child->m_parent = nullptr;
			child->InvalidateNode();
			child->OnParenting(nullptr);
		}

		SetParent(nullptr);
	}

	void Node::EnsureDerivedUpdate() const
	{
		if (!m_derivedUpdated)
			UpdateDerived();
	}

	void Node::EnsureTransformMatrixUpdate() const
	{
		if (!m_transformMatrixUpdated)
			UpdateTransformMatrix();
	}

	Vector3f Node::GetBackward() const
	{
		if (!m_derivedUpdated)
			UpdateDerived();

		return m_derivedRotation * Vector3f::Backward();
	}

	const std::vector<Node*>& Node::GetChilds() const
	{
		return m_childs;
	}

	Vector3f Node::GetDown() const
	{
		if (!m_derivedUpdated)
			UpdateDerived();

		return m_derivedRotation * Vector3f::Down();
	}

	Vector3f Node::GetForward() const
	{
		if (!m_derivedUpdated)
			UpdateDerived();

		return m_derivedRotation * Vector3f::Forward();
	}

	Vector3f Node::GetLeft() const
	{
		if (!m_derivedUpdated)
			UpdateDerived();

		return m_derivedRotation * Vector3f::Left();
	}

	NodeType Node::GetNodeType() const
	{
		return NodeType_Default;
	}

	const Node* Node::GetParent() const
	{
		return m_parent;
	}

	Vector3f Node::GetPosition(CoordSys coordSys) const
	{
		switch (coordSys)
		{
			case CoordSys_Global:
				if (!m_derivedUpdated)
					UpdateDerived();

				return m_derivedPosition;

			case CoordSys_Local:
				return m_position;
		}

		NazaraError("Coordinate system out of enum (0x" + String::Number(coordSys, 16) + ')');
		return Vector3f();
	}

	Vector3f Node::GetRight() const
	{
		if (!m_derivedUpdated)
			UpdateDerived();

		return m_derivedRotation * Vector3f::Right();
	}

	Quaternionf Node::GetRotation(CoordSys coordSys) const
	{
		switch (coordSys)
		{
			case CoordSys_Global:
				if (!m_derivedUpdated)
					UpdateDerived();

				return m_derivedRotation;

			case CoordSys_Local:
				return m_rotation;
		}

		NazaraError("Coordinate system out of enum (0x" + String::Number(coordSys, 16) + ')');
		return Quaternionf();
	}

	Vector3f Node::GetScale(CoordSys coordSys) const
	{
		switch (coordSys)
		{
			case CoordSys_Global:
				if (!m_derivedUpdated)
					UpdateDerived();

				return m_derivedScale;

			case CoordSys_Local:
				return m_scale;
		}

		NazaraError("Coordinate system out of enum (0x" + String::Number(coordSys, 16) + ')');
		return Vector3f();
	}

	const Matrix4f& Node::GetTransformMatrix() const
	{
		if (!m_transformMatrixUpdated)
			UpdateTransformMatrix();

		return m_transformMatrix;
	}

	Vector3f Node::GetUp() const
	{
		if (!m_derivedUpdated)
			UpdateDerived();

		return m_derivedRotation * Vector3f::Up();
	}

	bool Node::HasChilds() const
	{
		return !m_childs.empty();
	}

	Node& Node::Interpolate(const Node& nodeA, const Node& nodeB, float interpolation, CoordSys coordSys)
	{
		switch (coordSys)
		{
			case CoordSys_Global:
				if (!nodeA.m_derivedUpdated)
					nodeA.UpdateDerived();

				if (!nodeB.m_derivedUpdated)
					nodeB.UpdateDerived();

				m_position = ToLocalPosition(Vector3f::Lerp(nodeA.m_derivedPosition, nodeB.m_derivedPosition, interpolation));
				m_rotation = ToLocalRotation(Quaternionf::Slerp(nodeA.m_derivedRotation, nodeB.m_derivedRotation, interpolation));
				m_scale = ToLocalScale(Vector3f::Lerp(nodeA.m_derivedScale, nodeB.m_derivedScale, interpolation));
				break;

			case CoordSys_Local:
				m_position = Vector3f::Lerp(nodeA.m_position, nodeB.m_position, interpolation);
				m_rotation = Quaternionf::Slerp(nodeA.m_rotation, nodeB.m_rotation, interpolation);
				m_scale = Vector3f::Lerp(nodeA.m_scale, nodeB.m_scale, interpolation);
				break;
		}

		InvalidateNode();
		return *this;
	}

	Node& Node::Move(const Vector3f& movement, CoordSys coordSys)
	{
		switch (coordSys)
		{
			case CoordSys_Global:
			{
				if (m_parent)
				{
					if (!m_parent->m_derivedUpdated)
						m_parent->UpdateDerived();

					m_position += (m_parent->m_derivedRotation.GetConjugate()*(movement - m_parent->m_derivedPosition))/m_parent->m_derivedScale; // Compensation
				}
				else
					m_position += movement; // Rien n'affecte le node

				break;
			}

			case CoordSys_Local:
				m_position += m_rotation * movement;
				break;
		}

		InvalidateNode();
		return *this;
	}

	Node& Node::Move(float moveX, float moveY, float moveZ, CoordSys coordSys)
	{
		return Move(Vector3f(moveX, moveY, moveZ), coordSys);
	}

	Node& Node::Rotate(const Quaternionf& rotation, CoordSys coordSys)
	{
		// Évitons toute mauvaise surprise ..
		Quaternionf q(rotation);
		q.Normalize();

		switch (coordSys)
		{
			case CoordSys_Global:
			{
				if (!m_derivedUpdated)
					UpdateDerived();

				m_rotation *= m_derivedRotation.GetInverse() * q * m_derivedRotation; ///FIXME: Correct ?
				break;
			}

			case CoordSys_Local:
				m_rotation *= q;
				break;
		}

		m_rotation.Normalize();

		InvalidateNode();
		return *this;
	}

	Node& Node::Scale(const Vector3f& scale)
	{
		m_scale *= scale;

		InvalidateNode();
		return *this;
	}

	Node& Node::Scale(float scale)
	{
		m_scale *= scale;

		InvalidateNode();
		return *this;
	}

	Node& Node::Scale(float scaleX, float scaleY, float scaleZ)
	{
		m_scale.x *= scaleX;
		m_scale.y *= scaleY;
		m_scale.z *= scaleZ;

		InvalidateNode();
		return *this;
	}

	void Node::SetParent(const Node* node, bool keepDerived)
	{
		#if NAZARA_UTILITY_SAFE
		// On vérifie que le node n'est pas son propre parent
		const Node* parentNode = node;
		while (parentNode)
		{
			if (parentNode == this)
			{
				NazaraError("A node cannot be it's own parent");
				return;
			}

			parentNode = parentNode->GetParent();
		}
		#endif

		if (m_parent == node)
			return;

		if (keepDerived)
		{
			if (!m_derivedUpdated)
				UpdateDerived();

			if (m_parent)
				m_parent->RemoveChild(this);

			m_parent = node;
			if (m_parent)
				m_parent->AddChild(this);

			SetRotation(m_derivedRotation, CoordSys_Global);
			SetScale(m_derivedScale, CoordSys_Global);
			SetPosition(m_derivedPosition, CoordSys_Global);
		}
		else
		{
			if (m_parent)
				m_parent->RemoveChild(this);

			m_parent = node;
			if (m_parent)
				m_parent->AddChild(this);

			InvalidateNode();
		}

		OnParenting(node);
	}

	void Node::SetParent(const Node& node, bool keepDerived)
	{
		SetParent(&node, keepDerived);
	}

	void Node::SetPosition(const Vector3f& position, CoordSys coordSys)
	{
		switch (coordSys)
		{
			case CoordSys_Global:
				if (m_parent)
				{
					if (!m_parent->m_derivedUpdated)
						m_parent->UpdateDerived();

					m_position = (m_parent->m_derivedRotation.GetConjugate()*(position - m_parent->m_derivedPosition))/m_parent->m_derivedScale;
				}
				else
					m_position = position;
				break;

			case CoordSys_Local:
				m_position = position;
				break;
		}

		InvalidateNode();
	}

	void Node::SetPosition(float positionX, float positionY, float positionZ, CoordSys coordSys)
	{
		SetPosition(Vector3f(positionX, positionY, positionZ), coordSys);
	}

	void Node::SetRotation(const Quaternionf& rotation, CoordSys coordSys)
	{
		// Évitons toute mauvaise surprise ..
		Quaternionf q(rotation);
		q.Normalize();

		switch (coordSys)
		{
			case CoordSys_Global:
				if (m_parent)
					m_rotation = m_parent->GetRotation().GetConjugate() * q;
				else
					m_rotation = q;

				break;

			case CoordSys_Local:
				m_rotation = q;
				break;
		}

		InvalidateNode();
	}

	void Node::SetScale(const Vector2f& scale, CoordSys coordSys)
	{
		SetScale(Vector3f(scale, 1.f), coordSys);
	}

	void Node::SetScale(const Vector3f& scale, CoordSys coordSys)
	{
		switch (coordSys)
		{
			case CoordSys_Global:
				if (m_parent)
					m_scale = scale / m_parent->GetScale();
				else
					m_scale = scale;
				break;

			case CoordSys_Local:
				m_scale = scale;
				break;
		}

		InvalidateNode();
	}

	void Node::SetScale(float scale, CoordSys coordSys)
	{
		SetScale(Vector3f(scale), coordSys);
	}

	void Node::SetScale(float scaleX, float scaleY, float scaleZ, CoordSys coordSys)
	{
		SetScale(Vector3f(scaleX, scaleY, scaleZ), coordSys);
	}

	void Node::SetTransformMatrix(const Matrix4f& matrix)
	{
		m_transformMatrix = matrix;
		m_transformMatrixUpdated = true;
		m_derivedUpdated = false;
	}

	Vector3f Node::ToGlobalPosition(const Vector3f& localPosition) const
	{
		if (!m_transformMatrixUpdated)
			UpdateTransformMatrix();

		return m_transformMatrix.Transform(localPosition);
	}

	Quaternionf Node::ToGlobalRotation(const Quaternionf& localRotation) const
	{
		if (!m_derivedUpdated)
			UpdateDerived();

		return m_derivedRotation * localRotation;
	}

	Vector3f Node::ToGlobalScale(const Vector3f& localScale) const
	{
		if (!m_derivedUpdated)
			UpdateDerived();

		return m_derivedScale * localScale;
	}

	Vector3f Node::ToLocalPosition(const Vector3f& globalPosition) const
	{
		if (!m_derivedUpdated)
			UpdateDerived();

		return (m_derivedRotation.GetConjugate()*(globalPosition - m_derivedPosition))/m_derivedScale;
	}

	Quaternionf Node::ToLocalRotation(const Quaternionf& globalRotation) const
	{
		if (!m_derivedUpdated)
			UpdateDerived();

		return m_derivedRotation.GetConjugate() * globalRotation;
	}

	Vector3f Node::ToLocalScale(const Vector3f& globalScale) const
	{
		if (!m_derivedUpdated)
			UpdateDerived();

		return globalScale / m_derivedScale;
	}

	Node& Node::operator=(const Node& node)
	{
		SetParent(node.m_parent);

		m_position = node.m_position;
		m_rotation = node.m_rotation;
		m_scale = node.m_scale;

		InvalidateNode();

		return *this;
	}

	void Node::AddChild(Node* node) const
	{
		#ifdef NAZARA_DEBUG
		if (std::find(m_childs.begin(), m_childs.end(), node) != m_childs.end())
		{
			NazaraWarning("Child node is already a child of parent node");
			return;
		}
		#endif

		m_childs.push_back(node);
	}

	void Node::InvalidateNode()
	{
		m_derivedUpdated = false;
		m_transformMatrixUpdated = false;

		for (Node* node : m_childs)
			node->InvalidateNode();

		OnNodeInvalidation(this);
	}

	void Node::OnParenting(const Node* parent)
	{
		OnNodeNewParent(this, parent);
	}

	void Node::RemoveChild(Node* node) const
	{
		auto it = std::find(m_childs.begin(), m_childs.end(), node);
		if (it != m_childs.end())
			m_childs.erase(it);
		else
			NazaraWarning("Child not found");
	}

	void Node::UpdateDerived() const
	{
		if (!m_transformMatrixUpdated)
		{
			UpdateTransformMatrix();
			if (m_derivedUpdated)
				return;
		}

		m_transformMatrix.Decompose(m_derivedPosition, m_derivedRotation, m_derivedScale);
		m_derivedUpdated = true;
	}

	void Node::UpdateTransformMatrix() const
	{
		m_transformMatrix.MakeTransform(m_position, m_rotation, m_scale);
		if (m_parent)
		{
			m_transformMatrix.ConcatenateAffine(m_parent->GetTransformMatrix());
			m_derivedUpdated = false;
		}
		else
		{
			m_derivedPosition = m_position;
			m_derivedRotation = m_rotation;
			m_derivedScale = m_scale;
			m_derivedUpdated = true;
		}

		m_transformMatrixUpdated = true;
	}
}
