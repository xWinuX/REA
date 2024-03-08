#include "REA/System/RenderingPreparation.hpp"

#include <SplitEngine/Rendering/Shader.hpp>

namespace REA::System
{
	void RenderingPreparation::RunExecute(ECS::Context& context) { Rendering::Shader::UpdateGlobal(); }
}
