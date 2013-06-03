
 #include <ross.h>
 
//By Curtis Antolik

typedef struct {
    double MyCoordsX;
    double MyCoordsY;
    int nextJumpIDs[4];
    int prevJumpID;
    int nextIDEnd;
    int health;
    int draw;
    int available;
    
} Substation_State;