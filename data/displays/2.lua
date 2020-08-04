local battPowerKW = (CANServer_getVar("BattVolts") * CANServer_getVar("BattAmps") / 1000.0)
local graphBattPower = math.floor(math.min(math.max((24) * (battPowerKW) / (300), -24), 24))

local bsr = CANServer_getVar("BSR");
local bsl = CANServer_getVar("BSL");

if (bsr > 0 and bsl > 0) then
	return "3v63488c6m100r";
elseif (bsr > 0 and bls == 0) then
	return "2v63488c6m100r";
elseif (bsr == 0 and bsl > 0) then
	return "1v63488c6m100r";
else 
	local speedUnitText = "HMK"
	local displaySpeed = CANServer_getVar("VehSpeed")
	if (CANServer_getVar("DistanceUnitMiles") == 1) then
		speedUnitText = "HPM"
		displaySpeed = displaySpeed * 0.6213712;
	end

	return "65535c" .. math.floor(displaySpeed * 10) .. "v" .. speedUnitText .. "u" 
				.. graphBattPower .. "b0m100r"
end