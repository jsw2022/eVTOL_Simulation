/******************************************************************************
 *                   eVTOL Simulation Project
 *
 * Author:             Jim Wang
 *
 * Version:            01.00.01
 * Version Format:     Major-Minor-Implement
 ******************************************************************************/
 
 /* System headers */
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <random>
#include <vector>
#include <map>
#include <tuple>
#include <iomanip>
#include <chrono>

/* Headers for system time delay */
#ifdef _WIN32_
#include <windows.h>
#else
#include <unistd.h>
#endif

/* Local definations */
#define NUM_MINUTES_HOUR    60
#define NUM_TEST_HOURS      3
#define NUM_TOTAL_VEHS      20
#define NUM_PRECISION_DIG   2
#define NUM_CHARGER_BUF     4
#define NUM_TOTAL_CHARGERS  3
#define NUM_MILI_SEC_10     10
#define NUM_EXTRA_ONE       1
#define ID_INV_CHARGER      4
#define ID_SPARE_INDEX      3

/**
 *  Custom time delay function
 *   
 *  This function will be called for system time delay  
 */
void customSleep(int milliseconds) {
#ifdef _WIN32_
    Sleep(milliseconds);
#else
    usleep(milliseconds * 1000); // usleep uses microseconds
#endif
}

/**
 *  Charger class 
 *  
 *  Methods:
 *      isAvailable <public>
 *      - Check charger available status
 * 
 *      occupy <public>
 *      - Set available to false
 * 
 *      release <public>
 *      - Set available to true
 * 
 *      setId <public>
 *      - Set charger id with input parameter
 * 
 *      getChargerId <public>
 *      - Return charger id nmber
 * 
 *  Properties:
 *      available - bool
 *      chargerId - int
 */
class Charger {
    
public:

    /* Constructor - Charger()
     *
     * Default settings: available - true
     *                   chargerId - 3
     */
    Charger():available(true), chargerId(NUM_TOTAL_CHARGERS){
    }
    
    bool isAvailable(){
        return available;
    }

    void occupy(){
        available = false;
    }

    void release(){
        available = true;
    }
    
    void setId(int idNum){
        chargerId = idNum;
    }
    
    int getChargerId(){
        return chargerId;
    }

private:
    bool available;  
    int  chargerId;
};


/**
 *  EvotlVehicle class 
 *  
 *  Methods:
 *      simulateFlight <public>
 *      - Simulate vehicles for flying, and charging 
 *      - For flying, accumulate flight duration time, flight distance, passengerMiles
 *      - For charging, set charger status either release, or occupy, set charger id after charging 
 *      - For charging, accumulate charging time for each vehicle
 * 
 *      getFlightTime <public>
 *      - Return flight time for vehicle in flying cycles
 * 
 *      getDistanceTraveled <public>
 *      - Return traveled disctance for vehicle
 * 
 *      getTimeCharging <public>
 *      - Return charging time for vehicle
 * 
 *      getTotalFaults <public>
 *      - Return total faults number
 * 
 *      getTotalPassengerMiles <public>
 *      - Return calculated passenger miles
 * 
 *      getName <public>
 *      - Return vehicle type name
 * 
 *      getVehChargerId <public>
 *      - Return Charger id assiged to vehicle
 * 
 *      setMinutes <public>
 *      - Set munites left within test time window 
 * 
 *  Properties: 
 *      name                - string
 *      cruiseSpeed         - double
 *      batteryCapacity     - double
 *      timeToCharge        - double
 *      energyUseAtCruise   - double
 *      faultProbability    - double
 *      passengerCount      - int
 */
class EvtolVehicle{
    
public:
    /* Constructor - EvtolVehicle()
     *  
     * Default settings: all class properties in EvtolVehicle class 
     */
    EvtolVehicle(const std::string& name, double cruiseSpeed, double batteryCapacity,double timeToCharge, 
                 double energyUseAtCruise, int passengerCount, double faultProbability)
                 : name(name), cruiseSpeed(cruiseSpeed), batteryCapacity(batteryCapacity), 
                   timeToCharge(timeToCharge), energyUseAtCruise(energyUseAtCruise), passengerCount(passengerCount), 
                   faultProbability(faultProbability), flightTime(0.0), distanceTraveled(0), timeCharging(0.0), 
                   faults(0.0), passengerMiles(0.0), isFlying(true), chargerId(ID_SPARE_INDEX), disCharge(false), chargeNum(0){
    }

    
    void simulateFlight(Charger& charger){
        
        /* Check isFlying to calculate flight data */
        if(isFlying){
            
            /* Accumulate one minute counter */
            flightDuration++;
            /* Accumulate one minute travel distance and fault number */
            distanceTraveled += (float)cruiseSpeed/NUM_MINUTES_HOUR;
            faults           += faultProbability/NUM_MINUTES_HOUR;
            
            /* Calculate vehicle air time in minutes with full charge */
            int vehAirTime = (int)(batteryCapacity/energyUseAtCruise/cruiseSpeed*NUM_MINUTES_HOUR);
            
            /* Check flightDuration time with vehicles max air time or remaining test minutes */
            if(((vehAirTime<=minutesLeft) && (flightDuration >= vehAirTime))  ||
               ((vehAirTime>minutesLeft) && (flightDuration >= minutesLeft))){
                
                if(timeToCharge >= minutesLeft){
                    disCharge = true;
                }
                
                flightTime      += (float)flightDuration/NUM_MINUTES_HOUR;
                passengerMiles  =  (float)(cruiseSpeed*passengerCount*flightTime);
                
                flightDuration  = 0;
                
                /* Set flying status for charging */
                isFlying        = false;
            }
        }
        else{ /* Simulate charging */
            
            /* Find available charger with vehiclel needs to be charged */
            if (charger.isAvailable() && chargerId==ID_SPARE_INDEX && disCharge!=true){
                
                charger.occupy();
                
                chargingTime++;
                chargeNum++;
                chargerId = charger.getChargerId(); /* Assign charger Id */
            }
            else{ /* Check if current vehicle is charging on the charger */
                
                if(charger.getChargerId() == chargerId){
                    
                    chargingTime++;
                    
                    /* Check charging timer, and set release & isFlying status */
                    if(chargingTime >= (int)(timeToCharge*NUM_MINUTES_HOUR)){
                        
                        timeCharging += (float)chargingTime/NUM_MINUTES_HOUR;
                        chargingTime = 0;
                        
                        /* Have to make sure the associated charger is released by identified from chargerId */
                        charger.release();
                        charger.setId(ID_SPARE_INDEX);  /* Set charger id - 3 */
                        chargerId = ID_SPARE_INDEX;     /* Set charger id - 3 */
                        isFlying  = true;               /* Set flyting status for deploying */
                    }
                }
                else if(charger.getChargerId() == ID_INV_CHARGER){
                    
                    /* Do nothing */
                }
            }
        }
    }

    /* Getter methods */
    float getFlightTime() const{
        return flightTime;
    }

    double getDistanceTraveled() const{
        return distanceTraveled;
    }

    float getTimeCharging() const{
        return timeCharging;
    }

    float getTotalFaults() const{
        return faults;
    }

    float getTotalPassengerMiles() const{
        return passengerMiles;
    }
    
    std::string getName() const{
        return name;
    }
    
    int getVehChargerId()
    {
        return chargerId;
    }
    
    /* Set minutes left for remining test time */
    void setMinutes(int numMinutes){
        minutesLeft = numMinutes;
    }

private:
    std::string name;
    double cruiseSpeed;
    double batteryCapacity;
    double timeToCharge;
    double energyUseAtCruise;
    double faultProbability;
    int    passengerCount;

    float  flightTime;
    float  distanceTraveled;
    float  timeCharging;
    float  faults;
    float  passengerMiles;
    double travelTime;
    int    flightDuration;
    int    chargerId;
    int    chargingTime;
    
    int    minutesLeft;
    int    chargeNum;
    bool   disCharge;
    bool   isFlying;
};


/**
 *  SimulateManager class 
 * 
 *  Methods:
 *      runSimulation <public>
 *      - Initiate simmulation vehicles objects
 *      - Simulate 3 hours test time windows for all vehicles
 *      - Within 3 hours time windows, test vehicle charging, and flying cycles
 *      - Calculate static results, print final outputs
 * 
 *      getAvailableCharger <private>
 *      - Navigate all three chargers and return available charger object
 *      - Check vehicle charger id parameter for none available charger found, return vehicle charger object
 *      - Return last charger object for none valid vehicle charger id parameter 
 *      
 *  Properties:
 *      chargers        - vector list
 *      vehicles        - vector list
 *      vechicleTypes   - vector list 
 */
class SimulateManager{
    
public:
    /* Constructor 
    *
    *  Default Settings: chargers - 4
    */
    SimulateManager() : chargers(NUM_CHARGER_BUF){
    }

    /* Simulate actions method */
    void runSimulation(){
        
        /* Set random number */
        std::random_device rd;
        std::mt19937 gen(rd());
        
        /* Initialize 20 vehicles with random types */
        for (int i = 0; i < NUM_TOTAL_VEHS; i++){
            
            /* Randomly choose a vehicle type */
            int randomType = gen() % vehicleTypes.size();
            
            const auto& params = vehicleParameters[vehicleTypes[randomType]];
            
            /* Vehicle vector list initial */
            vehicles.emplace_back(
                getVehicleTypeName(randomType), // Pass the name using SimulationManager's function
                std::get<0>(params),            // cruiseSpeed
                std::get<1>(params),            // batteryCapacity
                std::get<2>(params),            // timeToCharge
                std::get<3>(params),            // energyUseAtCruise
                std::get<4>(params),            // passengerCount
                std::get<5>(params)             // faultProbability
            );
        }
        

        /* Simulate for 3 hours - assume one minute for each loop iteration */
        for (int minutes = 0; minutes < NUM_TEST_HOURS*NUM_MINUTES_HOUR; minutes++){
            
            for (EvtolVehicle& vehicle : vehicles){
                /* Set test minutes left */
                vehicle.setMinutes(NUM_TEST_HOURS*NUM_MINUTES_HOUR-minutes-NUM_EXTRA_ONE);
                
                vehicle.simulateFlight(getAvailableCharger(vehicle.getVehChargerId()));
            }
            
            /* Delay for 10ms for simulating one minute time window */
            customSleep(NUM_MILI_SEC_10);
        }

        /* Calculate statistics of the simulation */
        for (const EvtolVehicle& vehicle : vehicles){
            
            /* Test print out for each vehicle */
            // std::cout << "Vehicle Type: " << vehicle.getName() << std::endl;
            // std::cout << "Flight Time: " << std::fixed << std::setprecision(2)
            //           <<vehicle.getFlightTime() << " hours" << std::endl;
            // std::cout << "Distance Traveled: " << std::fixed << std::setprecision(2)
            //           <<vehicle.getDistanceTraveled() << " miles" << std::endl;
            // std::cout << "Time Charging: " << std::fixed << std::setprecision(2)
            //           <<vehicle.getTimeCharging() << " hours" << std::endl;
            // std::cout << "Total Faults: " << std::fixed << std::setprecision(2)<<vehicle.getTotalFaults() 
            //           << std::endl;
            // std::cout << "Total Passenger Miles: " << vehicle.getTotalPassengerMiles() << std::endl;
            // std::cout << "------------------------" << std::endl;
            
            /* Get total static data for final output calculations */
            if(vehicle.getFlightTime()){
                totalFlyVehicles++;
                totalHours          += vehicle.getFlightTime();
                totalDistance       += vehicle.getDistanceTraveled();
                totalTimeCharging   += vehicle.getTimeCharging();
                totalFaults         += vehicle.getTotalFaults();
                totalPassengerMiles += vehicle.getTotalPassengerMiles();
            }
        }
        
        /* Print the outputs */
        std::cout << "Average Flight Time: " << std::fixed << std::setprecision(NUM_PRECISION_DIG) 
                  << totalHours/totalFlyVehicles << " hours" << std::endl;
        std::cout << "Average Distance Traveled: " << std::fixed << std::setprecision(NUM_PRECISION_DIG) 
                  << totalDistance/totalFlyVehicles << " miles" << std::endl;
        std::cout << "Average Time Charging: " << std::fixed << std::setprecision(NUM_PRECISION_DIG) 
                  << totalTimeCharging/totalFlyVehicles << " hours" << std::endl;
        std::cout << "Total Faults: " << std::fixed << std::setprecision(NUM_PRECISION_DIG) 
                  << totalFaults << std::endl;
        std::cout << "Total Passenger Miles: " << std::fixed << std::setprecision(NUM_PRECISION_DIG) 
                  << totalPassengerMiles << std::endl;
    }

    std::string getVehicleTypeName(int index) const{
        return vehicleTypes[index];
    }

private:
    std::vector<Charger> chargers;
    std::vector<EvtolVehicle> vehicles;
    const std::vector<std::string> vehicleTypes = {"Alpha", "Bravo", "Charlie", "Delta", "Echo"};
    
    double totalDistance;
    float  totalHours;
    float  totalTimeCharging;
    float  totalFaults;
    float  totalPassengerMiles;
    int    totalFlyVehicles;
    
    /* Set up a map with vehicle types and their parameters */
    std::map<std::string, std::tuple<double, double, float, float, int, float>> vehicleParameters ={
        {"Alpha",   {120, 320, 0.6,  1.6, 4, 0.25}},
        {"Bravo",   {100, 100, 0.2,  1.5, 5, 0.1 }},
        {"Charlie", {160, 220, 0.8,  2.2, 3, 0.5 }},
        {"Delta",   {90,  120, 0.62, 0.8, 2, 0.22}},
        {"Echo",    {30,  150, 0.3,  5.8, 2, 0.61}},
    };

    /* Get charger object with vehicle charger id parameter */
    Charger& getAvailableCharger(int vehChargerId){
        
        /* Check availabe charger from list - regardless the last one */
        for(int i=0; i<(chargers.size()-NUM_EXTRA_ONE); i++){
            
            if(chargers[i].isAvailable()){
                
                chargers[i].setId(i);
                
                return chargers[i];
            }
        }
        
        /* If no available charger, return vehicle associated charger */
        if(vehChargerId < NUM_TOTAL_CHARGERS){
            
            return chargers[vehChargerId];
        }
        else{
            /* No charger is available */
            chargers[ID_SPARE_INDEX].setId(ID_INV_CHARGER); // Charger index 3 with ID 4
            chargers[ID_SPARE_INDEX].occupy();              // Set occupy status for the charger
            
            return chargers[ID_SPARE_INDEX];
        }
    }
};


/**
 *  Main Test Run 
 * 
 *  Actions:
 *      With SimulateManager object, launch runSimulation to start vehicle simulation process
 */
int main(){
    
    SimulateManager manager;
    
    manager.runSimulation();

    return 0;
}
