#pragma once

#include "Inventory.h"
#include "Models.h"
#include "Storage.h"

#include <istream>
#include <ostream>
#include <string>
#include <vector>

namespace stockflow {

class Application {
public:
    explicit Application(fs::path baseDirectory);
    int run(std::istream& input, std::ostream& output);

private:
    bool execute(const std::string& line, std::istream& input, std::ostream& output);
    void printHelp(std::ostream& output) const;
    void printProducts(const std::vector<const Product*>& products,
                       std::ostream& output) const;
    void addProduct(std::istream& input, std::ostream& output);
    void editProduct(int id, std::istream& input, std::ostream& output);
    void showStats(std::ostream& output) const;
    void generateReport(std::ostream& output);
    bool persist(const std::string& action, std::ostream& output);

    fs::path baseDirectory_;
    Inventory inventory_;
    Storage storage_;
    bool running_{true};
};

} 
