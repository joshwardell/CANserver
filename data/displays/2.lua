local bsr = CANServer_getVar("BSR");
local bsl = CANServer_getVar("BSL");

if (bsr > 0 and bsl > 0) then
	return "3v63488c6m100r";
elseif (bsr > 0 and bls == 0) then
	return "2v63488c6m100r";
elseif (bsr == 0 and bsl > 0) then
	return "1v63488c6m100r";
else 
	local speedUnitText = "hmk"
	if (CANServer_getVar("DistanceUnitMiles") == 1) then
		speedUnitText = "hpm"
	end
	return "65535c" .. math.floor(CANServer_getVar("VehSpeed")) .. "v" .. 
		speedUnitText .. "u" .. math.floor(CANServer_getVar("BattPower_Scaled_Bar")) .. "b0m100r"
end