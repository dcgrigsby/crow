/* rook.r  -  scans the battlefield like a rook, i.e., only 0,90,180,270 */
/* move horizontally only, but looks horz and vertically */
/* NOTE: Optimized for default 1024m battlefield.
   For other sizes, adjust center (512) and boundaries (25, 999). */

int course;
int boundary;
int d;

main()
{
  int y;

  /* move to center of board (512 for 1024m battlefield) */
  if (loc_y() < 512) {
    drive(90,70);                              /* start moving */
    while (loc_y() - 512 < 20 && speed() > 0)  /* stop near center */
      ;
  } else {
    drive(270,70);                             /* start moving */
    while (loc_y() - 512 > 20 && speed() > 0)  /* stop near center */
      ;
  }
  drive(y,0);

  /* initialize starting parameters */
  d = damage();
  course = 0;
  boundary = 999;  /* ~1m from edge of 1024m field */
  drive(course,30);

  /* main loop */

  while(1) {

    /* look all directions */
    look(0);
    look(90);
    look(180);
    look(270);

    /* if near end of battlefield, change directions */
    if (course == 0) {
      if (loc_x() > boundary || speed() == 0)
        change();
    }
    else {
      if (loc_x() < boundary || speed() == 0)
        change();
    }
  }

}

/* look somewhere, and fire cannon repeatedly at in-range target */
look(deg)
int deg;
{
  int range;

  /* 700m cannon range for 1024m battlefield (~70% of size) */
  while ((range=scan(deg,2)) > 0 && range <= 700)  {
    drive(course,0);
    cannon(deg,range);
    if (d+20 != damage()) {
      d = damage();
      change();
    }
  }
}


change() {
  if (course == 0) {
    boundary = 25;   /* ~1m from edge for 1024m field */
    course = 180;
  } else {
    boundary = 999;  /* ~1m from edge for 1024m field */
    course = 0;
  }
  drive(course,30);
}


/* end of rook.r */
