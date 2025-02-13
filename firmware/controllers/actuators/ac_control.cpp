#include "pch.h"

#include "ac_control.h"
#include "deadband.h"

// Deadbands to prevent rapid switching on/off of AC
static Deadband<200> maxRpmDeadband;
static Deadband<5> maxCltDeadband;
static Deadband<5> maxTpsDeadband;

bool AcController::getAcState() {
	latest_usage_ac_control = getTimeNowSeconds();
	auto rpm = Sensor::getOrZero(SensorType::Rpm);

	engineTooSlow = rpm < 500;

	if (engineTooSlow) {
		return false;
	}

	auto maxRpm = engineConfiguration->maxAcRpm;
	engineTooFast = maxRpm != 0 && maxRpmDeadband.gt(rpm, maxRpm);
	if (engineTooFast) {
		return false;
	}

	auto clt = Sensor::get(SensorType::Clt);

	noClt = !clt;
	// No AC with failed CLT
	if (noClt) {
		return false;
	}

	// Engine too hot, disable
	auto maxClt = engineConfiguration->maxAcClt;
	engineTooHot = (maxClt != 0) && maxCltDeadband.gt(clt.Value, maxClt);
	if (engineTooHot) {
		return false;
	}

	// TPS too high, disable
	auto maxTps = engineConfiguration->maxAcTps;
	tpsTooHigh = maxTps != 0 && maxTpsDeadband.gt(Sensor::getOrZero(SensorType::Tps1), maxTps);
	if (tpsTooHigh) {
			return false;
	}
	if (isDisabledByLua) {
		return false;
	}

	// All conditions allow AC, simply pass thru switch
	return acButtonState;
}

void AcController::onSlowCallback() {
	bool isEnabled = getAcState();

	m_acEnabled = isEnabled;

	enginePins.acRelay.setValue(isEnabled);
}

bool AcController::isAcEnabled() const {
	return m_acEnabled;
}
