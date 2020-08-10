## Signals and Busses

Popular Model 3/Y CAN messages and signals, and which bus they are on.

Updated with firmware 2020.24

```
V C ID  signals
X X 00C UI audio display
X X 04F GPS loc
  X 101 RCM pitch roll yaw
X X 108 Rear Tourqe -diff?
  X 111 RCM lat lon vert accel
X X 118 pedal gear
X   126 Rear V A
X X 129 Steering angle
X   132 BMS V A
  X 185 Brake Torque
X   1D8 Rear Torque -diff?
  X 219 TPMS press temps
X X 229 Gear lever
  X 238 UI speedlimit
X X 249 left stalk
X   252 BMX discharge regen heat
X X 257 speedometer
X   261 12v batt V A AH
X   264 charger V A W
X   266 Rear power heat
X   268 system power regen heat
X   287 PTC
X   292 BMS SOCs packenergy
X X 2B4 PCS V A
X   2D2 minmax V A
X   312 BMS temps
X X 318 date time
  X 31F TPMS press temps
X   332 BMS brick minmax V T
X   336 power regen rating
X   33A UI range SOC
X   352 BMS energy remaining full
X X 399 DAS BSD speedlimit
X   3A1 VCfront driverpresent
X   3D8 elevation
X X 3D9 GPS heading speed limit
X X 3FE brake T
X X 7FF GTW config

```
