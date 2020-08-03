--[[ Sort out the wattage based on V*A --]]
local battPower = (CANServer_getVar("BattVolts") * CANServer_getVar("BattAmps") / 1000.0);
CANServer_setVar("BattPower", battPower);

--[[ Scale the battery power for the bargraph --]]
local scaledBattPower = math.min(math.max((24) * (battPower) / (300), -24), 24);
CANServer_setVar("BattPower_Scaled_Bar", math.floor(scaledBattPower + 0.5));

--[[ Scale the rear torque for the bargraph --]]
local scaledRearTorque = math.min(math.max((24) * (CANServer_getVar("RearTorque")) / (400), -24), 24);
CANServer_setVar("RearTorque_Scaled_Bar", math.floor(scaledRearTorque + 0.5));

--[[ Update speed to be in the display units --]]
local vehSpeed = CANServer_getVar("VehSpeed");
if (CANServer_getVar("DistanceUnitMiles") == 1) then
	vehSpeed = vehSpeed * 0.6213712;
end
CANServer_setVar("VehSpeed", vehSpeed);

--[[ Sort out blind spots --]]
local bsl = CANServer_getVar("BSL");
local bsr = CANServer_getVar("BSR");
if (bsl > 2) then
	bsl = 0
end
if (bsr > 2) then
	bsr = 0
end
CANServer_setVar("BSL", bsl);
CANServer_setVar("BSR", bsr);