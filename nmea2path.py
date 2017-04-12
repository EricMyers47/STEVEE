# Read the lines of a file, expected to be in NMEA format,
# and output as a KML path file
#
# Eric Myers <Eric.Myers@usma.edu> - 08 April 2017
# United States Military Academy at West Point, New York
#######################################################################

import pynmea2 as nmea


class Point3D:
  def __init__(self):
    self.timestamp = " "
    self.lat = 0.0
    self.lon = 0.0
    self.alt = 0.0
  def pr(self):
     if hasattr(self,"alt"): 
       print ("     %f,%f,%.1f" % ( self.lon, self.lat,self.alt) )

# TODO: change this to read either name on command line, or from stdin
filename = "GPSLOG13.TXT"

try: 
  fd = open(filename)
except (OSError, IOError) as e:
  print "Cannot open file."
  print e
  exit()

# Begin a KML file.  Emulating an example from Google tutorial at
# https://developers.google.com/kml/documentation/kml_tut#placemarks

print """<?xml version="1.0" encoding="UTF-8"?>
<kml xmlns="http://www.opengis.net/kml/2.2">
  <Document>
    <name>GPSLOG13</name>
    <description>Sample data from Ultimate GPS Shield for Arduino: Eric's drive home.
    </description>
    <Style id="yellowLineGreenPoly">
      <LineStyle>
        <color>7f00ffff</color>
        <width>4</width>
      </LineStyle>
      <PolyStyle>
        <color>7f00ff00</color>
      </PolyStyle>
    </Style>
    <Placemark>
      <name>Absolute Extruded</name>
      <description>Transparent green wall with yellow
    outlines</description>
      <styleUrl>#yellowLineGreenPoly</styleUrl>
      <LineString>
        <extrude>1</extrude>
        <tessellate>1</tessellate>
        <altitudeMode>absolute</altitudeMode>
        <coordinates>"""


lastPoint = Point3D()   # most recent position/altitude
i=0

for line in fd:
    i = i+1
    #if i > 10 : break   

    #if  not line.startswith("$GPGGA") :
    #    continue
    #print line
    msg = nmea.parse(line)
    if not msg.is_valid:  continue
    #print msg.sentence_type, msg.timestamp, msg.latitude, msg.longitude

    #if msg.sentence_type == "GGA" :
    #  print msg.altitude

    # If the timestamp is different from the previous line then
    # dump what he have collected to the file.
    if not (msg.timestamp == lastPoint.timestamp) :
        lastPoint.pr()            # Print it out
        lastPoint = Point3D()     # and create a fresh, empty one

    # save the timestamp

    lastPoint.timestamp = msg.timestamp

    # save what we can get from this line

    if hasattr(msg, "latitude") :
        lastPoint.lat = msg.latitude

    if hasattr(msg, "longitude") :
        lastPoint.lon = msg.longitude

    if hasattr(msg, "altitude") :
        if msg.altitude != None :
            lastPoint.alt = msg.altitude

# Finish up the KML file

print """       </coordinates>
      </LineString>
    </Placemark>
  </Document>
</kml>"""


