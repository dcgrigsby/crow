
/* snipkiller */
/* strategy: kill snippers from the center of the for quadrants */
/* Adapts to configurable battlefield sizes */

main(){
  int corner, x, y, d;
  int low_pos, high_pos;

  /* Calculate corner positions based on battlefield size */
  low_pos = (batsiz() * 10) / 100;
  high_pos = (batsiz() * 90) / 100;

  corner=0;
  while(1){
    if(corner == 0) {x=high_pos;y=high_pos;} else
    if(corner == 1) {x=low_pos;y=high_pos;} else
    if(corner == 2) {x=low_pos;y=low_pos;} else
                    {x=high_pos;y=low_pos;}
    
    while(course(x,y)>30) {}
    
    if(d=scan(45+corner*90, 10)) {
      while (! cannon(45+corner*90, d) ){};
    } 
    corner=(corner+1)%4;
    
  }  
}

course(x,y){
  int direction, dist, dx, dy;
  
  dy=(loc_y()-y);
  dx=(loc_x()-x);
  
  direction=atan(100000*dy/dx);
  if(dx>=0) { direction= direction + 180; }
  else if(dy>=0) {direction= direction + 360; }  
  dist=(dy*dy+dx*dx);
 
  drive(direction, 100*dist+30);
  
  return(sqrt(dist));
}