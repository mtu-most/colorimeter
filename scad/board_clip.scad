
/**********

 Clips sandwich the various boards together using the holes in the LCD shield and LCD board to
 hold the package together.

 The entire assembled unit will fit inside the colorimeter case and will be held in place by a pair of
 M3 screws through the bottom of the case.

**********/

include <adafruit_lcd_shield_face.scad>
include <fio.scad>
include <fasteners.scad>

// in order to be consistent with fio and adafruit libraries, the clip origin will be the lower left
// as if the LCD shield were sitting atop the clip in it's design orientation

// l || x
// w || y
// t || z

xbee = 0;

// some more fio dims
t_fio_overall = (xbee == 1) ? 16 : 12;  // thickness of Fio with xbee onboard
t_fio_bd_to_xbee = (xbee == 1) ? 8.5 : 4.5; // thickness of fio and xbee w/o battery connector thickness
w_fio_xbee = 25; // width of the xbee socketed
w_fio_sockets = 19; // combined width of battery and USB sockets
clear_w_fio = 1.5;  // additional clearance for fio board
clear_t_fio = 0.75;

t_bd714_overall = 16; // thickness of the populated Adafruit LCD shield
t_lcd_overall = 11.5; // overall thickness of only the LCD board
clear_t_bd714 = 0.75;

l_battery = 63;
w_battery = 37;
t_battery = 7;

clear_comp = 1.5; // clearance between components
clear_lcd = 3; // clearance between lcd and shield

d_holes = 3.4;

// the adafruit LCD board is the longest and widest
l_overall = l_bd714 + 6; // add 3mm to each end
w_overall = w_bd714; // width same as the width of the LCD shield board

// clip dimensions
l_clip = (l_overall - l_battery) / 2 + 5; // battery is shortest - give it 5mm recess
w_clip = w_battery + clear_comp; // battery is the widest component
t_clip = clear_lcd + clear_comp * 2.5 + t_battery + t_fio_overall + t_bd714 + clear_t_bd714;

t_overall = t_clip + t_lcd_overall + lcd_relief;

echo(str("t_clip: ", t_clip, " t_overall: ", t_overall));

// x offsets relative to overall width of assembly; l_overall
x_offset_battery = (l_overall - l_battery) / 2;
x_offset_fio = (l_overall - l_fio) / 2;
x_offset_bd714 = (l_overall - l_bd714) / 2;
x_offset_holes_bd714 = (l_overall - ccl_holes_bd714) / 2;
x_offset_case_screw = x_offset_bd714 + 2;

// y offsets are relative to w_clip
y_offset_battery = t_battery/2 + (w_clip - w_battery) / 2;
y_offset_fio = (w_clip - w_large_fio - clear_w_fio) / 2;
y_offset_bd714 = -1; // there is no y offset for the LCD shield as it's the widest
y_offset_case_screw = 9;

z_offset_battery = (clear_comp + t_battery)/2; // on the bottom
z_offset_fio = clear_comp * 1.5 + t_battery + t_fio_bd_to_xbee; // in middle
z_offset_bd714 = clear_comp * 2.5 + t_battery + t_fio_overall;

module clip(fio_wide_end = 1) {
	difference() {
		// the body of the clip
		cube ([l_clip, w_clip, t_clip]);
		
		// battery opening - bottom most
		translate([x_offset_battery, y_offset_battery, z_offset_battery])
			rotate([0, 90, 0])
				battery();

		if (fio_wide_end == 1) {
			// fio above battery - one end is wider than the other this is wide end w/ sockets
			translate([x_offset_fio, y_offset_fio, z_offset_fio]) {
				cube([l_fio, w_large_fio + clear_w_fio, t_fio + clear_t_fio]); // fio board
		
				translate([0, (w_large_fio + clear_w_fio - w_fio_xbee)/2,  - t_fio_bd_to_xbee])
					cube([l_fio, w_fio_xbee, t_fio_bd_to_xbee]); // xbee opening
		
				translate([-x_offset_fio - 1, (w_large_fio + clear_w_fio - w_fio_sockets)/2, clear_t_fio + t_fio])
					cube([l_fio, w_fio_sockets, 6]); // fio socket opening
			}
	
			// nut trap and hole for attaching to case
			translate([x_offset_case_screw, y_offset_case_screw, 2]) 
				nut_trap();

			// the contrast pot on the shield gets in the way of the one corner:
//			translate([(l_overall - l_bd714) / 2, w_clip-1, t_clip - 5])
//				cube([8, 5, 10]);
		}
		else {
			translate([x_offset_fio, y_offset_fio + (w_large_fio - w_small_fio) / 2, z_offset_fio]) {
				cube([l_fio, w_small_fio + clear_w_fio, t_fio + clear_t_fio]); // fio board

				translate([-x_offset_fio - 1, (w_small_fio + clear_w_fio - w_fio_sockets)/2, clear_t_fio + t_fio])
					cube([l_fio, w_fio_sockets, 6]); // wire recess
			}

			// nut trap and hole for attaching to case
			translate([x_offset_case_screw, w_clip - y_offset_case_screw, 2]) 
				nut_trap();
		}

		// chop some relief for the fio
		translate([x_offset_fio + 3, -1, 1.5 * clear_comp + t_battery])
			cube([l_clip, w_clip + 2, t_clip]);

		// LCD shield board
		translate([x_offset_bd714, y_offset_bd714, z_offset_bd714])
			cube([l_bd714, w_bd714, t_bd714 + clear_t_bd714]);
	
		// clearance is required for sockets and joints on LCD board
		translate([x_offset_bd714 + 7, -1, z_offset_bd714 - clear_comp])
			cube([l_clip, w_clip + 2, 20]);
	
		// holes for screws to affix boards together
		translate([x_offset_holes_bd714, (w_clip - ccw_holes_bd714)/2, -1])
			holes(d_holes, t_clip+2);
	
		// holes for screw caps
		translate([x_offset_holes_bd714, (w_clip - ccw_holes_bd714)/2, -1])
			holes(5, t_clip - 16 + (t_standoff_lcd - h_M25_nut)/2 + t_bd_lcd + 1);	
	
		// recess for cables
		translate([-1, (w_large_fio + clear_w_fio - w_fio_sockets)/2 + y_offset_fio, 5])
			cube([4, w_fio_sockets, w_fio_sockets]);
	}

	// need something to tell the slicer there's a bridge:
		translate([3, 0, 0])
			cube([0.25, w_clip, t_clip]);

}

module battery() {
	hull() {
		cylinder (r=t_battery/2, h=l_battery);

		translate([0, w_battery-t_battery, 0])
			cylinder (r=t_battery / 2, h=l_battery);
	}

	// hole for cables
	translate([0, (w_battery - t_battery) / 2 - 3, -20])
		hull() {
			cylinder (r=t_battery/2, h=l_battery + 40);
	
			translate([0, 6, 0])
				cylinder (r=t_battery / 2, h=l_battery + 40);
		}
}

module holes(diameter, height, fn=48) {
	cylinder(r=diameter/2, h=height, $fn=fn);

	translate([0, ccw_holes_bd714, 0])
		cylinder(r=diameter/2, h=height, $fn=fn);
}

module nut_trap() {
	hull() {
		cylinder(r=d_M3_nut/2, h=h_M3_nut, $fn=6);

		translate([-15, 0, 0])
			cylinder(r=d_M3_nut/2, h=h_M3_nut, $fn=6);
	}

	translate([0, 0, -5])
		cylinder(r=d_M3_screw/2, h=15);
}

module both_clips() {
	rotate([0, 270, 0])
		clip(fio_wide_end = 0);
	
	translate([t_clip+6, 0, 0])
		rotate([0, 270, 0])
			clip(fio_wide_end = 1);
}

// the case interior
module case_interior() {
	// some clearance must be built in
	clear_faceplate = 1;
	clear_interior = 1;
	x_offset_int = (l_face + clear_faceplate - (l_overall + clear_interior)) / 2;
	y_offset_int = (w_face + clear_faceplate - (w_overall + clear_interior)) / 2;
	union() {
		translate([x_offset_int, y_offset_int, 0]) {
			cube([l_overall + clear_interior, w_overall + clear_interior, t_overall + clear_interior]);

		translate([x_offset_case_screw + clear_interior, y_offset_case_screw, -20])
			cylinder(r=d_M3_screw / 2, h = 21);

		translate([l_overall - clear_interior - x_offset_case_screw, y_offset_case_screw, -20])
			cylinder(r=d_M3_screw / 2, h = 21);
		}
		// the faceplate is inset
		translate([0, 0, t_overall - face_thickness + clear_interior])
			cube([l_face + clear_faceplate, w_face + clear_faceplate, face_thickness + 1]); // add 1mm so it clears the top

	}
}

module female_sizeM() {
	union() {
		cylinder(r=4, h=8);

		cylinder(r=5.25, h=2.5);

		translate([0, 0, 5])
			cylinder(r=5.75, h=4, $fn=6);
	}
}

module test_case() {
	difference() {
		translate([2, 2, 0])
		hull() {
			for (i=[0:1]) {
				for (j=[0:1]) {
					translate([i*(l_face + 1), j*(w_face + 1), 0])
						cylinder(r=2, h=t_overall);
				}
			}
		}
	
		translate([2, 2, 2])
			case_interior();

		// power connector
		translate([25, w_overall + 3, 10])
			rotate([270, 0, 0])
				female_sizeM();

		// opening for a cable
		translate([l_overall + 3, w_overall - 17, 4])
			rotate([0, 90, 0])
			rotate([0, 0, 90])
				hull() {
					cylinder(r=0.75, h=5);

					translate([14, 0, 0])
						cylinder(r=0.75, h=5);
			}
	}
}

//test_case();

//both_clips();

//clip(fio_wide_end = 1);
