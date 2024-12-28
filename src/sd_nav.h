#pragma once

#include <SD.h>
#include <FS.h>
#include <cstring>

class DirPtr
{
private:
  bool root;
  File dir;
  DirPtr *parent = nullptr;
  int32_t dirIndex = -1;
  String currentName;

  void copyPath(char *dest)
  {
    if (isRoot())
    {
      return;
    }
    parent->copyPath(dest);
    strcat(dest, dir.name());
  }

public:
  DirPtr(File &d, DirPtr *p)
  {
    dir = d;
    parent = p;
    root = false;
  }
  DirPtr(File &d)
  {
    dir = d;
    root = true;
  }
  ~DirPtr()
  {
    dir.close();
  }

  bool isRoot()
  {
    return root;
  }

  File getNext()
  {
    dirIndex++;
    return dir.openNextFile();
  }

  File getPrev()
  {
    log_d("Get prev in dir: %s, index: %d", dir.name(), dirIndex);
    if (0 == dirIndex) // at the top of dir
    {
      File f;
      return f; // return empty file
    }
    else if (dirIndex > 0)
    {
      dirIndex--;
      dir.seekDir(dirIndex);
      return dir.openNextFile();
    }
    else
    {
      updatePosition();
      return getPrev();
    }
  }

  void updatePosition()
  {
    log_d("Updating position: %s", currentName.c_str());
    dir.rewindDirectory();
    File f;
    while (true)
    {
      f = getNext();
      if (!f)
      {
        break;
      }
      log_d("The next file is: %s", f.name());
      if (f && 0 == strcmp(currentName.c_str(), f.name()))
      {
        log_d("Found current name: %s with index: %d", f.name(), dirIndex);
        break;
      }
      f.close();
    }
    f.close();
    if (dirIndex < 0)
    {
      dirIndex = 0;
    }
    log_d("Updated dir index: %d", dirIndex);
  }

  void setCurrentName(String name)
  {
    currentName = name;
  }

  DirPtr *close()
  {
    if (root)
    {
      dir.rewindDirectory();
      return this;
    }
    dir.close();
    return parent;
  }

  const char *getFullPath(const char *file)
  {
    return dir.path();
  }

  const char *name()
  {
    return dir.name();
  }
};

namespace Path
{
  /**
   * Get extension prepended with '.' or empty string
   */
  String getExt(const char *path)
  {
    auto p = strrchr(path, '.');
    if (nullptr == p)
    {
      return emptyString;
    }
    return String(p);
  }

  /**
   * Get file/directory name (last part after '/')
   */
  String getName(const char *path)
  {
    auto p = strrchr(path, '/');
    // e.g: file-path
    if (nullptr == p)
    {
      return String(path);
    }
    // // e.g: /file/path/
    // if (strchr(p, '/') == p)
    // {
    //   return emptyString;
    // }
    return String(p + 1);
  }

  /**
   * Get file name w.o. extension
   */
  String getTitle(const char *path)
  {
    auto p = strrchr(path, '/');
    if (nullptr == p)
    {
      p = (char *)path;
    }
    else
    {
      p += 1;
    }
    auto d = strchr(p, '.');
    if (d == nullptr)
    {
      return String(d);
    }
    auto len = d - p;
    char *title = new char[len + 1];
    strncpy(title, p, len);
    title[len] = '\0';
    return String(title);
  }

  /**
   * Get directory name (before last '/')
   */
  String getDirectory(const char *path)
  {
    auto p = strrchr(path, '/');
    // e.g: file-path or /file-path
    if (nullptr == p || path == p)
    {
      return String("/");
    }
    return String(path, p - path);
  }
}

class SDNav
{
private:
  const char *FILE_FILTER = ".mp3.wav.flac.aac.ogg.m4a";

  fs::FS *_fs;
  DirPtr *cDir;
  int _fileIndex = 0;
  String startFile;

  void restoreDirChain(const char *buff, const char *from)
  {
    log_d("restoreDirChain %s, %s", buff, from);
    auto p = strchr(from, '/');
    if (p != nullptr && p > from)
    {
      auto len = p - buff;
      char *path = new char[len + 1];
      strncpy(path, buff, len);
      path[len] = '\0';
      log_d("Path is %s", path);
      auto dir = _fs->open(String(path));
      cDir->setCurrentName(String(dir.name()));
      log_d("Set current name %s", dir.name());
      if (dir.isDirectory())
      {
        log_d("Update cDir");
        cDir = new DirPtr(dir, cDir);
      }
      free(path);
      restoreDirChain(buff, p + 1);
    }
    else
    {
      char *str = strdup(from);
      log_d("Set current name %s", str);
      cDir->setCurrentName(String(str));
      free(str);
    }
  }

public:
  SDNav(fs::FS &fs)
  {
    _fs = &fs;
    auto root = fs.open("/");
    if (!root)
    {
      log_e("Invalid root");
      exit(1);
    }
    log_d("Creating DirPtr %s", root.name());
    cDir = new DirPtr(root);
  }
  ~SDNav()
  {
    while (switchToParentDir())
      ;
    delete cDir;
  }

  fs::FS getFs()
  {
    return *_fs;
  }

  bool switchToParentDir()
  {
    if (cDir->isRoot())
    {
      log_d("cDir is root, can't switch");
      return false;
    }
    auto parent = cDir->close();
    delete cDir;
    cDir = parent;
    log_d("Switch to parent directory %s", cDir->name());
    return true;
  }

  void restoreStateFromPath(const char *buff)
  {
    restoreDirChain(buff, buff + 1);
    cDir->updatePosition();
  }

  String getNextDirFile()
  {
    switchToParentDir();
    return getNextFile();
  }

  String getNextFile()
  {
    while (true)
    {
      log_d("Getting next file");
      File f = cDir->getNext();
      if (!f)
      {
        log_d("No more files");
        if (switchToParentDir())
        {
          continue;
        }
        else
        {
          break;
        }
      }
      if (f.isDirectory())
      {
        log_d("Set cDir to the %s", f.name());
        cDir = new DirPtr(f, cDir);
        continue;
      }

      if (matchFilter(f.name()))
      {
        log_d("Found file %s", f.name());
        auto result = String(f.path());
        f.close();
        return result;
      }
      log_d("Next");
      f.close();
    }
    return emptyString;
  }

  String getPrevDirFile()
  {
    switchToParentDir();
    return getPrevFile();
  }

  String getPrevFile()
  {
    while (true)
    {
      log_d("Getting prev file");
      File f = cDir->getPrev();
      if (!f)
      {
        log_d("No more files");
        if (switchToParentDir())
        {
          continue;
        }
        else
        {
          break;
        }
      }
      if (f.isDirectory())
      {
        log_d("Set cDir to the %s", f.name());
        cDir = new DirPtr(f, cDir);
        continue;
      }

      if (matchFilter(f.name()))
      {
        log_d("Found file %s", f.name());
        auto result = String(f.path());
        f.close();
        return result;
      }
      log_d("Next");
      f.close();
    }
    return emptyString;
  }

  const bool matchFilter(const char *str)
  {
    log_d("Check match");
    bool res = false;
    auto p = strrchr(str, '.');
    if (p != nullptr)
    {
      char *ext = strdup(p);
      res = strstr(FILE_FILTER, ext) != nullptr;
      free(ext);
      return res;
    }
    return res;
  }

  String getCurrentDirName()
  {
    return cDir->name();
  }
};