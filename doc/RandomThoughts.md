Random Thoughts on the DIY MMU 2.0
======================
by Chuck Kozlowski
----------------------------

I started the home-brew MMU2 project on September 19th, 2018.  It has now been 4 weeks and the unit has been working quite well over the past few days after a solid month of tinkering.  Here are a couple of things to help those with their MMU2 adventures not necessarily in any type of useful order:

# 1.  Filament Path
make certain your path from the filament spool all the way to the top of the mk3 extruder is smooth and does not bind.  If there is any binding, find the source (typically at one of the interface points) and fix it.  You will need to do this for each of the 5 filament paths.  I can't emphasize this enough - you need to have each filament path flowing smoothhly in order for the MMU2 to operate correctly.  If you don't do this, you will pay the price for both filament loading and unloading while the MMU2 is attempting to manipulate the filament.

# 2.  Festo Couplers 
I used 10mm OD (threaded) x 4mm (ID) couplers at ALL interface points (one at the selector of the MMU2 and another at the top of the MK3).  My filament touches no metal and this is key to avoid small filament shavings from accumulating while the MMU2 is operating

# 3.  Filament Loading
## Part 1
I realize that the stock MMU2 requires the operator to dial-in the filament length for each of the 5 filaments.  Make certain you take the time to open the bondtech door and see that the filament is going ALL the way to the middle of the bondtech gear. If you don't do this then you will have intermittent filament-load issues which usuallly result in a missing layer during operation
## Part 2
I chose to modify the fundamental design and added a 2nd filament sensor at the top of the MK3 extruder so I know that the filament is actually getting loaded. In fact,  I don't really care about the length of the bowden tube from the MMU2 to the MK3 extruder since the sensor tells me when the filament has arrived.  When the filament arrives,  I push it an additional 31mm and it lands right in the middle of the bondtech gear EVERY time.  I think that the current design of the MMU2 is fundamentally flawed in this area and you will have a difficult time operating 100% of the time without a modification similar to the one I just outlined.

# 4.  Filament Unloading
This is another tricky area since the hot-end does things to the end of the filament during an extraction.  The goal is to form a tip with no stringiness before the filament unload occurs.  If you have not replaced your stock 2.00mm PTFE hot-end part (50mm in length) then STOP reading and go and install the provided part from your kit (1.85mm PTFE).  Since I built my unit from scratch and there is no 1.85mm PTFE in the wild - I procured some 1.80mm PTFE (flurostore.com) and it works wonderfully.  The main reason for this critical piece is to make sure the blob that naturally forms on the end of the filament during a full retraction needs to stay below the magical 2.00mm PTFE ID width.  If you don't do this upgrade then you will have filament unload jamming and that is suck factor 10.

# 5.  Slic3r Settings
Make sure your 'Cooling Moves' are at least 2 when you start.  This is key to forming a properly filament tip during the filament unload process.  Make certain that your 'filament load' setting is at least 21 mm/sec.  I have mine currently set at 30 mm/sec but that is because I have tuned my firmware in the MMU2  to match this speed.  There are some very critical things happending for up to 2 seconds during a filament load when both the MMU2 gears and the MK3 bondtech gear are operating synchronously - if you don't have the speeds match then you will hear filament grinding.  Grinding is not your friend - you will need to adjust the filament load speed until grinding stops.
