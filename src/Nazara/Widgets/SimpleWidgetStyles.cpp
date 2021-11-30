// Copyright (C) 2021 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "Nazara Engine - Widgets module"
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <Nazara/Widgets/SimpleWidgetStyles.hpp>
#include <Nazara/Graphics/Components/GraphicsComponent.hpp>
#include <Nazara/Utility/Components/NodeComponent.hpp>
#include <Nazara/Widgets/ButtonWidget.hpp>
#include <Nazara/Widgets/Canvas.hpp>
#include <Nazara/Widgets/LabelWidget.hpp>
#include <Nazara/Widgets/Widgets.hpp>
#include <Nazara/Widgets/Debug.hpp>

namespace Nz
{
	SimpleButtonWidgetStyle::SimpleButtonWidgetStyle(ButtonWidget* buttonWidget, std::shared_ptr<Material> material, std::shared_ptr<Material> hoveredMaterial, std::shared_ptr<Material> pressedMaterial, std::shared_ptr<Material> pressedHoveredMaterial) :
	ButtonWidgetStyle(buttonWidget),
	m_hoveredMaterial(std::move(hoveredMaterial)),
	m_material(std::move(material)),
	m_pressedMaterial(std::move(pressedMaterial)),
	m_pressedHoveredMaterial(std::move(pressedHoveredMaterial)),
	m_isHovered(false),
	m_isPressed(false)
	{
		assert(m_material);

		auto& registry = GetRegistry();
		UInt32 renderMask = GetRenderMask();

		m_sprite = std::make_shared<SlicedSprite>(m_material);
		m_textSprite = std::make_shared<TextSprite>(Widgets::Instance()->GetTransparentMaterial());

		m_spriteEntity = CreateGraphicsEntity();
		registry.get<GraphicsComponent>(m_spriteEntity).AttachRenderable(m_sprite, renderMask);

		m_textEntity = CreateGraphicsEntity();
		registry.get<GraphicsComponent>(m_textEntity).AttachRenderable(m_textSprite, renderMask);
	}

	void SimpleButtonWidgetStyle::Layout(const Vector2f& size)
	{
		m_sprite->SetSize(size);

		entt::registry& registry = GetRegistry();

		Boxf textBox = m_textSprite->GetAABB();
		registry.get<NodeComponent>(m_textEntity).SetPosition(size.x / 2.f - textBox.width / 2.f, size.y / 2.f - textBox.height / 2.f);
	}

	void SimpleButtonWidgetStyle::OnHoverBegin()
	{
		m_isHovered = true;
		UpdateMaterial(m_isHovered, m_isPressed);
	}

	void SimpleButtonWidgetStyle::OnHoverEnd()
	{
		m_isHovered = false;
		UpdateMaterial(m_isHovered, m_isPressed);
	}

	void SimpleButtonWidgetStyle::OnPress()
	{
		m_isPressed = true;
		UpdateMaterial(m_isHovered, m_isPressed);
	}

	void SimpleButtonWidgetStyle::OnRelease()
	{
		m_isPressed = false;
		UpdateMaterial(m_isHovered, m_isPressed);
	}

	void SimpleButtonWidgetStyle::UpdateText(const AbstractTextDrawer& drawer)
	{
		m_textSprite->Update(drawer);
	}

	void SimpleButtonWidgetStyle::UpdateMaterial(bool hovered, bool pressed)
	{
		if (pressed && hovered && m_pressedHoveredMaterial)
			m_sprite->SetMaterial(m_pressedHoveredMaterial);
		else if (pressed && m_pressedMaterial)
			m_sprite->SetMaterial(m_pressedMaterial);
		else if (hovered && m_hoveredMaterial)
			m_sprite->SetMaterial(m_hoveredMaterial);
		else
			m_sprite->SetMaterial(m_material);
	}


	SimpleLabelWidgetStyle::SimpleLabelWidgetStyle(LabelWidget* labelWidget, std::shared_ptr<Material> material, std::shared_ptr<Material> hoveredMaterial) :
	LabelWidgetStyle(labelWidget),
	m_hoveredMaterial(std::move(hoveredMaterial)),
	m_material(std::move(material))
	{
		assert(m_material);

		auto& registry = GetRegistry();
		UInt32 renderMask = GetRenderMask();

		m_textSprite = std::make_shared<TextSprite>(m_material);

		m_entity = CreateGraphicsEntity();
		registry.get<GraphicsComponent>(m_entity).AttachRenderable(m_textSprite, renderMask);
	}

	void SimpleLabelWidgetStyle::Layout(const Vector2f& size)
	{
		entt::registry& registry = GetRegistry();

		Boxf textBox = m_textSprite->GetAABB();
		registry.get<NodeComponent>(m_entity).SetPosition(size.x / 2.f - textBox.width / 2.f, size.y / 2.f - textBox.height / 2.f);
	}

	void SimpleLabelWidgetStyle::OnHoverBegin()
	{
		UpdateMaterial(true);
	}

	void SimpleLabelWidgetStyle::OnHoverEnd()
	{
		UpdateMaterial(false);
	}

	void SimpleLabelWidgetStyle::UpdateMaterial(bool hovered)
	{
		if (hovered && m_hoveredMaterial)
			m_textSprite->SetMaterial(m_hoveredMaterial);
		else
			m_textSprite->SetMaterial(m_material);
	}

	void SimpleLabelWidgetStyle::UpdateText(const AbstractTextDrawer& drawer)
	{
		m_textSprite->Update(drawer);
	}
}