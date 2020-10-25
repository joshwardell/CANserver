-- Display rear motor torque, with torque on bargraph

-- Turn display off if car center display is off
if ((CANServer_getAnalysisVar("DisplayOn") == 0) and (CANServer_getAnalysisVar("UI_systemActive") == 1)) then
    return "1m                  t0b1000r"
end

local rearTorque = CANServer_getAnalysisVar("RearTorque")
local graphRearTorque = math.floor(math.min(math.max((24) * (rearTorque) / (400), -24), 24))

return "65535c" .. math.floor(rearTorque * 10) .. "vMNu" .. graphRearTorque .. "b0m100r"
