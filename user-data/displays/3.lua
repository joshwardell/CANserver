if (CANServer_getAnalysisVar("DisplayOn") < 1) then
    return "1m t0b1000r"
end

return "1m2s DISPLAY    3   t500r"