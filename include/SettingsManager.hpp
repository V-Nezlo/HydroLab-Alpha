#pragma once

#include "core/Blackboard.hpp"

#include <array>
#include <chrono>
#include <fstream>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

using json = nlohmann::json;

enum class SettingType { BOOL, INT, FLOAT, STRING, SECONDS, MSECONDS};
using SettingValue = std::variant<bool, int, float, std::string, std::chrono::seconds, std::chrono::milliseconds>;

struct SettingDefinition {
	std::string key;
	SettingType type;
	SettingValue defaultValue;
	std::string description;
};

/* Таблица сериализации
 * std::chrono типы -> int
*/

/// \brief Класс менеджера настроек, является модулем BB
/// Автоматически создает entry в BB, но имеет и свои entry
/// Свои entry имеют удобно сериализируемые типы, так seconds -> int
class SettingsManager : public AbstractEntryObserver {
	using Schema = std::unordered_map<std::string, SettingDefinition, StrHash, StrEq>;
	using Parameters = std::unordered_map<std::string, SettingValue, StrHash, StrEq>;

	Schema schema;
	Parameters defaults;

	std::string filename;
	std::shared_ptr<Blackboard> blackboard;

	bool autoSave;
	bool initialized;

public:
	SettingsManager(std::shared_ptr<Blackboard> aBb,
					const std::string &aFileName = "config.json",
					bool aAutoSave = true);

	SettingsManager(const SettingsManager &) = delete;
	SettingsManager &operator=(const SettingsManager &) = delete;
	~SettingsManager() override;

	/// \brief Регистрация entry-параметров с публикацией в BB
	/// \param key Имя entry
	/// \param type Тип параметра
	/// \param defaultValue Значение по умолчанию
	/// \param desc Описание параметра
	void registerSetting(const std::string &key, SettingType type,
		SettingValue defaultValue, const std::string &desc = "");

	/// \brief Загрузить параметры из файла
	/// \return true если загрузка удачная, иначе false
	bool load();

	/// \brief Сохранить параметры в файл
	/// \return true если сохранение успешно, иначе false
	bool save();

	// AbstractEntryObserver interface
	void onEntryUpdated(std::string_view entry, const std::any &value) override;

private:

	/// \brief Сравнить существующую схему с новосозданной
	/// \param existingConfig существующая схема (из файла)
	/// \return true если схемы сходятся, иначе false
	bool validateSchema(const json &existingConfig) const;

	/// \brief Записать в BB, используется при инициализации
	/// \param vals пак параметров
	void writeToBlackboard(const Parameters &vals);

	/// \brief Собрать схему из переданных параметров
	/// \return json
	json generateSchema() const;

	/// \brief Возвращает json с параметрами, сериализует нетривиальные типы
	/// \return json с параметрами
	json getConfigForSaving() const;

	/// \brief deSerializeTypes
	/// \param effective
	void deSerializeTypes(Parameters &effective);

	/// \brief Получить строковое определение типа параметра
	/// \param type Тип параметра
	/// \return строка с названием
	static std::string toString(SettingType type);
};
