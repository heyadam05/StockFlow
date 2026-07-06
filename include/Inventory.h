#pragma once

#include "Models.h"

#include <string>
#include <vector>

namespace stockflow {

class Inventory {
public:
    const std::vector<Product>& products() const noexcept;
    const std::vector<Category>& categories() const noexcept;
    const std::vector<Transaction>& transactions() const noexcept;

    Product& addProduct(Product product);
    bool updateProduct(int id, const Product& replacement);
    bool deleteProduct(int id);
    Product* findProduct(int id);
    const Product* findProduct(int id) const;
    std::vector<const Product*> search(const std::string& query) const;
    std::vector<const Product*> filter(const std::string& field,
                                      const std::string& operation,
                                      const std::string& value) const;
    bool changeStock(int productId, int delta, const std::string& note,
                     std::string& error);

    bool addCategory(const std::string& name, const std::string& description,
                     std::string& error);
    bool renameCategory(const std::string& oldName, const std::string& newName,
                        std::string& error);
    bool removeCategory(const std::string& name, std::string& error);

    double totalValue() const noexcept;
    std::size_t lowStockCount() const noexcept;
    std::size_t outOfStockCount() const noexcept;

    void replaceData(std::vector<Product> products,
                     std::vector<Category> categories,
                     std::vector<Transaction> transactions);

private:
    int nextProductId() const;
    int nextCategoryId() const;
    int nextTransactionId() const;

    std::vector<Product> products_;
    std::vector<Category> categories_;
    std::vector<Transaction> transactions_;
};

} 
