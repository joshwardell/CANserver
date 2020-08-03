return "65535c" .. math.floor(CANServer_getVar("BattPower") * 10) .. 
	"vWK  Bu" .. math.floor(CANServer_getVar("BattPower_Scaled_Bar")) .. "b0m100r"
