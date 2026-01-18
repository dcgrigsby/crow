
/* snipkiller */
/* strategy: kill snippers from the center of the for quadrants */
/* NOTE: Optimized for default 1024m battlefield.
   Corner positions (100, 900) assume battlefield >= 1000m.
   For smaller fields, scale corners proportionally:
   - Low corner: 10% of battlefield_size (e.g., 102 for 1024m)
   - High corner: 90% of battlefield_size (e.g., 922 for 1024m) */

main(){
  int corner, x, y, d;

  corner=0;
  while(1){
    if(corner == 0) {x=900;y=900;} else
    if(corner == 1) {x=100;y=900;} else
    if(corner == 2) {x=100;y=100;} else
                    {x=900;y=100;}
    
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