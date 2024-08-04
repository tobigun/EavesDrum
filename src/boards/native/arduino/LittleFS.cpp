// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

// LittleFS Wrapper for Posix

#include <FSImpl.h>

using namespace fs;

class LittleFSFileImpl : public FileImpl {
public:
  LittleFSFileImpl(const String path, FILE* file) {
    this->path = path;
    this->f = file;
  }

  virtual size_t write(const uint8_t* buf, size_t size) {
    return fwrite(buf, size, 1, f);
  }

  virtual int read(uint8_t* buf, size_t size) {
    return fread(buf, size, 1, f);
  }

  virtual void flush() {
    fflush(f);
  }

  virtual bool seek(uint32_t pos, SeekMode mode) {
    return fseek(f, pos, (mode == SeekSet ? SEEK_SET : (mode == SeekCur ? SEEK_CUR : SEEK_END))) == 0;
  }

  virtual size_t position() const {
    return ftell(f);
  }

  virtual size_t size() const {
    fseek(f, 0L, SEEK_END);
    return ftell(f);
  }

  virtual bool truncate(uint32_t size) {
    return false;
  }

  virtual void close() {
    fclose(f);
  }

  virtual const char* name() const { return path.c_str(); }

  virtual const char* fullName() const { return path.c_str(); }

  virtual bool isFile() const {
    return true;
  }

  virtual bool isDirectory() const {
    return false;
  }

private:
  FILE* f;
  String path;
};

class LittleFSImpl : public FSImpl {
public:
  virtual bool setConfig(const FSConfig& cfg) { return true; }
  virtual bool begin() { return true; }
  virtual void end() {}
  virtual bool format() { return true; }
  virtual bool info(FSInfo& info) { return true; }

  virtual FileImplPtr open(const char* path, OpenMode openMode, AccessMode accessMode) {
    FILE* file = NULL;
    String filePath = getRealPath(path);
    if (accessMode == AM_READ) {
      file = fopen(filePath.c_str(), "r");
    } else if (accessMode == AM_WRITE) {
      file = fopen(filePath.c_str(), "w");
    } else if (accessMode == AM_RW) {
      file = fopen(filePath.c_str(), "rw");
    }

    if (!file) {
      return std::shared_ptr<FileImpl>();
    }

    LittleFSFileImpl* impl = new LittleFSFileImpl(filePath, file);
    return std::shared_ptr<FileImpl>(impl);
  }

  virtual bool exists(const char* path) {
    return access(getRealPath(path).c_str(), F_OK) == 0;
  }

  virtual DirImplPtr openDir(const char* path) { return nullptr; }
  virtual bool rename(const char* pathFrom, const char* pathTo) { return false; }
  virtual bool remove(const char* path) { return false; }
  virtual bool mkdir(const char* path) { return false; }
  virtual bool rmdir(const char* path) { return false; }
  virtual bool stat(const char* path, FSStat* st) { return false; }

private:
  String getRealPath(const char* path) {
    if (String(path).endsWith("config.yaml")) {
      return String("./config/config.yaml");
    } else {
      return String("./data/") + path;
    }
  }
};

FS LittleFS(std::shared_ptr<LittleFSImpl>(new LittleFSImpl()));
