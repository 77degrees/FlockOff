


module pcb(x, y, z)
{
   BOARDW = 35;
   BOARDL = 74;
   BOARDH = 2;
   
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
   
   // ESP32-S3
   color([0, .3, 0])
   translate([x - BOARDL / 2, y - BOARDW / 4, z + BOARDH + 1])
   cube([22, 18, 2], false);
   
   // ESP32-S3 USB
   color([.765, .765, .765])
   {
      translate([x - BOARDL / 2 - 2, y - BOARDW / 4 + 4.5, z + BOARDH + 4.5])
      rotate([0, 90, 0])
      cylinder(h=7 , d=3, $fn=360);
      
      //color([.765, .765, .765])
      translate([x - BOARDL / 2 - 2, y - BOARDW / 4 + 13.5, z + BOARDH + 4.5])
      rotate([0, 90, 0])
      cylinder(h=7 , d=3, $fn=360);
      
      //color([.765, .765, .765])
      translate([x - BOARDL / 2 - 2, y - BOARDW / 4 + 4.5, z + BOARDH + 3])
      cube([7, 9, 3], false);
   }
   
   // ESP32-S3 mounting headers
   color([0,0,0])
   {
      translate([x - BOARDL / 2 + 2, y - BOARDW / 2 + 9, z + BOARDH / 2])
      cube([18, 2, 2], false);
      
      translate([x - BOARDL / 2 + 2, y + BOARDW / 2 - 11, z + BOARDH / 2])
      cube([18, 2, 2], false);
   }
}

module bottom(width, length, height)
{
   WALLTHICK = 5;
   
   difference()
   {
      color([0, 0, 1])
      {
         translate([-width / 2, -length / 2, 0])
         cube([width, length, height], false);
      }
      
      color([0, 0, 1])
      {
         translate([-(width - WALLTHICK) / 2, -(length - WALLTHICK) / 2, WALLTHICK + .1])
         cube([width - WALLTHICK, length - WALLTHICK, height - WALLTHICK], false);
      }
      
      for(yy = [-14:7:14])
      {
         for (xx = [-40:7: 40])
         {
            color([0, 1, 1])
            translate([xx, yy, -1])
            rotate([0, 0, 45])
            cube([3, 3, WALLTHICK + 4], false);
         }
      }
      for (yy = [-17:7:17])
      {
         for (xx = [-37:7: 37])
         {
            color([0, 1, 1])
            translate([xx + .1, yy - .3, -1])
            rotate([0, 0, 45])
            cube([3, 3, WALLTHICK + 4], false);
         }
      }
    }



}

bottom(90, 45, 20);
pcb(0, 0, 20);// BOARDH / 2);
