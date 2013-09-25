The Sprinkler Controller hardware was developed by Nick Horvath to work with sprinklers pi. Select the "Direct Positive" Output mode in Settings.
It allows you to control 10 Zones or 9 Zones and a Master (Pump zone in sprinklers pi). Just solder a jumper wire from the center of JP1 to the designated pad to select which fits your system.
Since only one zone is enabled at a time they can share a common resistor (R1) except for zone 10 which has a dedicated resistor in case it is being used as a master.

Materials List:
1 3"x4" Dual sided PCB (I used photoresist boards from Jameco)
10 SSR with at least 0.8A 24V capacity, that can be switched by 3.3V. I used: Sharp PR29MF21NSZF http://www.digikey.com/product-search/en?WT.z_header=search_go&lang=en&site=us&keywords=425-2365-5-ND&x=0&y=0&formaction=on
Snap together screw terminals with 3.5mm pitch:
 1 block of 10 (I did 4+4+2 but it doesn't matter)
 1 block of 3.
2 100 Ohm Resistors (if you use a different SSR you may need to adjust the resistance value to match your relay's expected current and voltage drop)
1 2x13 Pin extra tall female header (standard height headers should work but you will need to tape off the USB and Ethernet jacks on the RPI and it will sit crooked, yes I found this out the hard way)
1 Raspberry Pi (rev doesn't matter)
