#pragma once

#include "BbNames.hpp"
#include "SettingsManager.hpp"
#include "core/Blackboard.hpp"
#include "core/FieldValidators.hpp"
#include "core/Helpers.hpp"
#include "core/InterfaceList.hpp"
#include "core/Options.hpp"
#include <memory>
#include <string>

/// \brief Централизованный генератор конфигурации
struct ConfigPackage {
	ConfigPackage(std::string aFileName, std::shared_ptr<Blackboard> aBb) :
		name{aFileName},
		bb{aBb},
		manager{aBb, aFileName}
	{
		registerSettings();
		generateValidators();
	}

	void registerSettings()
	{
		manager.registerSetting(Names::kLampEnabled, SettingType::BOOL, true, "Enable lamp");
		manager.registerSetting(Names::kLampOnTime, SettingType::INT, 0, "Lamp on time in minutes");
		manager.registerSetting(Names::kLampOffTime, SettingType::INT, 0, "Lamp off time in minutes");

		manager.registerSetting(Names::kPumpEnabled, SettingType::BOOL, true, "Enable pump");
		manager.registerSetting(Names::kPumpMode, SettingType::INT, 0, "Pump mode");

		manager.registerSetting(Names::kPumpOnTime, SettingType::SECONDS, 15, "Pump on time in seconds");
		manager.registerSetting(Names::kPumpOffTime, SettingType::SECONDS, 30, "Pump off time in seconds");
		manager.registerSetting(Names::kPumpSwingTime, SettingType::SECONDS, 8, "Pump swing time in seconds");
		manager.registerSetting(Names::kPumpValidTime, SettingType::SECONDS, 50, "Upper validation time in seconds");
		manager.registerSetting(Names::kPumpMaxFloodTime, SettingType::SECONDS, 180, "Max time for tank flooding in secs");

		manager.registerSetting(Names::kWaterLevelMinLevel, SettingType::FLOAT, 0.f, "Minimal water level in percent for pump operation");

		manager.registerSetting(Names::kSystemMaintance, SettingType::BOOL, false, "System maintance mode");
		manager.registerSetting(Names::kBridgeMacs + ".1", SettingType::STRING, "E8:31:CD:D6:D1:B4", "MAC1");
		manager.registerSetting(Names::kBridgeMacs + ".2", SettingType::STRING, "", "MAC2");
		manager.registerSetting(Names::kBridgeMacs + ".3", SettingType::STRING, "", "MAC3");
		manager.registerSetting(Names::kBridgeMacs + ".4", SettingType::STRING, "", "MAC4");
		manager.registerSetting(Names::kBridgeMacs + ".5", SettingType::STRING, "", "MAC5");
		manager.registerSetting(Names::kBridgeMacs + ".6", SettingType::STRING, "", "MAC6");

		// Генерация таблицы калибровки бака
		// Дефолтным значением будет калибровка на емкость 30 литров
		for (size_t i = 0; i < Options::kCalibTableSize; ++i) {
			const float level = static_cast<float>(i * 10);
			const float litre = static_cast<float>(i * 3);
			const std::string number = std::to_string(i);
			manager.registerSetting(Names::kLitreMeterCalibLev + number, SettingType::FLOAT, level, "Calibration point #" + number + " water level");
			manager.registerSetting(Names::kLitreMeterCalibLit + number, SettingType::FLOAT, litre, "Calibration point #" + number + " litres");
		}
	}

	void generateValidators()
	{
		std::unique_ptr<AbstractValidator> macVal = std::make_unique<MacFieldValidator>();
		bb->insertValidator(Names::kBridgeMacs, std::move(macVal));
	}

	bool load()
	{
		return manager.load();
	}

private:
	std::string name;
	std::shared_ptr<Blackboard> bb;

	SettingsManager manager;
};
