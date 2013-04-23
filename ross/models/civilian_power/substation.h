
 #include <ross.h>
 
//By Curtis Antolik
 
 typedef struct Substation_State Substation_State;
 


 struct Substation_State{
    double MyCoordsX;
    double MyCoordsY;
    int nextJumpIDs[4];
    int prevJumpID;
    int nextIDEnd;
    int health;
    int draw;
    int available;
    
};