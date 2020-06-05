#include <ShaderNode/DataModels/VecBinOp.hpp>
#include <Nazara/Renderer/ShaderBuilder.hpp>

template<Nz::ShaderAst::BinaryType BinOp>
VecBinOp<BinOp>::VecBinOp(ShaderGraph& graph) :
ShaderNode(graph)
{
	UpdateOutput();
}

template<Nz::ShaderAst::BinaryType BinOp>
Nz::ShaderAst::ExpressionPtr VecBinOp<BinOp>::GetExpression(Nz::ShaderAst::ExpressionPtr* expressions, std::size_t count) const
{
	assert(count == 2);
	using BuilderType = typename Nz::ShaderBuilder::template BinOpBuilder<BinOp>;
	constexpr BuilderType builder;
	return builder(expressions[0], expressions[1]);
}

template<Nz::ShaderAst::BinaryType BinOp>
QtNodes::NodeDataType VecBinOp<BinOp>::dataType(QtNodes::PortType /*portType*/, QtNodes::PortIndex portIndex) const
{
	assert(portIndex == 0 || portIndex == 1);

	return VecData::Type();
}

template<Nz::ShaderAst::BinaryType BinOp>
unsigned int VecBinOp<BinOp>::nPorts(QtNodes::PortType portType) const
{
	switch (portType)
	{
		case QtNodes::PortType::In:  return 2;
		case QtNodes::PortType::Out: return 1;
	}

	return 0;
}

template<Nz::ShaderAst::BinaryType BinOp>
std::shared_ptr<QtNodes::NodeData> VecBinOp<BinOp>::outData(QtNodes::PortIndex port)
{
	assert(port == 0);
	return m_output;
}

template<Nz::ShaderAst::BinaryType BinOp>
void VecBinOp<BinOp>::setInData(std::shared_ptr<QtNodes::NodeData> value, int index)
{
	assert(index == 0 || index == 1);

	std::shared_ptr<VecData> castedValue;
	if (value)
	{
		assert(dynamic_cast<VecData*>(value.get()) != nullptr);

		castedValue = std::static_pointer_cast<VecData>(value);
	}

	if (index == 0)
		m_lhs = std::move(castedValue);
	else
		m_rhs = std::move(castedValue);

	UpdateOutput();
}

template<Nz::ShaderAst::BinaryType BinOp>
bool VecBinOp<BinOp>::ComputePreview(QPixmap& pixmap)
{
	if (!m_lhs || !m_rhs)
		return false;

	pixmap = QPixmap::fromImage(m_output->preview);
	return true;
}

template<Nz::ShaderAst::BinaryType BinOp>
void VecBinOp<BinOp>::UpdateOutput()
{
	if (!m_lhs || !m_rhs || m_lhs->componentCount != m_rhs->componentCount)
	{
		m_output = std::make_shared<VecData>(4);
		m_output->preview = QImage(1, 1, QImage::Format_RGBA8888);
		m_output->preview.fill(QColor::fromRgb(0, 0, 0, 0));
		return;
	}

	m_output = std::make_shared<VecData>(m_lhs->componentCount);

	const QImage& leftPreview = m_lhs->preview;
	const QImage& rightPreview = m_rhs->preview;
	int maxWidth = std::max(leftPreview.width(), rightPreview.width());
	int maxHeight = std::max(leftPreview.height(), rightPreview.height());

	// Exploit COW
	QImage leftResized = leftPreview;
	if (leftResized.width() != maxWidth || leftResized.height() != maxHeight)
		leftResized = leftResized.scaled(maxWidth, maxHeight);

	QImage rightResized = rightPreview;
	if (rightResized.width() != maxWidth || rightResized.height() != maxHeight)
		rightResized = rightResized.scaled(maxWidth, maxHeight);

	m_output->preview = QImage(maxWidth, maxHeight, QImage::Format_RGBA8888);
	ApplyOp(leftResized.constBits(), rightResized.constBits(), m_output->preview.bits(), maxWidth * maxHeight * 4);

	Q_EMIT dataUpdated(0);

	UpdatePreview();
}