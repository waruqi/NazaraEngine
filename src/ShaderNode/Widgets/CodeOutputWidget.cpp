#include <ShaderNode/Widgets/CodeOutputWidget.hpp>
#include <Nazara/Shader/GlslWriter.hpp>
#include <Nazara/Shader/LangWriter.hpp>
#include <Nazara/Shader/Ast/AstConstantPropagationVisitor.hpp>
#include <Nazara/Shader/Ast/EliminateUnusedPassVisitor.hpp>
#include <Nazara/Shader/Ast/SanitizeVisitor.hpp>
#include <Nazara/Shader/SpirvPrinter.hpp>
#include <Nazara/Shader/SpirvWriter.hpp>
#include <ShaderNode/ShaderGraph.hpp>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QVBoxLayout>

enum class OutputLanguage
{
	GLSL,
	NZSL,
	SpirV
};

CodeOutputWidget::CodeOutputWidget(const ShaderGraph& shaderGraph) :
m_shaderGraph(shaderGraph)
{
	m_textOutput = new QTextEdit;

	m_outputLang = new QComboBox;
	m_outputLang->addItem("GLSL", int(OutputLanguage::GLSL));
	m_outputLang->addItem("NZSL", int(OutputLanguage::NZSL));
	m_outputLang->addItem("SPIR-V", int(OutputLanguage::SpirV));
	connect(m_outputLang, qOverload<int>(&QComboBox::currentIndexChanged), [this](int)
	{
		Refresh();
	});

	m_optimisationCheckbox = new QCheckBox;
	m_optimisationCheckbox->setText("Enable optimisation");
	connect(m_optimisationCheckbox, &QCheckBox::stateChanged, [this](int)
	{
		Refresh();
	});

	QVBoxLayout* verticalLayout = new QVBoxLayout;
	verticalLayout->addWidget(m_textOutput);
	verticalLayout->addWidget(m_outputLang);
	verticalLayout->addWidget(m_optimisationCheckbox);

	setLayout(verticalLayout);

	Refresh();
}

void CodeOutputWidget::Refresh()
{
	try
	{
		Nz::ShaderWriter::States states;

		for (std::size_t i = 0; i < m_shaderGraph.GetOptionCount(); ++i)
		{
			const auto& option = m_shaderGraph.GetOption(i);
			states.optionValues[Nz::CRC32(option.name)] = m_shaderGraph.IsOptionEnabled(i);
		}

		Nz::ShaderAst::ModulePtr shaderModule = m_shaderGraph.ToModule();

		if (m_optimisationCheckbox->isChecked())
		{
			Nz::ShaderAst::SanitizeVisitor::Options sanitizeOptions;
			sanitizeOptions.optionValues = states.optionValues;

			shaderModule = Nz::ShaderAst::Sanitize(*shaderModule, sanitizeOptions);

			Nz::ShaderAst::AstConstantPropagationVisitor optimiser;
			shaderModule = Nz::ShaderAst::PropagateConstants(*shaderModule);
			shaderModule = Nz::ShaderAst::EliminateUnusedPass(*shaderModule);
		}

		std::string output;
		OutputLanguage outputLang = static_cast<OutputLanguage>(m_outputLang->currentIndex());
		switch (outputLang)
		{
			case OutputLanguage::GLSL:
			{
				Nz::GlslWriter::BindingMapping bindingMapping;
				for (const auto& buffer : m_shaderGraph.GetBuffers())
					bindingMapping.emplace(Nz::UInt64(buffer.setIndex) << 32 | Nz::UInt64(buffer.bindingIndex), bindingMapping.size());

				for (const auto& texture : m_shaderGraph.GetTextures())
					bindingMapping.emplace(Nz::UInt64(texture.setIndex) << 32 | Nz::UInt64(texture.bindingIndex), bindingMapping.size());

				Nz::GlslWriter writer;
				output = writer.Generate(ShaderGraph::ToShaderStageType(m_shaderGraph.GetType()), *shaderModule, bindingMapping, states);
				break;
			}

			case OutputLanguage::NZSL:
			{
				Nz::LangWriter writer;
				output = writer.Generate(*shaderModule, states);
				break;
			}

			case OutputLanguage::SpirV:
			{
				Nz::SpirvWriter writer;
				std::vector<std::uint32_t> spirv = writer.Generate(*shaderModule, states);

				Nz::SpirvPrinter printer;
				output = printer.Print(spirv.data(), spirv.size());
				break;
			}
		}

		int scrollValue = m_textOutput->verticalScrollBar()->value();
		m_textOutput->setText(QString::fromStdString(output));
		m_textOutput->verticalScrollBar()->setValue(scrollValue);
	}
	catch (const std::exception& e)
	{
		m_textOutput->setText("Generation failed: " + QString::fromStdString(e.what()));
	}
}
