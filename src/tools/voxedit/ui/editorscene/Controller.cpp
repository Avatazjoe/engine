#include "Controller.h"
#include "frontend/Movement.h"

void Controller::resetCamera(const voxel::RawVolume* volume) {
	_camera.setAngles(0.0f, 0.0f, 0.0f);

	if (volume == nullptr) {
		return;
	}
	const voxel::Region& region = volume->getEnclosingRegion();
	const glm::ivec3& center = region.getCentre();
	_camera.setTarget(center);
	if (_camMode == Controller::SceneCameraMode::Free) {
		_camera.setPosition(glm::vec3(-center));
		_camera.lookAt(glm::vec3(0.0001f));
	} else if (_camMode == Controller::SceneCameraMode::Top) {
		_camera.setPosition(glm::vec3(center.x, region.getHeightInCells() + center.y, center.z));
		_camera.lookAt(glm::down);
	} else if (_camMode == Controller::SceneCameraMode::Left) {
		_camera.setPosition(glm::vec3(region.getWidthInCells() + center.x, center.y, center.z));
		_camera.lookAt(glm::right);
	} else if (_camMode == Controller::SceneCameraMode::Front) {
		_camera.setPosition(glm::vec3(center.x, center.y, region.getDepthInCells() + center.z));
		_camera.lookAt(glm::backward);
	}
}

void Controller::changeCameraRotationType(video::CameraRotationType type) {
	_camera.setRotationType(type);
}

void Controller::update(long deltaFrame) {
	const float speed = _cameraSpeed * static_cast<float>(deltaFrame);
	const glm::vec3& moveDelta = getMoveDelta(speed, _moveMask);
	_camera.move(moveDelta);
	_camera.update(deltaFrame);
}

void Controller::init(Controller::SceneCameraMode mode) {
	switch (mode) {
	case SceneCameraMode::Top:
	case SceneCameraMode::Front:
	case SceneCameraMode::Left:
		_camera.setMode(video::CameraMode::Orthogonal);
		break;
	case SceneCameraMode::Free:
		_camera.setMode(video::CameraMode::Perspective);
		break;
	}
	_rotationSpeed = core::Var::get(cfg::ClientMouseRotationSpeed, "0.01");
}

void Controller::onResize(const glm::ivec2& pos, const glm::ivec2& size) {
	_camera.init(glm::ivec2(), size);
}

void Controller::zoom(float level) {
	const float value = _cameraSpeed * level;
	const glm::vec3& moveDelta = glm::backward * value;
	_camera.move(moveDelta);
	const float targetDistance = glm::clamp(_camera.targetDistance() - value, 0.0f, 1000.0f);
	_camera.setTargetDistance(targetDistance);
}

bool Controller::move(bool rotate, int x, int y) {
	if (rotate) {
		const float yaw = x - _mouseX;
		const float pitch = y - _mouseY;
		const float s = _rotationSpeed->floatVal();
		if (_camMode == Controller::SceneCameraMode::Free) {
			_camera.turn(yaw * s);
			_camera.pitch(pitch * s);
		}
		_mouseX = x;
		_mouseY = y;
		return false;
	}
	_mouseX = x;
	_mouseY = y;
	return true;
}
