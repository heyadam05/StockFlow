#include "../include/Inventory.h"
#include "../include/Utils.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace stockflow {

const std::vector<Product>& Inventory::products() const noexcept { return products_; }
const std::vector<Category>& Inventory::categories() const noexcept { return categories_; }
const std::vector<Transaction>& Inventory::transactions() const noexcept { return transactions_; }

Product& Inventory::addProduct(Product product) {
    if (utils::trim(product.name).empty()) throw std::invalid_argument("Product name is required.");
    if (product.price < 0) throw std::invalid_argument("Price cannot be negative.");
    if (product.quantity < 0) throw std::invalid_argument("Quantity cannot be negative.");
    if (product.minimumQuantity < 0) throw std::invalid_argument("Minimum quantity cannot be negative.");
    if (!product.barcode.empty() && std::any_of(products_.begin(), products_.end(),
        [&](const Product& current) { return current.barcode == product.barcode; }))
        throw std::invalid_argument("Barcode already exists.");
    if (product.id <= 0) product.id = nextProductId();
    if (product.createdAt.empty()) product.createdAt = utils::now();
    product.updatedAt = utils::now();
    products_.push_back(std::move(product));
    return products_.back();
}

bool Inventory::updateProduct(int id, const Product& replacement) {
    Product* product = findProduct(id);
    if (!product || utils::trim(replacement.name).empty() || replacement.price < 0 ||
        replacement.minimumQuantity < 0 ||
        (!replacement.barcode.empty() &&
         std::any_of(products_.begin(), products_.end(), [&](const Product& current) {
             return current.id != id && current.barcode == replacement.barcode;
         }))) return false;
    Product updated = replacement;
    updated.id = id;
    updated.quantity = product->quantity;
    updated.createdAt = product->createdAt;
    updated.updatedAt = utils::now();
    *product = std::move(updated);
    return true;
}

bool Inventory::deleteProduct(int id) {
    auto position = std::find_if(products_.begin(), products_.end(),
                                 [id](const Product& product) { return product.id == id; });
    if (position == products_.end()) return false;
    products_.erase(position);
    return true;
}

Product* Inventory::findProduct(int id) {
    auto position = std::find_if(products_.begin(), products_.end(),
                                 [id](const Product& product) { return product.id == id; });
    return position == products_.end() ? nullptr : &*position;
}

const Product* Inventory::findProduct(int id) const {
    auto position = std::find_if(products_.begin(), products_.end(),
                                 [id](const Product& product) { return product.id == id; });
    return position == products_.end() ? nullptr : &*position;
}

std::vector<const Product*> Inventory::search(const std::string& query) const {
    const std::string needle = utils::lower(query);
    std::vector<const Product*> result;
    for (const Product& product : products_) {
        const std::string haystack = utils::lower(product.name + '\n' + product.description +
            '\n' + product.category + '\n' + product.supplier + '\n' + product.barcode);
        if (haystack.find(needle) != std::string::npos) result.push_back(&product);
    }
    return result;
}

std::vector<const Product*> Inventory::filter(const std::string& field,
                                               const std::string& operation,
                                               const std::string& value) const {
    const std::string normalized = utils::lower(field);
    const std::string needle = utils::lower(value);
    std::vector<const Product*> result;
    for (const Product& product : products_) {
        bool matches = false;
        if (normalized == "category") matches = utils::lower(product.category) == needle;
        else if (normalized == "supplier") matches = utils::lower(product.supplier) == needle;
        else if (normalized == "quantity" || normalized == "price") {
            double expected{};
            if (!utils::parseDouble(value, expected)) continue;
            const double actual = normalized == "quantity" ? product.quantity : product.price;
            matches = operation == "<" ? actual < expected :
                      operation == ">" ? actual > expected :
                      operation == "<=" ? actual <= expected :
                      operation == ">=" ? actual >= expected :
                      std::abs(actual - expected) < 0.000001;
        }
        if (matches) result.push_back(&product);
    }
    return result;
}

bool Inventory::changeStock(int productId, int delta, const std::string& note,
                            std::string& error) {
    Product* product = findProduct(productId);
    if (!product) { error = "Product not found."; return false; }
    if (product->quantity + delta < 0) {
        error = "Quantity cannot be negative.";
        return false;
    }
    product->quantity += delta;
    product->updatedAt = utils::now();
    transactions_.push_back({nextTransactionId(), productId,
        delta > 0 ? TransactionType::In : delta < 0 ? TransactionType::Out :
        TransactionType::Adjustment, std::abs(delta), utils::now(), "local", note});
    return true;
}

bool Inventory::addCategory(const std::string& name, const std::string& description,
                            std::string& error) {
    const std::string clean = utils::trim(name);
    if (clean.empty()) { error = "Category name is required."; return false; }
    if (std::any_of(categories_.begin(), categories_.end(), [&](const Category& category) {
        return utils::lower(category.name) == utils::lower(clean);
    })) { error = "Category already exists."; return false; }
    categories_.push_back({nextCategoryId(), clean, description});
    return true;
}

bool Inventory::renameCategory(const std::string& oldName, const std::string& newName,
                               std::string& error) {
    auto found = std::find_if(categories_.begin(), categories_.end(), [&](const Category& c) {
        return utils::lower(c.name) == utils::lower(oldName);
    });
    if (found == categories_.end()) { error = "Category not found."; return false; }
    if (utils::trim(newName).empty()) { error = "Category name is required."; return false; }
    if (std::any_of(categories_.begin(), categories_.end(), [&](const Category& category) {
        return category.id != found->id &&
               utils::lower(category.name) == utils::lower(utils::trim(newName));
    })) { error = "Category already exists."; return false; }
    const std::string previous = found->name;
    found->name = utils::trim(newName);
    for (Product& product : products_)
        if (utils::lower(product.category) == utils::lower(previous)) product.category = found->name;
    return true;
}

bool Inventory::removeCategory(const std::string& name, std::string& error) {
    if (std::any_of(products_.begin(), products_.end(), [&](const Product& product) {
        return utils::lower(product.category) == utils::lower(name);
    })) { error = "Cannot remove category because it is in use."; return false; }
    auto position = std::find_if(categories_.begin(), categories_.end(), [&](const Category& c) {
        return utils::lower(c.name) == utils::lower(name);
    });
    if (position == categories_.end()) { error = "Category not found."; return false; }
    categories_.erase(position);
    return true;
}

double Inventory::totalValue() const noexcept {
    double total = 0;
    for (const Product& product : products_) total += product.price * product.quantity;
    return total;
}

std::size_t Inventory::lowStockCount() const noexcept {
    return std::count_if(products_.begin(), products_.end(), [](const Product& product) {
        return product.quantity > 0 && product.quantity <= product.minimumQuantity;
    });
}

std::size_t Inventory::outOfStockCount() const noexcept {
    return std::count_if(products_.begin(), products_.end(),
                         [](const Product& product) { return product.quantity == 0; });
}

void Inventory::replaceData(std::vector<Product> products,
                            std::vector<Category> categories,
                            std::vector<Transaction> transactions) {
    products_ = std::move(products);
    categories_ = std::move(categories);
    transactions_ = std::move(transactions);
}

int Inventory::nextProductId() const {
    int next = 101;
    for (const Product& product : products_) next = std::max(next, product.id + 1);
    return next;
}
int Inventory::nextCategoryId() const {
    int next = 1;
    for (const Category& category : categories_) next = std::max(next, category.id + 1);
    return next;
}
int Inventory::nextTransactionId() const {
    int next = 1;
    for (const Transaction& transaction : transactions_) next = std::max(next, transaction.id + 1);
    return next;
}

} 
