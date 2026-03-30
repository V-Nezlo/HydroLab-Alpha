#pragma once

#include "BbNames.hpp"
#include "core/Blackboard.hpp"
#include "core/BlackboardEntry.hpp"
#include "core/Options.hpp"
#include "core/Types.hpp"
#include "logger/Logger.hpp"
#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>

/// \brief Вычислитель литраха рабочего тела и бака
class LitreMeter : public AbstractEntryObserver {
	struct CalibEntry {
		float level;
		float litres;
	};

	std::shared_ptr<Blackboard> bb;
	std::vector<CalibEntry> calibTable;

	struct {
		BlackboardEntry<float> waterLevel;
		BlackboardEntry<bool> flowDetector;
		BlackboardEntry<bool> calibTableOutdated;
		BlackboardEntry<PlainType> plainType;
	} inKeys;

	struct {
		BlackboardEntry<float> fullLitre;
		BlackboardEntry<float> tankLitre;
		BlackboardEntry<float> tubeLitre;
	} outKeys;

public:
	LitreMeter(std::shared_ptr<Blackboard> aBb) :
		bb{aBb},
		inKeys
		{
			{Names::getValueNameByDevice(Names::kWaterLevelDev), aBb},
			{Names::getValueNameByDevice(Names::kFlowDetectorDev), aBb},
			{"calibTableOutdated, aBb"},
			{Names::kPumpPlainType, aBb}
		},
		outKeys
		{
			{Names::kLitreMeterTankVal, aBb},
			{Names::kLitreMeterTubeVal, aBb},
			{Names::kLitreMeterFullVal, aBb},
		}
	{
		inKeys.calibTableOutdated = false;
		inKeys.calibTableOutdated.subscribe(this);
		inKeys.waterLevel.subscribe(this);

		calibTable.reserve(16);
	}

	// AbstractEntryObserver interface
	void onEntryUpdated(std::string_view entry, const std::any &value) override
	{
		if (entry == inKeys.waterLevel.getName()) {
			const float level = std::any_cast<float>(value);
			const float litres = levelToLitres(level);
			// Если сейчас идет осушение и все фитинги без воды - значит весь обьем находится в баке
			if (inKeys.plainType() == PlainType::Drainage && inKeys.flowDetector() == false) {
				outKeys.fullLitre = litres;
			}

			outKeys.tubeLitre = outKeys.fullLitre() - level;
			outKeys.tankLitre = litres;

		} else if (entry == inKeys.calibTableOutdated.getName()) {
			if (inKeys.calibTableOutdated()) {
				inKeys.calibTableOutdated = false;
				recalculateTable();
			}
		}
	}

private:
	bool recalculateTable()
	{
		const auto levKeys = bb->getKeysByPrefix(Names::kLitreMeterCalibLev);
		const auto litKeys = bb->getKeysByPrefix(Names::kLitreMeterCalibLit);

		if (levKeys.size() != litKeys.size() || levKeys.size() != Options::kCalibTableSize) {
			HYDRO_LOG_ERROR("Wrong calibration table size");
			return false;
		}

		const size_t arraySize = Options::kCalibTableSize;
		std::vector<CalibEntry> newTable;

		for (size_t i = 0; i < arraySize; ++i) {
			const auto levI = bb->get<float>(Names::kLitreMeterCalibLev + std::to_string(i));
			const auto litI = bb->get<float>(Names::kLitreMeterCalibLit + std::to_string(i));

			if (!levI || !litI) {
				HYDRO_LOG_ERROR("Calibration table access error");
				return false;
			}

			if (!std::isfinite(levI.value()) || !std::isfinite(litI.value())) {
				HYDRO_LOG_ERROR("Calibration table contains non-finite value");
				return false;
			}

			CalibEntry entry = {levI.value(), litI.value()};
			newTable.push_back(entry);
		}

		std::sort(newTable.begin(), newTable.end(), [] (const CalibEntry &a, const CalibEntry &b) {return a.level < b.level;});

		for (size_t i = 1; i < arraySize; ++i) {
			if (std::fabsf(newTable[i].level - newTable[i - 1].level) < 0.001f) {
				HYDRO_LOG_ERROR("Dublicating calibration values");
				return false;
			}
			if (newTable[i].litres < newTable[i - 1].litres) {
				HYDRO_LOG_ERROR("Calibration table not monotonic");
				return false;
			}
		}

		calibTable = std::move(newTable);
		return true;
	}

	float levelToLitres(float aLevel) const
	{
		if (calibTable.empty()) {
			HYDRO_LOG_ERROR("Calibration table is empty");
			return 0.0f;
		}

		if (aLevel <= calibTable.front().level) {
			return calibTable.front().litres;
		}

		if (aLevel >= calibTable.back().level) {
			return calibTable.back().litres;
		}

		const auto it = std::lower_bound(calibTable.begin(), calibTable.end(), aLevel,
			[](const CalibEntry &aEntry, float aLev) { return aEntry.level < aLev; });

		if (it == calibTable.end()) {
			return calibTable.back().litres;
		}

		const auto &right = *it;
		const auto &left = *(it - 1);

		const float levRange = right.level - left.level;
		const float k = (aLevel - left.level) / levRange;
		return left.litres + k * (right.litres - left.litres);
	}
};

