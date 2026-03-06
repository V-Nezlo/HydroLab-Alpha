#pragma once

#include <UtilitaryRS/RsTypes.hpp>
#include <array>
#include <cstdint>

struct CalibPoint {
	float level;
	float litre;
};

enum class PumpModes {
	EBBNormal,
	EBBSwing,
	Dripping,
};

enum class PlainType { Drainage, Irrigation };
enum class SwingState { SwingOn, SwingOff };

// clang-format off
enum class DeviceStatus {
	NotFound    = 0,
	Warning     = 1,
	Working     = 2,
	Error       = 3
};
// clang-format on

namespace Log {

enum class Level {STATUS = 0, ERROR = 1, WARN = 2, INFO = 3, DEBUG = 4, TRACE = 5 };

}
