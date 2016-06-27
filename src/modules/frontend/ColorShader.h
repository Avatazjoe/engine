/**
 * @file
 */

#pragma once

#include "video/Shader.h"

namespace frontend {

class ColorShader : public video::Shader {
public:
	bool setup() {
		if (!loadProgram("shaders/color")) {
			return false;
		}
		// no attributes
		checkUniforms({"u_view", "u_projection"});
		return true;
	}

	inline bool setView(const glm::mat4& u_view) const {
		if (!hasUniform("u_view")) {
			return false;
		}
		setUniformMatrix("u_view", u_view);
		return true;
	}

	inline bool setProjection(const glm::mat4& u_projection) const {
		if (!hasUniform("u_projection")) {
			return false;
		}
		setUniformMatrix("u_projection", u_projection);
		return true;
	}

};

typedef std::shared_ptr<ColorShader> ColorShaderPtr;

}
