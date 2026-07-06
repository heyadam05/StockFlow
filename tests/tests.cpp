#include "../include/Inventory.h"
#include "../include/Filesystem.h"
#include "../include/Storage.h"
#include "../include/Utils.h"

#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace stockflow;

namespace {
int passed = 0;

void expect(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
    ++passed;
}

Product sample(std::string name = "Keyboard", int quantity = 10) {
    Product p;
    p.name = std::move(name);
    p.description = "Mechanical";
    p.category = "Accessories";
    p.supplier = "Test Supplier";
    p.barcode = "BAR-" + p.name;
    p.price = 49.99;
    p.quantity = quantity;
    p.minimumQuantity = 3;
    p.location = "Shelf A";
    return p;
}

void testInventoryCrud() {
    Inventory inventory;
    Product& created = inventory.addProduct(sample());
    expect(created.id == 101, "First generated ID must be 101");
    expect(inventory.findProduct(101) != nullptr, "Added product must be found");
    expect(std::abs(inventory.totalValue() - 499.9) < 0.001, "Total value is incorrect");

    Product updated = created;
    updated.name = "Gaming Keyboard";
    updated.price = 79.5;
    expect(inventory.updateProduct(101, updated), "Product update failed");
    expect(inventory.findProduct(101)->name == "Gaming Keyboard", "Update was not applied");
    Product second = sample("Mouse", 3);
    second.barcode = "MOUSE-2";
    inventory.addProduct(second);
    Product duplicate = *inventory.findProduct(102);
    duplicate.barcode = inventory.findProduct(101)->barcode;
    expect(!inventory.updateProduct(102, duplicate), "Duplicate barcode edit was accepted");
    expect(inventory.deleteProduct(101), "Product delete failed");
    expect(inventory.products().size() == 1, "Wrong product count after delete");
}

void testValidationAndStock() {
    Inventory inventory;
    inventory.addProduct(sample("Mouse", 5));
    std::string error;
    expect(inventory.changeStock(101, 4, "delivery", error), "Stock receipt failed");
    expect(inventory.findProduct(101)->quantity == 9, "Receipt quantity is incorrect");
    expect(inventory.transactions().back().type == TransactionType::In, "IN transaction missing");
    expect(inventory.changeStock(101, -3, "sale", error), "Stock issue failed");
    expect(inventory.transactions().back().type == TransactionType::Out, "OUT transaction missing");
    expect(!inventory.changeStock(101, -100, "", error), "Negative stock was accepted");
    expect(inventory.findProduct(101)->quantity == 6, "Rejected operation changed stock");

    bool rejected = false;
    try { inventory.addProduct(sample("Mouse", -1)); }
    catch (const std::invalid_argument&) { rejected = true; }
    expect(rejected, "Negative initial stock was accepted");
}

void testSearchFilterCategoriesStats() {
    Inventory inventory;
    std::string error;
    expect(inventory.addCategory("Accessories", "Peripherals", error), "Category add failed");
    expect(!inventory.addCategory("accessories", "", error), "Duplicate category was accepted");
    inventory.addProduct(sample("Keyboard", 2));
    Product monitor = sample("Monitor", 0);
    monitor.category = "Electronics";
    monitor.supplier = "Displays Ltd";
    monitor.price = 200;
    monitor.barcode = "MON-1";
    inventory.addProduct(monitor);
    expect(inventory.search("supplier").size() == 1, "Search failed");
    expect(inventory.filter("price", ">", "100").size() == 1, "Price filter failed");
    expect(inventory.filter("category", "=", "electronics").size() == 1, "Category filter failed");
    expect(inventory.lowStockCount() == 1, "Low stock count is incorrect");
    expect(inventory.outOfStockCount() == 1, "Out-of-stock count is incorrect");
    expect(!inventory.removeCategory("Accessories", error), "In-use category was removed");
    expect(inventory.renameCategory("Accessories", "Peripherals", error), "Category rename failed");
    expect(inventory.findProduct(101)->category == "Peripherals", "Rename did not update product");
}

void testPersistenceAndCsv() {
    const auto root = fs::temp_directory_path() / "stockflow_tests";
    std::error_code ignored;
    fs::remove_all(root, ignored);
    Storage storage(root / "data");
    Inventory original;
    std::string error;
    expect(original.addCategory("Accessories", "With\ttab", error), "Test category failed");
    Product p = sample("Quoted \"Keyboard\"", 7);
    p.description = "Line one\nLine two";
    original.addProduct(p);
    expect(original.changeStock(101, -2, "customer order", error), "Stock setup failed");
    expect(storage.save(original, error), "Save failed: " + error);

    Inventory loaded;
    expect(storage.load(loaded, error), "Load failed: " + error);
    expect(loaded.products().size() == 1, "Loaded product count is incorrect");
    expect(loaded.products()[0].description == p.description, "Escaped data was corrupted");
    expect(loaded.transactions().size() == 1, "Transactions were not persisted");
    expect(storage.exportCsv(loaded, root / "products.csv", error), "CSV export failed");

    Inventory imported;
    expect(storage.importCsv(imported, root / "products.csv", error), "CSV import failed: " + error);
    expect(imported.products().size() == 1, "CSV product count is incorrect");
    expect(imported.products()[0].name == p.name, "CSV quotes were corrupted");
    expect(storage.backup(loaded, root / "backups", error), "Backup failed");
    expect(fs::exists(root / "backups" / ("backup_" + utils::today()) /
                                   "products.db"), "Backup file is missing");
    fs::remove_all(root, ignored);
}
} 

int main() {
    try {
        testInventoryCrud();
        testValidationAndStock();
        testSearchFilterCategoriesStats();
        testPersistenceAndCsv();
        std::cout << "All tests passed (" << passed << " assertions).\n";
        return 0;
    } catch (const std::exception& exception) {
        std::cerr << "TEST FAILURE: " << exception.what() << '\n';
        return 1;
    }
}
