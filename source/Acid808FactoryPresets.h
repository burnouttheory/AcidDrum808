#pragma once
// Acid Drum 808: install-only-new Factory presets from the admin-staged
// ProgramData folder into the CURRENT USER's redirected Documents folder.
// Never delete or overwrite a personal preset; never touch User/.
#include <filesystem>
#include <mutex>
#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>
#include <system_error>

namespace acid808factory {
inline int installMissing(const std::filesystem::path& presetsRoot,
                          const std::filesystem::path& stagedFactory) {
  namespace fs = std::filesystem;
  std::error_code ec;
  if (!fs::is_directory(stagedFactory, ec) || ec) return 0;
  const fs::path destFactory = presetsRoot / "Factory";
  fs::create_directories(destFactory, ec);
  if (ec) return 0;
  int added=0;
  fs::recursive_directory_iterator it(stagedFactory,fs::directory_options::skip_permission_denied,ec);
  const fs::recursive_directory_iterator end;
  for (; !ec && it!=end; it.increment(ec)) {
    std::error_code local;
    if (it->is_symlink(local) || local || !it->is_regular_file(local) || local) continue;
    auto file=it->path();
    auto ext=file.extension().string();
    std::transform(ext.begin(),ext.end(),ext.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});
    if (ext!=".aciddrum808preset") continue;
    const auto size=fs::file_size(file,local);
    if (local || size==0 || size>2*1024*1024) continue;
    const fs::path relative=file.lexically_relative(stagedFactory);
    if (relative.empty() || relative.is_absolute()) continue;
    bool unsafe=false;
    for (const auto& segment:relative) if(segment=="..") unsafe=true;
    if (unsafe) continue;
    const fs::path target=destFactory / relative;
    fs::create_directories(target.parent_path(),local);
    if(local)continue;
    // Do not follow existing destination symlinks or overwrite files.
    if (fs::exists(target,local) || local) continue;
    if(fs::copy_file(file,target,fs::copy_options::none,local) && !local) ++added;
  }
  return added;
}
inline int sync(const std::filesystem::path& presetsRoot) {
  static std::once_flag once;
  static int installed=0;
  std::call_once(once,[&](){
    const char* programData=std::getenv("PROGRAMDATA");
    if(programData && *programData) {
      installed=installMissing(presetsRoot,
          std::filesystem::path(programData)/"AcidDrum808"/"FactoryContent"/"presets"/"Factory");
    }
  });
  return installed;
}
}
