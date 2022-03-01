// Copyright (C) 2022 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "Nazara Engine - Graphics module"
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <Nazara/Graphics/PhongLightingMaterial.hpp>
#include <Nazara/Core/Algorithm.hpp>
#include <Nazara/Core/ErrorFlags.hpp>
#include <Nazara/Graphics/PredefinedShaderStructs.hpp>
#include <Nazara/Renderer/Renderer.hpp>
#include <Nazara/Shader/ShaderLangParser.hpp>
#include <Nazara/Utility/BufferMapper.hpp>
#include <Nazara/Utility/FieldOffsets.hpp>
#include <Nazara/Utility/MaterialData.hpp>
#include <cassert>
#include <filesystem>
#include <Nazara/Graphics/Debug.hpp>

namespace Nz
{
	namespace
	{
		const UInt8 r_shader[] = {
			#include <Nazara/Graphics/Resources/Shaders/phong_material.nzsl.h>
		};
	}

	PhongLightingMaterial::PhongLightingMaterial(MaterialPass& material) :
	BasicMaterial(material, NoInit{})
	{
		// Most common case: don't fetch texture indexes as a little optimization
		const std::shared_ptr<const MaterialSettings>& materialSettings = GetMaterial().GetSettings();
		if (materialSettings == s_phongMaterialSettings)
		{
			m_basicUniformOffsets = s_basicUniformOffsets;
			m_basicOptionIndexes = s_basicOptionIndexes;
			m_basicTextureIndexes = s_basicTextureIndexes;

			m_phongOptionIndexes = s_phongOptionIndexes;
			m_phongTextureIndexes = s_phongTextureIndexes;
			m_phongUniformOffsets = s_phongUniformOffsets;
		}
		else
		{
			m_basicOptionIndexes.alphaTest = materialSettings->GetOptionIndex("AlphaTest");
			m_basicOptionIndexes.hasAlphaMap = materialSettings->GetOptionIndex("HasAlphaMap");
			m_basicOptionIndexes.hasDiffuseMap = materialSettings->GetOptionIndex("HasDiffuseMap");

			m_phongOptionIndexes.hasEmissiveMap = materialSettings->GetOptionIndex("HasEmissiveMap");
			m_phongOptionIndexes.hasHeightMap = materialSettings->GetOptionIndex("HasHeightMap");
			m_phongOptionIndexes.hasNormalMap = materialSettings->GetOptionIndex("HasNormalMap");
			m_phongOptionIndexes.hasSpecularMap = materialSettings->GetOptionIndex("HasSpecularMap");

			m_basicTextureIndexes.alpha = materialSettings->GetTextureIndex("Alpha");
			m_basicTextureIndexes.diffuse = materialSettings->GetTextureIndex("Diffuse");

			m_phongTextureIndexes.emissive = materialSettings->GetTextureIndex("Emissive");
			m_phongTextureIndexes.height = materialSettings->GetTextureIndex("Height");
			m_phongTextureIndexes.normal = materialSettings->GetTextureIndex("Normal");
			m_phongTextureIndexes.specular = materialSettings->GetTextureIndex("Specular");

			m_uniformBlockIndex = materialSettings->GetUniformBlockIndex("MaterialSettings");
			if (m_uniformBlockIndex != MaterialSettings::InvalidIndex)
			{
				m_basicUniformOffsets.alphaThreshold = materialSettings->GetUniformBlockVariableOffset(m_uniformBlockIndex, "AlphaThreshold");
				m_basicUniformOffsets.diffuseColor = materialSettings->GetUniformBlockVariableOffset(m_uniformBlockIndex, "DiffuseColor");

				m_phongUniformOffsets.ambientColor = materialSettings->GetUniformBlockVariableOffset(m_uniformBlockIndex, "AmbientColor");
				m_phongUniformOffsets.shininess = materialSettings->GetUniformBlockVariableOffset(m_uniformBlockIndex, "Shininess");
				m_phongUniformOffsets.specularColor = materialSettings->GetUniformBlockVariableOffset(m_uniformBlockIndex, "SpecularColor");
			}
			else
			{
				m_basicUniformOffsets.alphaThreshold = MaterialSettings::InvalidIndex;
				m_basicUniformOffsets.diffuseColor = MaterialSettings::InvalidIndex;

				m_phongUniformOffsets.ambientColor = MaterialSettings::InvalidIndex;
				m_phongUniformOffsets.shininess = MaterialSettings::InvalidIndex;
				m_phongUniformOffsets.specularColor = MaterialSettings::InvalidIndex;
			}
		}
	}

	Color PhongLightingMaterial::GetAmbientColor() const
	{
		NazaraAssert(HasAmbientColor(), "Material has no ambient color uniform");

		const std::vector<UInt8>& bufferData = GetMaterial().GetUniformBufferConstData(m_uniformBlockIndex);

		const float* colorPtr = AccessByOffset<const float*>(bufferData.data(), m_phongUniformOffsets.ambientColor);
		return Color(colorPtr[0] * 255, colorPtr[1] * 255, colorPtr[2] * 255, colorPtr[3] * 255); //< TODO: Make color able to use float
	}

	float Nz::PhongLightingMaterial::GetShininess() const
	{
		NazaraAssert(HasShininess(), "Material has no shininess uniform");

		const std::vector<UInt8>& bufferData = GetMaterial().GetUniformBufferConstData(m_uniformBlockIndex);
		return AccessByOffset<const float&>(bufferData.data(), m_phongUniformOffsets.shininess);
	}

	Color PhongLightingMaterial::GetSpecularColor() const
	{
		NazaraAssert(HasSpecularColor(), "Material has no specular color uniform");

		const std::vector<UInt8>& bufferData = GetMaterial().GetUniformBufferConstData(m_uniformBlockIndex);

		const float* colorPtr = AccessByOffset<const float*>(bufferData.data(), m_phongUniformOffsets.specularColor);
		return Color(colorPtr[0] * 255, colorPtr[1] * 255, colorPtr[2] * 255, colorPtr[3] * 255); //< TODO: Make color able to use float
	}

	void PhongLightingMaterial::SetAmbientColor(const Color& ambient)
	{
		NazaraAssert(HasAmbientColor(), "Material has no ambient color uniform");

		std::vector<UInt8>& bufferData = GetMaterial().GetUniformBufferData(m_uniformBlockIndex);
		float* colorPtr = AccessByOffset<float*>(bufferData.data(), m_phongUniformOffsets.ambientColor);
		colorPtr[0] = ambient.r / 255.f;
		colorPtr[1] = ambient.g / 255.f;
		colorPtr[2] = ambient.b / 255.f;
		colorPtr[3] = ambient.a / 255.f;
	}

	void PhongLightingMaterial::SetShininess(float shininess)
	{
		NazaraAssert(HasShininess(), "Material has no shininess uniform");

		std::vector<UInt8>& bufferData = GetMaterial().GetUniformBufferData(m_uniformBlockIndex);
		AccessByOffset<float&>(bufferData.data(), m_phongUniformOffsets.shininess) = shininess;
	}

	void PhongLightingMaterial::SetSpecularColor(const Color& diffuse)
	{
		NazaraAssert(HasSpecularColor(), "Material has no specular color uniform");

		std::vector<UInt8>& bufferData = GetMaterial().GetUniformBufferData(m_uniformBlockIndex);
		float* colorPtr = AccessByOffset<float*>(bufferData.data(), m_phongUniformOffsets.specularColor);
		colorPtr[0] = diffuse.r / 255.f;
		colorPtr[1] = diffuse.g / 255.f;
		colorPtr[2] = diffuse.b / 255.f;
		colorPtr[3] = diffuse.a / 255.f;
	}

	const std::shared_ptr<MaterialSettings>& PhongLightingMaterial::GetSettings()
	{
		return s_phongMaterialSettings;
	}

	MaterialSettings::Builder PhongLightingMaterial::Build(PhongBuildOptions& options)
	{
		MaterialSettings::Builder settings = BasicMaterial::Build(options);

		assert(settings.uniformBlocks.size() == 1);
		std::vector<MaterialSettings::UniformVariable> variables = std::move(settings.uniformBlocks.front().uniforms);
		settings.uniformBlocks.clear();

		if (options.phongOffsets.ambientColor != std::numeric_limits<std::size_t>::max())
		{
			variables.push_back({
				"AmbientColor",
				options.phongOffsets.ambientColor
			});
		}

		if (options.phongOffsets.shininess != std::numeric_limits<std::size_t>::max())
		{
			variables.push_back({
				"Shininess",
				options.phongOffsets.shininess
			});
		}

		if (options.phongOffsets.shininess != std::numeric_limits<std::size_t>::max())
		{
			variables.push_back({
				"SpecularColor",
				options.phongOffsets.specularColor
			});
		}

		static_assert(sizeof(Vector4f) == 4 * sizeof(float), "Vector4f is expected to be exactly 4 floats wide");

		if (options.phongOffsets.ambientColor != std::numeric_limits<std::size_t>::max())
			AccessByOffset<Vector4f&>(options.defaultValues.data(), options.phongOffsets.ambientColor) = Vector4f(0.f, 0.f, 0.f, 1.f);

		if (options.phongOffsets.specularColor != std::numeric_limits<std::size_t>::max())
			AccessByOffset<Vector4f&>(options.defaultValues.data(), options.phongOffsets.specularColor) = Vector4f(1.f, 1.f, 1.f, 1.f);

		if (options.phongOffsets.shininess != std::numeric_limits<std::size_t>::max())
			AccessByOffset<float&>(options.defaultValues.data(), options.phongOffsets.shininess) = 2.f;

		// Textures
		if (options.phongTextureIndexes)
			options.phongTextureIndexes->emissive = settings.textures.size();

		settings.textures.push_back({
			7,
			"Emissive",
			ImageType::E2D
		});

		if (options.phongTextureIndexes)
			options.phongTextureIndexes->height = settings.textures.size();

		settings.textures.push_back({
			8,
			"Height",
			ImageType::E2D
		});

		if (options.phongTextureIndexes)
			options.phongTextureIndexes->normal = settings.textures.size();

		settings.textures.push_back({
			9,
			"Normal",
			ImageType::E2D
		});

		if (options.phongTextureIndexes)
			options.phongTextureIndexes->specular = settings.textures.size();

		settings.textures.push_back({
			10,
			"Specular",
			ImageType::E2D
		});

		if (options.uniformBlockIndex)
			*options.uniformBlockIndex = settings.uniformBlocks.size();

		settings.uniformBlocks.push_back({
			0,
			"MaterialSettings",
			options.phongOffsets.totalSize,
			std::move(variables),
			options.defaultValues
		});

		settings.sharedUniformBlocks.push_back(PredefinedLightData::GetUniformBlock(6, ShaderStageType::Fragment));
		settings.predefinedBindings[UnderlyingCast(PredefinedShaderBinding::LightDataUbo)] = 6;

		settings.shaders = options.shaders;

		for (std::shared_ptr<UberShader> uberShader : settings.shaders)
		{
			constexpr std::size_t InvalidOption = std::numeric_limits<std::size_t>::max();

			auto FetchLocationOption = [&](const std::string& optionName)
			{
				const UberShader::Option* optionPtr;
				if (!uberShader->HasOption(optionName, &optionPtr))
					return InvalidOption;

				//if (optionPtr->type != ShaderAst::ExpressionType{ ShaderAst::PrimitiveType::Int32 })
				//	throw std::runtime_error("Location options must be of type i32");

				return optionPtr->index;
			};

			std::size_t positionLocationIndex = FetchLocationOption("PosLocation");
			std::size_t colorLocationIndex = FetchLocationOption("ColorLocation");
			std::size_t normalLocationIndex = FetchLocationOption("NormalLocation");
			std::size_t tangentLocationIndex = FetchLocationOption("TangentLocation");
			std::size_t uvLocationIndex = FetchLocationOption("UvLocation");

			uberShader->UpdateConfigCallback([=](UberShader::Config& config, const std::vector<RenderPipelineInfo::VertexBufferData>& vertexBuffers)
			{
				if (vertexBuffers.empty())
					return;

				const VertexDeclaration& vertexDeclaration = *vertexBuffers.front().declaration;
				const auto& components = vertexDeclaration.GetComponents();

				std::size_t locationIndex = 0;
				for (const auto& component : components)
				{
					switch (component.component)
					{
						case VertexComponent::Position:
							if (positionLocationIndex != InvalidOption)
								config.optionValues[positionLocationIndex] = static_cast<Int32>(locationIndex);

							break;

						case VertexComponent::Color:
							if (colorLocationIndex != InvalidOption)
								config.optionValues[colorLocationIndex] = static_cast<Int32>(locationIndex);

							break;

						case VertexComponent::Normal:
							if (normalLocationIndex != InvalidOption)
								config.optionValues[normalLocationIndex] = static_cast<Int32>(locationIndex);

							break;

						case VertexComponent::Tangent:
							if (tangentLocationIndex != InvalidOption)
								config.optionValues[tangentLocationIndex] = static_cast<Int32>(locationIndex);

							break;

						case VertexComponent::TexCoord:
							if (uvLocationIndex != InvalidOption)
								config.optionValues[uvLocationIndex] = static_cast<Int32>(locationIndex);

							break;

						case VertexComponent::Unused:
						default:
							break;
					}

					++locationIndex;
				}
			});
		}

		// Options

		// HasEmissiveMap
		if (options.phongOptionIndexes)
			options.phongOptionIndexes->hasEmissiveMap = settings.options.size();

		MaterialSettings::BuildOption(settings.options, settings.shaders, "HasEmissiveMap", "HasEmissiveTexture");

		// HasHeightMap
		if (options.phongOptionIndexes)
			options.phongOptionIndexes->hasHeightMap = settings.options.size();

		MaterialSettings::BuildOption(settings.options, settings.shaders, "HasHeightMap", "HasHeightTexture");

		// HasNormalMap
		if (options.phongOptionIndexes)
			options.phongOptionIndexes->hasNormalMap = settings.options.size();

		MaterialSettings::BuildOption(settings.options, settings.shaders, "HasNormalMap", "HasNormalTexture");

		// HasSpecularMap
		if (options.phongOptionIndexes)
			options.phongOptionIndexes->hasSpecularMap = settings.options.size();

		MaterialSettings::BuildOption(settings.options, settings.shaders, "HasSpecularMap", "HasSpecularTexture");

		return settings;
	}

	std::vector<std::shared_ptr<UberShader>> PhongLightingMaterial::BuildShaders()
	{
		ShaderAst::ModulePtr shaderModule;

#ifdef NAZARA_DEBUG
		std::filesystem::path shaderPath = "../../src/Nazara/Graphics/Resources/Shaders/phong_material.nzsl";
		if (std::filesystem::exists(shaderPath))
		{
			try
			{
				shaderModule = ShaderLang::ParseFromFile(shaderPath);
			}
			catch (const std::exception& e)
			{
				NazaraError(std::string("failed to load shader from engine folder: ") + e.what());
			}
		}
#endif

		if (!shaderModule)
			shaderModule = ShaderLang::Parse(std::string_view(reinterpret_cast<const char*>(r_shader), sizeof(r_shader)));

		auto shader = std::make_shared<UberShader>(ShaderStageType::Fragment | ShaderStageType::Vertex, std::move(shaderModule));

		return { std::move(shader) };
	}

	auto PhongLightingMaterial::BuildUniformOffsets() -> std::pair<PhongUniformOffsets, FieldOffsets>
	{
		auto basicOffsets = BasicMaterial::BuildUniformOffsets();
		FieldOffsets fieldOffsets = basicOffsets.second;

		PhongUniformOffsets uniformOffsets;
		uniformOffsets.ambientColor = fieldOffsets.AddField(StructFieldType::Float3);
		uniformOffsets.specularColor = fieldOffsets.AddField(StructFieldType::Float3);
		uniformOffsets.shininess = fieldOffsets.AddField(StructFieldType::Float1);

		uniformOffsets.totalSize = fieldOffsets.GetAlignedSize();

		return std::make_pair(std::move(uniformOffsets), std::move(fieldOffsets));
	}

	bool PhongLightingMaterial::Initialize()
	{
		std::tie(s_phongUniformOffsets, std::ignore) = BuildUniformOffsets();

		std::vector<UInt8> defaultValues(s_phongUniformOffsets.totalSize);

		PhongBuildOptions options;
		options.defaultValues = std::move(defaultValues);
		options.shaders = BuildShaders();

		// Basic material
		options.basicOffsets = s_basicUniformOffsets;

		// Phong Material
		options.phongOffsets = s_phongUniformOffsets;
		options.phongOptionIndexes = &s_phongOptionIndexes;
		options.phongTextureIndexes = &s_phongTextureIndexes;

		s_phongMaterialSettings = std::make_shared<MaterialSettings>(Build(options));

		return true;
	}

	void PhongLightingMaterial::Uninitialize()
	{
		s_phongMaterialSettings.reset();
	}

	std::shared_ptr<MaterialSettings> PhongLightingMaterial::s_phongMaterialSettings;
	std::size_t PhongLightingMaterial::s_phongUniformBlockIndex;
	PhongLightingMaterial::PhongOptionIndexes PhongLightingMaterial::s_phongOptionIndexes;
	PhongLightingMaterial::PhongTextureIndexes PhongLightingMaterial::s_phongTextureIndexes;
	PhongLightingMaterial::PhongUniformOffsets PhongLightingMaterial::s_phongUniformOffsets;
}
