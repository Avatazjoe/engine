/**
 * @file
 */

#pragma once

#include "core/Var.h"
#include <algorithm>
#include <vector>

namespace util {

template<class Functor>
static void visitVarSorted(Functor&& func, uint32_t flags) {
	std::vector<core::VarPtr> varList;
	core::Var::visit([&](const core::VarPtr& var) {
		if (flags == 0 || (var->getFlags() & flags) != 0) {
			varList.push_back(var);
		}
	});
	std::sort(varList.begin(), varList.end(), [] (const core::VarPtr& v1, const core::VarPtr& v2) {
		return v1->name() < v2->name();
	});
	for (const core::VarPtr& var : varList) {
		func(var);
	}
}


}
