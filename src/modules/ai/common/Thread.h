/**
 * @file
 */
#pragma once

#include <atomic>
#include "core/String.h"
#include <thread>

namespace ai {

// TODO: not a real read-write-lock - maybe someday
class ReadWriteLock {
private:
	const core::String _name;
	mutable std::atomic_flag _locked = ATOMIC_FLAG_INIT;
public:
	ReadWriteLock(const core::String& name) :
			_name(name) {
	}

	inline void lockRead() const {
		while (_locked.test_and_set(std::memory_order_acquire)) {
			std::this_thread::yield();
		}
	}

	inline void unlockRead() const {
		_locked.clear(std::memory_order_release);
	}

	inline void lockWrite() {
		while (_locked.test_and_set(std::memory_order_acquire)) {
			std::this_thread::yield();
		}
	}

	inline void unlockWrite() {
		_locked.clear(std::memory_order_release);
	}
};

class ScopedReadLock {
private:
	const ReadWriteLock& _lock;
public:
	explicit ScopedReadLock(const ReadWriteLock& lock) : _lock(lock) {
		_lock.lockRead();
	}
	~ScopedReadLock() {
		_lock.unlockRead();
	}
};

class ScopedWriteLock {
private:
	ReadWriteLock& _lock;
public:
	explicit ScopedWriteLock(ReadWriteLock& lock) : _lock(lock) {
		_lock.lockWrite();
	}
	~ScopedWriteLock() {
		_lock.unlockWrite();
	}
};

#ifndef AI_THREAD_LOCAL
#if defined(__APPLE__)
// XCode8 clang will support thread_local
#define AI_THREAD_LOCAL static
#else
#define AI_THREAD_LOCAL thread_local
#endif
#endif

}
