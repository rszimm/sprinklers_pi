The Sprinkler Controller hardware was developed by Nick Horvath to work with sprinklers pi. Select the "Direct Positive" Output mode in Settings.
It allows you to control 10 Zones or 9 Zones and a Master (Pump zone in sprinklers pi). Just solder a jumper wire from the center of JP1 to the designated pad to select which fits your system.
Since only one zone is enabled at a time they can share a common resistor (R1) except for zone 10 which has a dedicated resistor in case it is being used as a master.

The layout was created in Fritzing (http://fritzing.org/) a free, open source, cross platform, PCB prototyping tool. I've also exported the board etch masks to PDF and SVG (I used the PDF when I did mine).

Materials List:
1 3"x4" Dual sided PCB (I used photoresist boards from Jameco)
10 SSR with at least 0.8A 24VAC capacity, that can be switched by 3.3V. I used: Sharp PR29MF21NSZF http://www.digikey.com/product-search/en?WT.z_header=search_go&lang=en&site=us&keywords=425-2365-5-ND&x=0&y=0&formaction=on
Snap together screw terminals with 3.5mm pitch:
 1 block of 10 (I did 4+4+2 but it doesn't matter)
 1 block of 3.
2 100 Ohm Resistors (if you use a different SSR you may need to adjust the resistance value to match your relay's expected current and voltage drop)
1 2x13 Pin extra tall female header (standard height headers should work but you will need to tape off the USB and Ethernet jacks on the RPI and it will sit crooked, yes I found this out the hard way)
1 Raspberry Pi (rev doesn't matter)

Notes & Helpful Hints:
* I made sure you can top-solder all the pins for the RPi because the header faces downward, and you bottom-solder all the screw terminals. To get all the Pi headers right I had to use 2 vias, just solder a bit of wire through the board on the top and bottom to complete the via (a lead clipped off a resistor after it's soldered in works well).
* Since DIY circuit boards are not thru-hole plated be sure to solder components on the side where there are traces coming off of the pad (and both sides if both sides have traces).
* If you're getting a different SSR make sure the pin functions are the same as the one I used or you will basically have to redesign the whole board.
* This is a good how-to video on home PCB etching with photo-resist: http://www.youtube.com/watch?v=MQezw8hlSrk
* Make sure you line up the first layer well, because you will have to match the second layer to it on the other side of the board without being able to see it. If you don't your drill holes won't line up.
* You photo expose both layers, one at a time, then develop, then etch.
* I used this etchant (Copper Chloride in Aqueous Hydrochloric Acid Solution) instead of Ferric Chloride (FeCl), it worked really well, was cheap and easy to make, chemicals were available at the hardware store, and now I don't have an environmental hazard to dispose of because it lasts indefinitely: http://www.instructables.com/id/Stop-using-Ferric-Chloride-etchant!--A-better-etc/#intro
