#pragma once
#include"../Tool/nlohmann/json.hpp"
#include<string>
#include<fstream>
#include<stdexcept>
#include<filesystem>
#include<vector>
#include<optional>

using json = nlohmann::json;

class ConfigReader
{
private:
    nlohmann::json m_json;

    std::string readFile(const std::string& filePath) {
        std::ifstream file(filePath, std::ios::in | std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("配置文件打开失败：" + filePath);
        }
        std::string content((std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>());
        file.close();
        return content;
    }

public:
    explicit ConfigReader(const std::string& configFilePath) {
        try {
            std::string jsonContent = readFile(configFilePath);
            m_json = json::parse(jsonContent);
        }
        catch (const json::parse_error& e) {
            throw std::runtime_error("JSON解析失败：" + std::string(e.what()));
        }
        catch (const std::exception& e) {
            throw std::runtime_error("配置文件读取失败：" + std::string(e.what()));
        }
    }

    // 读取字符串（支持嵌套key，如 {"login", "brokerID"}）
    std::string getString(const std::vector<std::string>& keys, const std::string& defaultValue = "") {
        json current = m_json;
        for (const auto& key : keys) {
            if (!current.contains(key)) return defaultValue;
            current = current[key];
        }
        return current.is_string() ? current.get<std::string>() : defaultValue;
    }

   
    std::string getString(const std::string& key, const std::string& defaultValue = "") {
        return getString({ key }, defaultValue);
    }

    // 读取布尔值
    bool getBool(const std::vector<std::string>& keys, bool defaultValue = false) {
        json current = m_json;
        for (const auto& key : keys) {
            if (!current.contains(key)) return defaultValue;
            current = current[key];
        }
        return current.is_boolean() ? current.get<bool>() : defaultValue;
    }

    // 读取整数
    int getInt(const std::vector<std::string>& keys, int defaultValue = 0) {
        json current = m_json;
        for (const auto& key : keys) {
            if (!current.contains(key)) return defaultValue;
            current = current[key];
        }
        return current.is_number_integer() ? current.get<int>() : defaultValue;
    }

    // 读取浮点数
    double getDouble(const std::vector<std::string>& keys, double defaultValue = 0.0) {
        json current = m_json;
        for (const auto& key : keys) {
            if (!current.contains(key)) return defaultValue;
            current = current[key];
        }
        return current.is_number() ? current.get<double>() : defaultValue;
    }

    //读取字符串数组（订阅合约列表用）
    std::vector<std::string> getStringArray(const std::vector<std::string>& keys) {
        std::vector<std::string> result;
        json current = m_json;
        for (const auto& key : keys) {
            if (!current.contains(key) || !current[key].is_array()) {
                return result;
            }
            current = current[key];
        }
        for (const auto& item : current) {
            if (item.is_string()) result.push_back(item.get<std::string>());
        }
        return result;
    }

    //获取单个合约的专属校验配置
    std::optional<nlohmann::json> getContractValidationConfig(const std::string& contractName) const {
        std::vector<std::string> keys = { "validation", "contract_specific", contractName };
        nlohmann::json current = m_json;
        for (const auto& key : keys) {
            if (!current.contains(key)) {
                return std::nullopt;
            }
            current = current[key];
        }
        // 显式判断返回，避免类型不匹配
        if (current.is_object()) {
            return current; 
        }
        return std::nullopt;
    }

    //获取全局默认校验配置
    json getGlobalValidationConfig() {
        if (m_json.contains("validation") && m_json["validation"].contains("global_default")) {
            return m_json["validation"]["global_default"];
        }
        return json::object();
    }
};