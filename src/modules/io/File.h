/**
 * @file
 */

#pragma once

#include <string>
#include <memory>
#include "IOResource.h"

struct SDL_RWops;

namespace io {

/**
 * @brief Wrapper for file based io.
 *
 * @see FileSystem
 */
class File : public IOResource {
	friend class FileStream;
protected:
	SDL_RWops* _file;
	const std::string _rawPath;

	void close();
	int read(void *buf, size_t size, size_t maxnum);
	long tell() const;
	long seek(long offset, int seekType) const;

public:
	File(const std::string& rawPath);
	virtual ~File();

	bool exists() const;
	long length() const;
	std::string getExtension() const;
	std::string getPath() const;
	std::string getFileName() const;

	SDL_RWops* createRWOps() const;
	long write(const unsigned char *buf, size_t len) const;
	int read(void **buffer);
	int read(void *buffer, int n);
	const std::string& getName() const;
	std::string load();
};

inline bool File::exists() const {
	return _file != nullptr;
}

typedef std::shared_ptr<File> FilePtr;

}
