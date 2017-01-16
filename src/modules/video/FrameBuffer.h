/**
 * @file
 */

#pragma once

#include "Renderer.h"
#include <glm/vec2.hpp>

namespace video {

class FrameBuffer {
	friend class ScopedFrameBuffer;
private:
	Id _fbo = video::InvalidId;
	Id _texture = video::InvalidId;
	Id _depth = video::InvalidId;

	glm::ivec2 _dimension;

	int32_t _viewport[4] = {0, 0, 0, 0};
public:
	~FrameBuffer();

	bool init(const glm::ivec2& dimension);
	void shutdown();

	void bind();
	void unbind();

	Id texture() const;

	const glm::ivec2& dimension() const;
};

inline Id FrameBuffer::texture() const {
	return _texture;
}

inline const glm::ivec2& FrameBuffer::dimension() const {
	return _dimension;
}

}
