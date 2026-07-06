#pragma once

#include "Inventory.h"
#include "Filesystem.h"

#include <string>

namespace stockflow {

class Storage {
public:
    explicit Storage(fs::path dataDirectory);

    bool load(Inventory& inventory, std::string& error) const;
    bool save(const Inventory& inventory, std::string& error) const;
    bool backup(const Inventory& inventory, const fs::path& directory,
                std::string& error) const;
    bool exportCsv(const Inventory& inventory, const fs::path& file,
                   std::string& error) const;
    bool importCsv(Inventory& inventory, const fs::path& file,
                   std::string& error) const;
    void log(const std::string& message) const;
    const fs::path& dataDirectory() const noexcept;

private:
    fs::path dataDirectory_;
};

} 
