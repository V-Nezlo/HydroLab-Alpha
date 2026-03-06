#include "SettingsManager.hpp"
#include <filesystem>
#include <iostream>

SettingsManager::SettingsManager(std::shared_ptr<Blackboard> aBlackboard,
		const std::string &aFilename, bool aAutoSave) :
	filename{aFilename},
	blackboard{aBlackboard},
	autoSave{aAutoSave},
	initialized{false}
{
}

SettingsManager::~SettingsManager()
{
	for (const auto &[key, _] : schema) {
		blackboard->unsubscribe(key, this);
	}
}

void SettingsManager::registerSetting(
	const std::string &key, SettingType type, SettingValue defaultValue, const std::string &desc)
{
	SettingDefinition def{key, type, defaultValue, desc};
	schema[key] = def;
	defaults[key] = defaultValue;

	blackboard->subscribe(key, this);
}

bool SettingsManager::load()
{
	auto effective = defaults;
	json config;
	bool isFileExist = std::filesystem::exists(filename);
	bool isSchemaValid = true;
	bool result = true;

	if (!isFileExist) {
		std::cout << "Config not found. Creating default config.\n";
	} else {
		try {
			std::ifstream file(filename);
			config = json::parse(file);
			isSchemaValid = validateSchema(config);

			if (!isSchemaValid) {
				std::cout << "Schema mismatch. Recreating config with defaults.\n";
			} else {
				// Подмешиваем значения если файл есть и он валиден
				if (config.contains("values") && config["values"].is_object()) {
					for (auto &[key, jval] : config["values"].items()) {
						if (!schema.contains(key))
							continue;

						const auto &def = schema.at(key);

						try {
							switch (def.type) {
								case SettingType::BOOL:
									effective[key] = jval.get<bool>();
									break;

								case SettingType::INT:
									effective[key] = jval.get<int>();
									break;

								case SettingType::FLOAT:
									effective[key] = jval.get<float>();
									break;

								case SettingType::STRING:
									effective[key] = jval.get<std::string>();
									break;

								case SettingType::SECONDS:
									// Fallthrough
								case SettingType::MSECONDS:
									// Сериализация в int
									effective[key] = jval.get<int>();
									break;
							}
						} catch (...) {
							effective[key] = def.defaultValue;
						}
					}
				}

				std::cout << "Config loaded successfully.\n";
			}

		} catch (const std::exception &e) {
			std::cerr << "Error loading config: " << e.what() << "\n";
			result = false;
		}
	}

	deSerializeTypes(effective);
	writeToBlackboard(effective);

	initialized = true;
	std::cout << "Config loaded successfully.\n";
	return result;
}

void SettingsManager::onEntryUpdated(std::string_view entry, const std::any & /*value*/)
{
	if (!initialized) {
		return;
	}

	if (schema.contains(entry) && autoSave) {
		save();
	}
}

bool SettingsManager::save()
{
	try {
		json config = getConfigForSaving();
		std::ofstream file(filename);
		file << config.dump(4);

		return true;

	} catch (const std::exception &e) {
		std::cerr << "Error saving config: " << e.what() << "\n";
		return false;
	}
}

void SettingsManager::writeToBlackboard(const Parameters &vals)
{
	for (const auto &[key, val] : vals) {
		std::visit([&](auto &&arg) { blackboard->set(key, arg); }, val);
	}
}

bool SettingsManager::validateSchema(const json &existingConfig) const
{
	if (!existingConfig.contains("schema") || !existingConfig["schema"].is_object()) {
		return false;
	}

	const json &existingSchema = existingConfig["schema"];

	for (const auto &[key, def] : schema) {
		if (!existingSchema.contains(key)) {
			return false;
		}

		const json &edef = existingSchema[key];
		if (!edef.contains("type")) {
			return false;
		}

		if (edef["type"] != toString(def.type)) {
			return false;
		}
	}

	return true;
}

json SettingsManager::generateSchema() const
{
	json out;

	for (const auto &[key, def] : schema) {
		json d;
		d["type"] = toString(def.type);
		d["description"] = def.description;

		std::visit(
			[&](auto &&arg) {
				using T = std::decay_t<decltype(arg)>;
				if constexpr (std::is_same_v<T, std::chrono::seconds>) {
					d["default"] = arg.count();
					d["unit"] = "s";
				} else if constexpr (std::is_same_v<T, std::chrono::milliseconds>) {
					d["default"] = arg.count();
					d["unit"] = "ms";
				} else {
					d["default"] = json(arg);
				}
			},
			def.defaultValue);

		out[key] = d;
	}

	return out;
}

json SettingsManager::getConfigForSaving() const
{
	json cfg;
	cfg["schema"] = generateSchema();

	json vals;

	for (const auto &[key, def] : schema) {
		switch (def.type) {
			case SettingType::BOOL:
				if (auto v = blackboard->get<bool>(key)) vals[key] = *v;
				break;

			case SettingType::INT:
				if (auto v = blackboard->get<int>(key)) vals[key] = *v;
				break;

			case SettingType::FLOAT:
				if (auto v = blackboard->get<float>(key)) vals[key] = *v;
				break;

			case SettingType::STRING:
				if (auto v = blackboard->get<std::string>(key)) vals[key] = *v;
				break;

			case SettingType::SECONDS:
				if (auto v = blackboard->get<std::chrono::seconds>(key)) vals[key] = v->count();
				break;

			case SettingType::MSECONDS:
				if (auto v = blackboard->get<std::chrono::milliseconds>(key)) vals[key] = v->count();
				break;
		}
	}

	cfg["values"] = vals;
	return cfg;
}

std::string SettingsManager::toString(SettingType type)
{
	switch (type) {
		case SettingType::BOOL:
			return "bool";
		case SettingType::INT:
			return "int";
		case SettingType::FLOAT:
			return "float";
		case SettingType::STRING:
			return "string";
		case SettingType::SECONDS:
			return "seconds";
		case SettingType::MSECONDS:
			return "milliseconds";

		default:
			return "unknown";
	}
}

void SettingsManager::deSerializeTypes(Parameters &effective)
{
	for (const auto &[key, def] : schema) {
		auto it = effective.find(key);
		if (it == effective.end()) {
			effective[key] = def.defaultValue;
			continue;
		}

		auto &v = it->second;

		auto asInt = [&]() -> std::optional<int> {
			if (auto p = std::get_if<int>(&v))
				return *p;
			if (auto p = std::get_if<float>(&v))
				return static_cast<int>(*p);
			if (auto p = std::get_if<bool>(&v))
				return *p ? 1 : 0;
			return std::nullopt;
		};

		switch (def.type) {
			case SettingType::SECONDS: {
				if (std::holds_alternative<std::chrono::seconds>(v))
					break;
				auto n = asInt().value_or(0);
				if (n < 0)
					n = 0;
				v = std::chrono::seconds{n};
				break;
			}

			case SettingType::MSECONDS: {
				if (std::holds_alternative<std::chrono::milliseconds>(v))
					break;
				auto n = asInt().value_or(0);
				if (n < 0)
					n = 0;
				v = std::chrono::milliseconds{n};
				break;
			}

			default:
				break;
		}
	}
}
