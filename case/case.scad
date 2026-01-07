
BOARDW = 35;
BOARDL = 74;
BOARDH = 2;
   

module pcb(x, y, z)
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
   WALLTHICK = 20;
   
   difference()
   {
      color([0, 0, 1])
      {
         translate([-width / 2, -length / 2, 0])
         cube([width, length, height], false);
      }
      
      // main well
      color([0, 0, 1])
      {
         translate([-(width - WALLTHICK) / 2, -(length - WALLTHICK) / 2, 3])
         cube([width - WALLTHICK, length - WALLTHICK, height - height + 20], false);
      }
      
      // board well 1
      color ([.3, .3, 1])
      {
          translate([-width / 2 + 2, -length / 2 + 2, 10])
          cube([79.5, 38, 15], false);
      }
      
      // board well 2
      color ([.3, .3, 1])
      {
          translate([-width / 2 + 10, -length / 2 + 2, 3])
          cube([58, 38, 15], false);
      }
      
      // usb well
      color ([.1, .4, 1])
      {
          translate([-width / 2 - 2, -length / 2 + 13, 13])
          cube ([8, 16, 15], false);
      }
      
      // diamonds 1
      for(yy = [-10:7:10])
      {
         for (xx = [-30:7:30])
         {
            color([0, 1, 1])
            translate([xx, yy + 1, -1])
            rotate([0, 0, 45])
            cube([3, 3, 4.01], false);
         }
      }
      
      // diamonds 2
      for (yy = [-5:7:5])
      {
         for (xx = [-27:7: 27])
         {
            color([0, 1, 1])
            translate([xx + .9, yy - .4, -1])
            rotate([0, 0, 45])
            cube([3, 3, 5], false);
         }
      }

      // holes for threaded inserts
      color([1, .7568, .4314])
      {
         translate([-5 - BOARDL / 2 + 3.5, 0 - BOARDW / 2 + 3.5, 10])
         cylinder(h = 18, d = 4, center = true, $fn=360);
         
         translate([-5 + BOARDL / 2 - 3.5, 0 - BOARDW / 2 + 3.5, 10])
         cylinder(h = 18, d = 4, center = true, $fn=360);
         
         translate([-5 + BOARDL / 2 - 3.5, 0 + BOARDW / 2 - 3.5, 10])
         cylinder(h = 18, d = 4, center = true, $fn=360);
         
         translate([-5 - BOARDL / 2 + 3.5, 0 + BOARDW / 2 - 3.5, 10])
         cylinder(h = 18, d = 4, center = true, $fn=360);
      }
      
      // antenna openings
      color([0, 0, 1])
      {
          translate([width / 2, 0, height / 2 + 1])
          cube([30, 13, 13], true);
          translate([width / 2, 0, height / 2 + 5])
          cube([30, 13, 13], true);
          
          translate([width / 2 - 7, -8, height / 2 - 8])
          cube([6, 16, 20], false);
      }
      
    }
   
 
    // antenna
    color([1,.922,.804])
    translate ([38.5S, -7.5, 2])
    cube([5, 15, 15], false);
}

bottom(90, 42, 20);
pcb(-5, 0, 10);// BOARDH / 2);
