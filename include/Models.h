#pragma once

#include <string>

namespace stockflow {

struct Product {
    int id{};
    std::string name;
    std::string description;
    std::string category;
    std::string supplier;
    std::string barcode;
    double price{};
    int quantity{};
    int minimumQuantity{};
    std::string location;
    std::string createdAt;
    std::string updatedAt;
};

enum class TransactionType { In, Out, Adjustment };

struct Transaction {
    int id{};
    int productId{};
    TransactionType type{TransactionType::Adjustment};
    int amount{};
    std::string date;
    std::string user;
    std::string note;
};

struct Category {
    int id{};
    std::string name;
    std::string description;
};

std::string toString(TransactionType type);
TransactionType transactionTypeFromString(const std::string& value);

} 
