#pragma once

#include <string>
#include <vector>

namespace stockflow::utils {

std::string trim(const std::string& value);
std::string lower(std::string value);
std::vector<std::string> splitCommand(const std::string& line);
std::string now();
std::string today();
std::string escapeField(const std::string& value);
std::string unescapeField(const std::string& value);
std::vector<std::string> splitFields(const std::string& line);
bool parseInt(const std::string& value, int& result);
bool parseDouble(const std::string& value, double& result);
std::string formatMoney(double value, const std::string& currency = "EUR");

} 
