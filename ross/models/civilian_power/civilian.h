#include <ross.h>
 //By Curtis Antolik
 
 typedef struct node item;
 

 struct node{
    double x;
    double y;
    struct node *next;
};


typedef struct {
    double HomeCoordsX;
    double HomeCoordsY;
    double MyCoordsX;
    double MyCoordsY;
    int curTravelNode;
    int workLength;
    int homeLength;
    int leisureLength;
    int hospitalLength;
    int escapeLength;
    int workPath[50][2];
    int homePath[50][2];
    int escapePath[50][2];
    int hospitalPath[50][2];
    int leisurePath[50][2];
    int Home;
    int familySize;
    int familyPresent;
    int Working;
    int Hospital;
    int Traveling;
    int Leisure;
    int CrisisLevel;
    int Money;
    int health;
    int TimeInCrisisLevel;
    int CrisisLevelStart;
    int Stubbornness;
    int PanicLevel;
    int workID ;
    int homeID;
    int hospitalID;
    int leisureID;
    int evacuated;
    int homePowerUsage;
    int workPowerUsage;
    int hospitalPowerUsage;
    int leisurePowerUsage;
    
} Civilian_State;
