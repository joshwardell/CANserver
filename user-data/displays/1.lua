if (CANServer_getAnalysisVar("DisplayOn") < 1) then
    return "1m t0b1000r"
end

local rearTorque = CANServer_getAnalysisVar("RearTorque")
local graphRearTorque = math.floor(math.min(math.max((24) * (rearTorque) / (400), -24), 24))

return "65535c" .. math.floor(rearTorque * 10) .. "vWK  Bu" .. graphRearTorque .. "b0m100r"
