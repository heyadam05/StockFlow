#include "../include/Models.h"

#include <stdexcept>

namespace stockflow {

std::string toString(TransactionType type) {
    switch (type) {
    case TransactionType::In: return "IN";
    case TransactionType::Out: return "OUT";
    case TransactionType::Adjustment: return "ADJUSTMENT";
    }
    return "ADJUSTMENT";
}

TransactionType transactionTypeFromString(const std::string& value) {
    if (value == "IN") return TransactionType::In;
    if (value == "OUT") return TransactionType::Out;
    if (value == "ADJUSTMENT") return TransactionType::Adjustment;
    throw std::invalid_argument("Unknown transaction type: " + value);
}

} 
