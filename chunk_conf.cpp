#include "p2p/p2p.h"
#include "chunk_conf.h"

ChunkConf ChunkConf::config_;

ChunkConf::ChunkConf() {
  
}


ChunkConf::~ChunkConf() {
  
}

ChunkConf *ChunkConf::GetChunkConf() {
  return &config_;
}

bool ChunkConf::Initialize(const std::string &path) {
  File file;
  if (!file.Initialize(path)) {
    WARN("Open file %s failed errno: %d", STR(path), errno);
    return false;
  }
  
  char buffer[4096];
  uint64_t maxSize = 4096;
  while(file.ReadLine((uint8_t*)buffer, maxSize)) {
    std::string str = buffer;
    trim(str);
    if (str.empty() || '#' == str[0])
      continue;

    std::vector<std::string> res;
    split(str, "#", res);

    str = res[0];
    trim(str);
    if ( str.empty() )
      continue;
    res.clear();

    split(str, "=", res);
    if ( 2 != res.size() ) {
      WARN("valid format %s", STR(str));
      continue;
    }

    config_map_[res[0]] = res[1];
    maxSize = 4096;
  }
  file.Close();
  return true;
}

int32_t ChunkConf::GetInt(const std::string &key, int32_t def) {
  ConfigMap::iterator i = config_map_.find(key);
  if ( config_map_.end() == i ) {
    return def;
  }

  return atoi(STR(MAP_VAL(i)));
}

std::string ChunkConf::GetString(const std::string &key, const std::string &def) {
  ConfigMap::iterator i = config_map_.find(key);
  if ( config_map_.end() == i ) {
    return def;
  }

  return MAP_VAL(i);
}

bool ChunkConf::GetBool(const std::string &key, bool def) {
  ConfigMap::iterator i = config_map_.find(key);
  if ( config_map_.end() == i ) {
    return def;
  }

  if ( "true" == lowerCase(MAP_VAL(i)) )
    return true;
  return false;
}

