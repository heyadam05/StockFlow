#include "../include/Storage.h"
#include "../include/Utils.h"

#include <fstream>
#include <sstream>

namespace stockflow {
namespace {

bool atomicWrite(const fs::path& path, const std::string& content,
                 std::string& error) {
    try {
        if (!path.parent_path().empty()) fs::create_directories(path.parent_path());
        const auto temporary = path.string() + ".tmp";
        {
            std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
            if (!output) { error = "Cannot write " + temporary; return false; }
            output << content;
            if (!output) { error = "Failed while writing " + temporary; return false; }
        }
        std::error_code ignored;
        fs::remove(path, ignored);
        fs::rename(temporary, path);
        return true;
    } catch (const std::exception& exception) {
        error = exception.what();
        return false;
    }
}

std::string csvField(const std::string& value) {
    std::string escaped = value;
    std::size_t position = 0;
    while ((position = escaped.find('"', position)) != std::string::npos) {
        escaped.insert(position, 1, '"');
        position += 2;
    }
    return '"' + escaped + '"';
}

std::vector<std::string> parseCsv(const std::string& line) {
    std::vector<std::string> fields;
    std::string field;
    bool quoted = false;
    for (std::size_t index = 0; index < line.size(); ++index) {
        const char c = line[index];
        if (c == '"') {
            if (quoted && index + 1 < line.size() && line[index + 1] == '"') {
                field += '"';
                ++index;
            } else quoted = !quoted;
        } else if (c == ',' && !quoted) {
            fields.push_back(field);
            field.clear();
        } else field += c;
    }
    fields.push_back(field);
    return fields;
}

bool csvRecordComplete(const std::string& record) {
    bool quoted = false;
    for (std::size_t index = 0; index < record.size(); ++index) {
        if (record[index] != '"') continue;
        if (quoted && index + 1 < record.size() && record[index + 1] == '"') {
            ++index;
        } else {
            quoted = !quoted;
        }
    }
    return !quoted;
}

} 

Storage::Storage(fs::path dataDirectory)
    : dataDirectory_(std::move(dataDirectory)) {}

bool Storage::load(Inventory& inventory, std::string& error) const {
    try {
        fs::create_directories(dataDirectory_);
        std::vector<Product> products;
        std::vector<Category> categories;
        std::vector<Transaction> transactions;

        std::ifstream productFile(dataDirectory_ / "products.db");
        std::string line;
        while (std::getline(productFile, line)) {
            if (line.empty()) continue;
            const auto f = utils::splitFields(line);
            if (f.size() != 12) { error = "Invalid products.db record."; return false; }
            Product p;
            if (!utils::parseInt(f[0], p.id) || !utils::parseDouble(f[6], p.price) ||
                !utils::parseInt(f[7], p.quantity) ||
                !utils::parseInt(f[8], p.minimumQuantity)) {
                error = "Invalid number in products.db."; return false;
            }
            p.name=f[1]; p.description=f[2]; p.category=f[3]; p.supplier=f[4];
            p.barcode=f[5]; p.location=f[9]; p.createdAt=f[10]; p.updatedAt=f[11];
            products.push_back(std::move(p));
        }

        std::ifstream categoryFile(dataDirectory_ / "categories.db");
        while (std::getline(categoryFile, line)) {
            if (line.empty()) continue;
            const auto f = utils::splitFields(line);
            Category c;
            if (f.size() != 3 || !utils::parseInt(f[0], c.id)) {
                error = "Invalid categories.db record."; return false;
            }
            c.name=f[1]; c.description=f[2];
            categories.push_back(std::move(c));
        }

        std::ifstream transactionFile(dataDirectory_ / "transactions.db");
        while (std::getline(transactionFile, line)) {
            if (line.empty()) continue;
            const auto f = utils::splitFields(line);
            Transaction t;
            if (f.size() != 8 || !utils::parseInt(f[0], t.id) ||
                !utils::parseInt(f[1], t.productId) || !utils::parseInt(f[3], t.amount)) {
                error = "Invalid transactions.db record."; return false;
            }
            t.type=transactionTypeFromString(f[2]); t.date=f[4]; t.user=f[5];
            t.note=f[6] + f[7]; 
            transactions.push_back(std::move(t));
        }

        if (categories.empty()) {
            int id = 1;
            for (const char* name : {"Electronics","Food","Books","Furniture",
                                     "Office","Gaming","Accessories","Other"})
                categories.push_back({id++, name, "Default category"});
        }
        inventory.replaceData(std::move(products), std::move(categories),
                              std::move(transactions));
        return true;
    } catch (const std::exception& exception) {
        error = exception.what();
        return false;
    }
}

bool Storage::save(const Inventory& inventory, std::string& error) const {
    std::ostringstream products;
    for (const Product& p : inventory.products())
        products << p.id << '\t' << utils::escapeField(p.name) << '\t'
                 << utils::escapeField(p.description) << '\t'
                 << utils::escapeField(p.category) << '\t'
                 << utils::escapeField(p.supplier) << '\t'
                 << utils::escapeField(p.barcode) << '\t' << p.price << '\t'
                 << p.quantity << '\t' << p.minimumQuantity << '\t'
                 << utils::escapeField(p.location) << '\t'
                 << utils::escapeField(p.createdAt) << '\t'
                 << utils::escapeField(p.updatedAt) << '\n';
    std::ostringstream categories;
    for (const Category& c : inventory.categories())
        categories << c.id << '\t' << utils::escapeField(c.name) << '\t'
                   << utils::escapeField(c.description) << '\n';
    std::ostringstream transactions;
    for (const Transaction& t : inventory.transactions())
        transactions << t.id << '\t' << t.productId << '\t' << toString(t.type)
                     << '\t' << t.amount << '\t' << utils::escapeField(t.date)
                     << '\t' << utils::escapeField(t.user) << '\t'
                     << utils::escapeField(t.note) << "\t\n";
    return atomicWrite(dataDirectory_ / "products.db", products.str(), error) &&
           atomicWrite(dataDirectory_ / "categories.db", categories.str(), error) &&
           atomicWrite(dataDirectory_ / "transactions.db", transactions.str(), error);
}

bool Storage::backup(const Inventory& inventory, const fs::path& directory,
                     std::string& error) const {
    Storage target(directory / ("backup_" + utils::today()));
    return target.save(inventory, error);
}

bool Storage::exportCsv(const Inventory& inventory, const fs::path& file,
                        std::string& error) const {
    std::ostringstream output;
    output << "id,name,description,category,supplier,barcode,price,quantity,minimum,location\n";
    for (const Product& p : inventory.products())
        output << p.id << ',' << csvField(p.name) << ',' << csvField(p.description)
               << ',' << csvField(p.category) << ',' << csvField(p.supplier) << ','
               << csvField(p.barcode) << ',' << p.price << ',' << p.quantity << ','
               << p.minimumQuantity << ',' << csvField(p.location) << '\n';
    return atomicWrite(file, output.str(), error);
}

bool Storage::importCsv(Inventory& inventory, const fs::path& file,
                        std::string& error) const {
    std::ifstream input(file);
    if (!input) { error = "Cannot open CSV file."; return false; }
    std::string line;
    std::getline(input, line);
    int lineNumber = 1;
    Inventory candidate = inventory;
    while (std::getline(input, line)) {
        ++lineNumber;
        if (line.empty()) continue;
        const int recordStart = lineNumber;
        while (!csvRecordComplete(line)) {
            std::string continuation;
            if (!std::getline(input, continuation)) {
                error = "Unterminated CSV field at line " + std::to_string(recordStart);
                return false;
            }
            ++lineNumber;
            line += '\n' + continuation;
        }
        auto f = parseCsv(line);
        if (f.size() != 10) { error = "Invalid CSV at line " + std::to_string(recordStart); return false; }
        Product p;
        if (!utils::parseDouble(f[6], p.price) || !utils::parseInt(f[7], p.quantity) ||
            !utils::parseInt(f[8], p.minimumQuantity)) {
            error = "Invalid CSV number at line " + std::to_string(recordStart); return false;
        }
        p.name=f[1]; p.description=f[2]; p.category=f[3]; p.supplier=f[4];
        p.barcode=f[5]; p.location=f[9]; p.id=0;
        try { candidate.addProduct(std::move(p)); }
        catch (const std::exception& exception) {
            error = "CSV line " + std::to_string(recordStart) + ": " + exception.what();
            return false;
        }
    }
    inventory = std::move(candidate);
    return true;
}

void Storage::log(const std::string& message) const {
    try {
        fs::create_directories(dataDirectory_);
        std::ofstream output(dataDirectory_ / "logs.txt", std::ios::app);
        output << '[' << utils::now() << "] " << message << '\n';
    } catch (...) {}
}

const fs::path& Storage::dataDirectory() const noexcept {
    return dataDirectory_;
}

} 
