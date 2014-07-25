#ifndef _CHUNKCONFIGURATION_H
#define _CHUNKCONFIGURATION_H

#include "common.h"

class ChunkConf {
public:
  ~ChunkConf();
  static ChunkConf *GetChunkConf();

  bool Initialize(const std::string &path);
  int32_t GetInt(const std::string &key, int32_t def);
  std::string GetString(const std::string &key, const std::string &def);
  bool GetBool(const std::string &key, bool def);

private:
  ChunkConf();

  static ChunkConf config_;
  typedef std::tr1::unordered_map<std::string, std::string> ConfigMap;
  ConfigMap config_map_;
};



#endif // _P2PCONFIGURATION_H
