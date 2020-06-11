Using Octave, https://octave.sourceforge.io, and comFramework, https://sourceforge.net/projects/comframe/, an Octave program was written to extract important message and signal information from a DBC file and save it as a .xlsx file. The resulting program has the following setup: 

| Message Name |	Message ID |	Message Length |	Signal Name |	Signal Start Bit |	Signal Length |	Signal Factor |	Signal Offset |	Signal Unit |
| --- |	--- |	--- |	--- |	--- |	--- |	--- |	--- |	--- |

The .stg file can be edited to change what is imported from the DBC file into the Octave structure and the Octave program can be changed to change the resulting .xlsx file. 

Octave pacakges io and windows will also need to be installed. https://octave.sourceforge.io/packages.php
