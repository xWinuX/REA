#include "REA/System/RenderingPreparation.hpp"

#include <SplitEngine/Rendering/Shader.hpp>

namespace REA::System
{
	void RenderingPreparation::RunExecute(ECS::ContextProvider& contextProvider, uint8_t stage) { Rendering::Shader::UpdateGlobal(); }
}
