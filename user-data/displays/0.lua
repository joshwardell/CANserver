-- Display battery power in KW, with battery power on bargraph

-- Turn display off if car center display is off
if ((CANServer_getAnalysisVar("DisplayOn") == 0) and (CANServer_getAnalysisVar("UI_systemActive") == 1)) then
    return "1m                  t0b1000r"
end

-- calculate power from volts * amps
local battPowerKW = (CANServer_getAnalysisVar("BattVolts") * CANServer_getAnalysisVar("BattAmps") / 1000.0)
local graphBattPower = math.floor(math.min(math.max((24) * (battPowerKW) / (300), -24), 24))

return "65535c" .. math.floor(battPowerKW * 10) .. "vWK  Bu" .. graphBattPower .. "b0m100r"
