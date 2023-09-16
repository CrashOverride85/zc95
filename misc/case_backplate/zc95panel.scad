difference() {
  cube([198,58,1.5]);
  translate([0,0,1])linear_extrude(height=.5){ 
    translate([198/2, 58/2]) {
         text("ZC95", font = "Liberation:style=Bold", size=25,halign="center",valign="center");
    }
  }
}