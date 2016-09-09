/**
 * @file
 */

#include "AICharacter.h"
#include "backend/entity/Npc.h"
#include "core/String.h"

namespace backend {

AICharacter::AICharacter(ai::CharacterId id, Npc& npc) :
		ai::ICharacter(id), _npc(npc) {
	setOrientation(ai::randomf(glm::two_pi<float>()));
	setAttribute(ai::attributes::NAME, npc.name() + " " + std::to_string(id));
	setAttribute(ai::attributes::ID, std::to_string(id));
}

AICharacter::~AICharacter() {
}

void AICharacter::update(int64_t dt, bool debuggingActive) {
	_npc.moveToGround();

	// TODO: attrib for passive aggro
	if (true) {
		_npc.visitVisible([&] (const EntityPtr& e) {
			ai::AggroMgr& aggro = _npc.ai()->getAggroMgr();
			aggro.addAggro(e->id(), dt / 1000.0);
		});
	}

	if (debuggingActive) {
		setAttribute(ai::attributes::POSITION, ai::Str::toString(getPosition()));
		setAttribute(ai::attributes::ORIENTATION, std::to_string(ai::toDegrees(getOrientation())));
		const attrib::Attributes& attribs =  _npc._attribs;
		for (int i = 0; i <= (int)attrib::Type::MAX; ++i) {
			const attrib::Type attribType = (attrib::Type)i;
			const double current = attribs.current(attribType);
			const double max = attribs.max(attribType);
			setAttribute(network::EnumNameAttribType(attribType), core::string::format("%f/%f", current, max));
		}
	}
	ai::ICharacter::update(dt, debuggingActive);
}

}
