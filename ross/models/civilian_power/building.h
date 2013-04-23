 #include <ross.h>
 
 //By Curtis Antolik

 typedef struct Building_State Building_State;
 

 struct Building_State{
    double MyCoordsX;
    double MyCoordsY;
    int powerlineServicing;
    int capacity;
    int occupancy;
    int home;
    int occupants[4];
    int health;
    int draw;
    int hasPower;
    int occupantPresent[4];
    
};
