#include "../include/Application.h"
#include "../include/Utils.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <numeric>
#include <stdexcept>
#include <utility>

namespace stockflow {
namespace {

std::string ask(std::istream& input, std::ostream& output, const std::string& prompt,
                const std::string& current = {}) {
    output << prompt;
    if (!current.empty()) output << " [" << current << ']';
    output << ": ";
    std::string value;
    std::getline(input, value);
    value = utils::trim(value);
    return value.empty() ? current : value;
}

std::vector<const Product*> allProducts(const Inventory& inventory) {
    std::vector<const Product*> products;
    for (const Product& product : inventory.products()) products.push_back(&product);
    return products;
}

} 

Application::Application(fs::path baseDirectory)
    : baseDirectory_(std::move(baseDirectory)), storage_(baseDirectory_ / "data") {}

int Application::run(std::istream& input, std::ostream& output) {
    std::string error;
    if (!storage_.load(inventory_, error)) {
        output << "Could not load database: " << error << '\n';
        return 1;
    }
    if (!storage_.save(inventory_, error))
        output << "Warning: could not initialize database: " << error << '\n';

    output << "==========================================\n"
           << "StockFlow\nInventory Management System\nVersion 1.0\n"
           << "==========================================\n\n"
           << "Database loaded successfully.\n\nProducts: " << inventory_.products().size()
           << "\nCategories: " << inventory_.categories().size() << "\n";
    const auto alerts = inventory_.lowStockCount() + inventory_.outOfStockCount();
    if (alerts) output << "\nWarning: " << alerts << " product(s) require restocking.\n";
    output << "\nType \"help\" to see available commands.\n\n";

    std::string line;
    while (running_ && output << "> " && std::getline(input, line))
        execute(line, input, output);
    return 0;
}

bool Application::execute(const std::string& line, std::istream& input,
                          std::ostream& output) {
    const auto args = utils::splitCommand(line);
    if (args.empty()) return true;
    const std::string command = utils::lower(args[0]);
    int id{};

    if (command == "help") printHelp(output);
    else if (command == "add") addProduct(input, output);
    else if (command == "list") printProducts(allProducts(inventory_), output);
    else if (command == "show") {
        if (args.size() != 2 || !utils::parseInt(args[1], id)) {
            output << "Usage: show <product-id>\n";
        } else if (const Product* p = inventory_.findProduct(id)) {
            output << "ID: " << p->id << "\nName: " << p->name
                   << "\nDescription: " << p->description << "\nCategory: " << p->category
                   << "\nSupplier: " << p->supplier << "\nBarcode: " << p->barcode
                   << "\nPrice: " << utils::formatMoney(p->price)
                   << "\nQuantity: " << p->quantity << "\nMinimum: " << p->minimumQuantity
                   << "\nLocation: " << p->location << "\nCreated: " << p->createdAt
                   << "\nUpdated: " << p->updatedAt << '\n';
        } else output << "Product not found.\n";
    } else if (command == "edit") {
        if (args.size() != 2 || !utils::parseInt(args[1], id))
            output << "Usage: edit <product-id>\n";
        else editProduct(id, input, output);
    } else if (command == "delete") {
        if (args.size() != 2 || !utils::parseInt(args[1], id))
            output << "Usage: delete <product-id>\n";
        else if (!inventory_.findProduct(id)) output << "Product not found.\n";
        else {
            const std::string confirmation = ask(input, output, "Delete product? (Y/N)");
            if (utils::lower(confirmation) == "y") {
                inventory_.deleteProduct(id);
                persist("Deleted product " + std::to_string(id), output);
                output << "Product deleted.\n";
            } else output << "Cancelled.\n";
        }
    } else if (command == "stock") {
        int delta{};
        if (args.size() < 3 || !utils::parseInt(args[1], id) ||
            !utils::parseInt(args[2], delta) || delta == 0) {
            output << "Usage: stock <product-id> <+/-amount> [note]\n";
        } else {
            std::string note;
            for (std::size_t i = 3; i < args.size(); ++i) note += (note.empty() ? "" : " ") + args[i];
            std::string error;
            if (!inventory_.changeStock(id, delta, note, error)) output << error << '\n';
            else {
                persist("Updated stock for product " + std::to_string(id), output);
                output << "Quantity updated.\n";
            }
        }
    } else if (command == "search") {
        if (args.size() < 2) output << "Usage: search <text>\n";
        else {
            std::string query;
            for (std::size_t i = 1; i < args.size(); ++i) query += (query.empty() ? "" : " ") + args[i];
            printProducts(inventory_.search(query), output);
        }
    } else if (command == "filter") {
        if (args.size() != 3 && args.size() != 4)
            output << "Usage: filter <category|supplier> <value> OR "
                      "filter <quantity|price> <<|>|=|<=|=> <value>\n";
        else {
            const std::string op = args.size() == 4 ? args[2] : "=";
            const std::string value = args.size() == 4 ? args[3] : args[2];
            printProducts(inventory_.filter(args[1], op, value), output);
        }
    } else if (command == "categories") {
        if (args.size() == 1 || utils::lower(args[1]) == "list") {
            for (const Category& category : inventory_.categories())
                output << category.id << " | " << category.name << " | "
                       << category.description << '\n';
        } else if (utils::lower(args[1]) == "add" && args.size() >= 3) {
            std::string error, description;
            for (std::size_t i=3; i<args.size(); ++i) description += (description.empty()?"":" ")+args[i];
            if (!inventory_.addCategory(args[2], description, error)) output << error << '\n';
            else { persist("Added category " + args[2], output); output << "Category added.\n"; }
        } else if (utils::lower(args[1]) == "remove" && args.size() == 3) {
            std::string error;
            if (!inventory_.removeCategory(args[2], error)) output << error << '\n';
            else { persist("Removed category " + args[2], output); output << "Category removed.\n"; }
        } else if (utils::lower(args[1]) == "rename" && args.size() == 4) {
            std::string error;
            if (!inventory_.renameCategory(args[2], args[3], error)) output << error << '\n';
            else { persist("Renamed category " + args[2], output); output << "Category renamed.\n"; }
        } else output << "Usage: categories [list|add <name> [description]|remove <name>|rename <old> <new>]\n";
    } else if (command == "transactions") {
        output << "ID | Date | Product | Type | Amount | Note\n";
        for (auto it = inventory_.transactions().rbegin(); it != inventory_.transactions().rend(); ++it)
            output << it->id << " | " << it->date << " | " << it->productId << " | "
                   << toString(it->type) << " | " << it->amount << " | " << it->note << '\n';
    } else if (command == "stats") showStats(output);
    else if (command == "report") generateReport(output);
    else if (command == "export") {
        const auto path = args.size() > 1 ? fs::path(args[1])
                                          : baseDirectory_ / "exports" / "products.csv";
        std::string error;
        if (storage_.exportCsv(inventory_, path, error)) {
            storage_.log("Exported CSV to " + path.string());
            output << "Exported to " << path.string() << '\n';
        } else output << "Export failed: " << error << '\n';
    } else if (command == "import") {
        if (args.size() != 2) output << "Usage: import <csv-file>\n";
        else {
            std::string error;
            if (!storage_.importCsv(inventory_, args[1], error)) output << "Import failed: " << error << '\n';
            else { persist("Imported CSV from " + args[1], output); output << "Import complete.\n"; }
        }
    } else if (command == "clear") {
        output << "\x1B[2J\x1B[H";
    } else if (command == "exit" || command == "quit") {
        running_ = false;
        output << "Goodbye.\n";
    } else output << "Unknown command. Type \"help\" for help.\n";
    return true;
}

void Application::printHelp(std::ostream& output) const {
    output << "Commands:\n"
           << "  add                              Add a product interactively\n"
           << "  list                             List products\n"
           << "  show <id>                        Show product details\n"
           << "  edit <id>                        Edit a product\n"
           << "  delete <id>                      Delete a product\n"
           << "  stock <id> <+/-amount> [note]    Receive or issue stock\n"
           << "  search <text>                    Search products\n"
           << "  filter <field> [operator] <value> Filter products\n"
           << "  categories [operation]           Manage categories\n"
           << "  transactions                     Show movement history\n"
           << "  stats                            Show inventory statistics\n"
           << "  report                           Generate a text report\n"
           << "  export [file.csv]                Export products to CSV\n"
           << "  import <file.csv>                Import products from CSV\n"
           << "  clear                            Clear the terminal\n"
           << "  exit                             Save and exit\n"
           << "Tip: quote values containing spaces, e.g. search \"MX Master\".\n";
}

void Application::printProducts(const std::vector<const Product*>& products,
                                std::ostream& output) const {
    if (products.empty()) { output << "No products found.\n"; return; }
    output << std::left << std::setw(6) << "ID" << std::setw(28) << "Name"
           << std::right << std::setw(8) << "Qty" << std::setw(14) << "Price"
           << "  Category\n" << std::string(72, '-') << '\n';
    for (const Product* p : products) {
        const char* status = p->quantity == 0 ? "[OUT] " :
                             p->quantity <= p->minimumQuantity ? "[LOW] " : "";
        output << std::left << std::setw(6) << p->id
               << std::setw(28) << (std::string(status) + p->name).substr(0, 27)
               << std::right << std::setw(8) << p->quantity
               << std::setw(14) << utils::formatMoney(p->price)
               << "  " << p->category << '\n';
    }
}

void Application::addProduct(std::istream& input, std::ostream& output) {
    Product product;
    product.name = ask(input, output, "Product name");
    product.description = ask(input, output, "Description");
    product.category = ask(input, output, "Category", "Other");
    product.supplier = ask(input, output, "Supplier");
    product.barcode = ask(input, output, "Barcode");
    const std::string price = ask(input, output, "Price", "0");
    const std::string quantity = ask(input, output, "Quantity", "0");
    const std::string minimum = ask(input, output, "Minimum quantity", "0");
    product.location = ask(input, output, "Location");
    if (!utils::parseDouble(price, product.price) || product.price < 0) {
        output << "Invalid price.\n"; return;
    }
    if (!utils::parseInt(quantity, product.quantity) || product.quantity < 0 ||
        !utils::parseInt(minimum, product.minimumQuantity) || product.minimumQuantity < 0) {
        output << "Invalid quantity.\n"; return;
    }
    try {
        const int id = inventory_.addProduct(std::move(product)).id;
        persist("Added product " + std::to_string(id), output);
        output << "Product added successfully. ID: " << id << '\n';
    } catch (const std::exception& exception) { output << exception.what() << '\n'; }
}

void Application::editProduct(int id, std::istream& input, std::ostream& output) {
    const Product* existing = inventory_.findProduct(id);
    if (!existing) { output << "Product not found.\n"; return; }
    Product updated = *existing;
    updated.name=ask(input,output,"Name",updated.name);
    updated.description=ask(input,output,"Description",updated.description);
    updated.category=ask(input,output,"Category",updated.category);
    updated.supplier=ask(input,output,"Supplier",updated.supplier);
    updated.barcode=ask(input,output,"Barcode",updated.barcode);
    const std::string price=ask(input,output,"Price",std::to_string(updated.price));
    const std::string minimum=ask(input,output,"Minimum quantity",std::to_string(updated.minimumQuantity));
    updated.location=ask(input,output,"Location",updated.location);
    if (!utils::parseDouble(price, updated.price) || updated.price < 0 ||
        !utils::parseInt(minimum, updated.minimumQuantity) || updated.minimumQuantity < 0) {
        output << "Invalid value.\n"; return;
    }
    if (!inventory_.updateProduct(id, updated)) output << "Update failed.\n";
    else { persist("Edited product " + std::to_string(id), output); output << "Product updated.\n"; }
}

void Application::showStats(std::ostream& output) const {
    double average = inventory_.products().empty() ? 0 :
        std::accumulate(inventory_.products().begin(), inventory_.products().end(), 0.0,
            [](double total, const Product& p) { return total + p.price; }) /
        inventory_.products().size();
    output << "Products: " << inventory_.products().size()
           << "\nCategories: " << inventory_.categories().size()
           << "\nTransactions: " << inventory_.transactions().size()
           << "\nTotal stock value: " << utils::formatMoney(inventory_.totalValue())
           << "\nAverage price: " << utils::formatMoney(average)
           << "\nLow stock: " << inventory_.lowStockCount()
           << "\nOut of stock: " << inventory_.outOfStockCount() << '\n';
}

void Application::generateReport(std::ostream& output) {
    try {
        const auto directory = baseDirectory_ / "reports";
        fs::create_directories(directory);
        std::string date = utils::today();
        std::replace(date.begin(), date.end(), '-', '_');
        const auto path = directory / ("report_" + date + ".txt");
        std::ofstream report(path);
        report << "StockFlow Inventory Report\nGenerated: " << utils::now()
               << "\n\nProducts: " << inventory_.products().size()
               << "\nCategories: " << inventory_.categories().size()
               << "\nTransactions: " << inventory_.transactions().size()
               << "\nTotal value: " << utils::formatMoney(inventory_.totalValue())
               << "\nLow stock: " << inventory_.lowStockCount()
               << "\nOut of stock: " << inventory_.outOfStockCount() << "\n\n";
        for (const Product& p : inventory_.products())
            if (p.quantity <= p.minimumQuantity)
                report << p.id << " | " << p.name << " | " << p.quantity
                       << " / minimum " << p.minimumQuantity << '\n';
        if (!report) throw std::runtime_error("Could not write report.");
        storage_.log("Generated report " + path.string());
        output << "Report created: " << path.string() << '\n';
    } catch (const std::exception& exception) {
        output << "Report failed: " << exception.what() << '\n';
    }
}

bool Application::persist(const std::string& action, std::ostream& output) {
    std::string error;
    if (!storage_.save(inventory_, error)) {
        output << "Warning: data could not be saved: " << error << '\n';
        return false;
    }
    storage_.log(action);
    if (!storage_.backup(inventory_, baseDirectory_ / "backups", error))
        output << "Warning: backup failed: " << error << '\n';
    return true;
}

} 
