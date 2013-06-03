 #include <ross.h>
 
 //By Curtis Antolik

typedef struct {
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
    
} Building_State;
