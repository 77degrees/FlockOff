
BOARDW = 35;
BOARDL = 74;
BOARDH = 2;

module board(x, y, z)
{
   // main board with mounting holes
   difference()
   {
      color([.3, 0, .7])
      {
         translate([x, y, z])
         cube([BOARDL, BOARDW, BOARDH], true);
      }
      
      // 4 each 4mm diameter holes are 3.5mm from each edge
      color([0, 0, 0])
      {
         translate([x - BOARDL / 2 + 3.5, y - BOARDW / 2 + 3.5, z])
         cylinder(h = BOARDH + 2, d = 4, center = true, $fn=360);
         
         translate([x + BOARDL / 2 - 3.5, y - BOARDW / 2 + 3.5, z])
         cylinder(h = BOARDH + 2, d = 4, center = true, $fn=360);
         
         translate([x + BOARDL / 2 - 3.5, y + BOARDW / 2 - 3.5, z])
         cylinder(h = BOARDH + 2, d = 4, center = true, $fn=360);
         
         translate([x - BOARDL / 2 + 3.5, y + BOARDW / 2 - 3.5, z])
         cylinder(h = BOARDH + 2, d = 4, center = true, $fn=360);
      }
      
      // ESP32-S3
      color([0, .3, 0])
      translate([x, y, z + BOARDH])
      cube([22, 18, 2], true);
   }
   
   // LEDS
   color([.9, .9, .9])
   {
      translate([x - 2.5, y - 7.5, z + BOARDH + .5])
      cylinder(h = 1.5, d = 8.5, center = true, $fn=360);
      
      translate([x - 2.5, y - 7.5, z + BOARDH + 3.5])
      cylinder(h = 7, d = 8, center = true, $fn=360);
      
      translate([x - 2.5, y - 7.5, z + BOARDH + 7])
      sphere(d = 8, $fn=360);
      
      translate([x - 2.5, y + 8.75, z + BOARDH + .5])
      cylinder(h = 1.5, d = 8.5, center = true, $fn=360);
      
      translate([x - 2.5, y + 8.75, z + BOARDH + 3.5])
      cylinder(h = 7, d = 8, center = true, $fn=360);
      
      translate([x - 2.5, y + 8.75, z + BOARDH + 7])
      sphere(d = 8, $fn=360);
   }
}

module bottom(width, length, height)
{
   difference()
   {
      color([0, 0, 1])
      {
         translate([-width / 2, -length / 2, 0])
         cube([width, length, height], false);
      }
      
      color([0, 0, 1])
      {
         translate([-(width - 3) / 2, -(length - 3) / 2, 3.1])
         cube([width - 3, length - 3, height - 3], false);
      }
      
      
    }
}

//bottom(90, 45, 20);
board(0, 0, BOARDH / 2);
