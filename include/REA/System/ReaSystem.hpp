#pragma once
#include <SplitEngine/Application.hpp>

using namespace SplitEngine;

namespace REA
{
	namespace System
	{
		template<typename... T>
		class ReaSystem : public ECS::SystemBase
		{
			public:
				ReaSystem()
				{
					_signature.ExtendSizeBy(TypeIDGenerator<ECS::Component>::GetCount());
					(_signature.SetBit(TypeIDGenerator<ECS::Component>::GetID<T>()), ...);
				}

			protected:
				void RunExecute(ECS::ContextProvider& contextProvider, uint8_t stage) final
				{
					if (!_cachedArchetypes)
					{
						_archetypes = contextProvider.Registry->GetArchetypesWithSignature(_signature);

						_archetypes.erase(std::remove_if(_archetypes.begin(), _archetypes.end(), [](ECS::Archetype* archetype) { return archetype->Entities.empty(); }),
						                  _archetypes.end());

						_cachedArchetypes = true;
					}

					if (!_archetypes.empty()) { ExecuteArchetypes(_archetypes, contextProvider, stage); }
				}

				virtual void ExecuteArchetypes(std::vector<ECS::Archetype*>& archetypes, ECS::ContextProvider& contextProvider, uint8_t stage)
				{
					for (ECS::Archetype* archetype: archetypes)
					{
						std::apply([this, &archetype, &contextProvider, stage](T*... components) { Execute(components..., archetype->Entities, contextProvider, stage); },
						           std::make_tuple(reinterpret_cast<T*>(archetype->GetComponentsRaw<T>().data())...));
					}
				}

				virtual void Execute(T*..., std::vector<uint64_t>& entities, ECS::ContextProvider& context, uint8_t stage) {}

			private:
				std::vector<ECS::Archetype*> _archetypes;

				DynamicBitSet _signature{};
		};
	};
}
