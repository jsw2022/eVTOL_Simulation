/******************************************************************************
 *                   eVTOL Simulation Project
 *
 * Author:             Jim Wang
 *
 * Version:            01.00.03
 * Version Format:     Major-Minor-Implement
 * 
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
#include <cstdint>

/* Headers for system time delay */
#ifdef _WIN32_
#include <windows.h>
#else
#include <unistd.h>
#endif

/* Local definations */
#define NUM_TOTAL_VEHS      20
#define NUM_TOTAL_CHARGERS  3
#define NUM_CHARGER_BUF     4
#define ID_SPARE_INDEX      3
#define ID_INV_CHARGER      4
#define NUM_TEST_HOURS      3
#define NUM_MINUTES_HOUR    60
#define NUM_PRECISION_DIG   2
#define NUM_MILI_SEC_10     10
#define NUM_EXTRA_ONE       1
#define NUM_MINUTE_ONE      1
#define INT_COUNT_0         0
#define INT_INDX_0          0
#define INT_INDX_1          1
#define INT_INDX_2          2
#define INT_INDX_3          3
#define INT_INDX_4          4
#define INT_INDX_5          5
#define INT_RETN_0          0
#define FLT_NUM_0P0         0.0
#define NUM_US_RESOLUTION   1000

/*******************************************************************************
 *  Custom time delay function
 *   
 *  This function will be called for system time delay  
 *******************************************************************************/
void customSleep(uint8_t milliseconds) {
#ifdef _WIN32_
    Sleep(milliseconds);
#else
    usleep(milliseconds * NUM_US_RESOLUTION); // usleep uses microseconds
#endif
}

/*******************************************************************************
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
 *      available 		- bool
 *      vehChargerId 	- uint8_t
 *******************************************************************************/
class Charger{
    
public:

    /* Constructor - Charger()
     *
     * Default settings: available 		- true
     *                   vehChargerId 	- ID_SPARE_INDEX (3)
     */
    Charger():available(true), chargerGetId(ID_SPARE_INDEX){
    }
    
    /* Check charger available status */
    bool isAvailable(){
        return available;
    }

    /* Set charger is occupied for charging */
    void occupy(){
        available = false;
    }

    /* Set charger is released from charging status */
    void release(){
        available = true;
    }
    
    /* Set charger id to identify if the charger is assigned to vehicle
       for charging, or it is stand by for id(3) or not valid 
       by assigning id(4) with charger list index(3) */
    void setId(uint8_t idNum){        
        /* Check charger id is valid */
        if (idNum <= ID_INV_CHARGER){
            chargerGetId = idNum;
        }
        else{
            /* This case won't happen from logic, handle wild number assigned */
            std::cout << "Warning: invalid charger id - " << idNum << std::endl;
        }        
    }
    
    /* Get charger id number */
    uint8_t getChargerId() const{
        return chargerGetId;
    }

private:
    bool 	available;  
    uint8_t chargerGetId;
    
};/* End of Charger */

/*******************************************************************************
 *  EvotlVehicle class 
 *  
 *  Methods:
 *      simulateFlight <public>
 *      - Simulate vehicles for flying, and charging 
 *      - For flying, accumulate flight duration time, flight distance, passengerMiles
 *      - For charging, set charger status either release, or occupy, set charger id after charging 
 *      - For charging, accumulate charging time for each vehicle
 * 
 *      get_flt_FlightTime <public>
 *      - Return flight time for vehicle in flying cycles
 * 
 *      get_dbl_DistanceTraveled <public>
 *      - Return traveled disctance for vehicle
 * 
 *      get_flt_TimeCharging <public>
 *      - Return charging time for vehicle
 * 
 *      get_flt_TotalFaults <public>
 *      - Return total faults number
 * 
 *      get_flt_TotalPassengerMiles <public>
 *      - Return calculated passenger miles
 * 
 *      get_str_Name <public>
 *      - Return vehicle type name
 * 
 *      get_int_VehChargerId <public>
 *      - Return Charger id assiged to vehicle
 * 
 *      set_int_Minutes <public>
 *      - Set munites left within test time window 
 * 
 *  Properties: 
 *      name                - string
 *      cruiseSpeed         - double
 *      batteryCapacity     - double
 *      timeToCharge        - float
 *      energyUseAtCruise   - float
 *      faultProbability    - float
 *      passengerCount      - uint8_t
 *******************************************************************************/
class EvtolVehicle{
    
public:
    /* Constructor - EvtolVehicle()
     *  
     * Default settings: all class properties in EvtolVehicle class 
     */
    EvtolVehicle(const std::string& name, double cruiseSpeed, double batteryCapacity,
                 double timeToCharge, double energyUseAtCruise, uint8_t passengerCount, 
                 double faultProbability)
                 : name(name), cruiseSpeed(cruiseSpeed), batteryCapacity(batteryCapacity), 
                   timeToCharge(timeToCharge), energyUseAtCruise(energyUseAtCruise), 
                   passengerCount(passengerCount), faultProbability(faultProbability), 
                   flightTime(FLT_NUM_0P0), distanceTraveled(INT_COUNT_0), 
                   timeCharging(FLT_NUM_0P0), faults(FLT_NUM_0P0), passengerMiles(FLT_NUM_0P0), 
                   isFlying(true), vehChargerId(ID_SPARE_INDEX), 
                   disCharge(false), chargeNum(INT_COUNT_0), flightDuration(INT_COUNT_0), 
				   chargingTime(INT_COUNT_0){ 
    }
    
    /* Flight simulation function */
    void simulateFlight(Charger& charger){
        
        /* Check isFlying to calculate flight data */
        if(isFlying){
            
            /* Accumulate one minute counter */
            flightDuration++;
            
            /* Accumulate one minute travel distance and fault number */
            distanceTraveled += (float)cruiseSpeed/NUM_MINUTES_HOUR;
            faults           += (float)faultProbability/NUM_MINUTES_HOUR; 
            
            /* Calculate vehicle air time in minutes with full charge */
            float vehDisRange = (float)(batteryCapacity/energyUseAtCruise);
            uint8_t vehAirTime = (uint8_t)(vehDisRange/cruiseSpeed*NUM_MINUTES_HOUR);
            
            
            /* Check the first fligtht status */
            if(chargeNum == INT_COUNT_0){
                
                if(flightDuration>=vehAirTime){
                    
                    flightTime += ((float)flightDuration)/NUM_MINUTES_HOUR;
                    
                    // std::cout<<"First flight time "<< flightDuration <<std::endl; //test_point
                    
                    /* Reset flightDuration */
                    flightDuration = INT_COUNT_0;
                    
                    isFlying = false;
                }
            }
            else{
                
                    /* Check flightDuration time with vehicles max air time or remaining test minutes */                    
                    if((flightDuration >= vehAirTime) || (minutesLeft == INT_COUNT_0)){
                        
                        /* Check totall remaining time */
                        if(timeToCharge >= minutesLeft){
                            disCharge = true;
                        }
                        
                        flightTime      += ((float)flightDuration)/NUM_MINUTES_HOUR;
                        passengerMiles  =  (float)(cruiseSpeed*passengerCount*flightTime);
                        
                        // std::cout<<"--Flight time -- "<<flightDuration<<std::endl; //test_point
                        
                        /* Set flightDuration to 1 next switching from charge side */
                        flightDuration  = NUM_MINUTE_ONE;
                        
                        /* Set flying status for charging */
                        isFlying        = false;
                    }
            }
            
        }
        else{ /* Simulate charging */
            
            /* Find available charger with vehiclel needs to be charged */
            if (charger.isAvailable() && (vehChargerId==ID_SPARE_INDEX) && (disCharge!=true)){
                
                charger.occupy();
                
                chargingTime++;
                chargeNum++;
                vehChargerId = charger.getChargerId(); /* Assign charger Id */
            }
            else{ /* Check if current vehicle is charging on the charger */
                
                if(charger.getChargerId() == vehChargerId){
                    
                    chargingTime++;
                    
                    /* Check charging timer, and set release & isFlying status */
                    if(chargingTime >= (uint8_t)(timeToCharge*NUM_MINUTES_HOUR)){
                        
                        timeCharging += (float)chargingTime/NUM_MINUTES_HOUR;
                        
                        // std::cout<<"++Charging Time "<<chargingTime<<std::endl; //test_point
                        
                        chargingTime = INT_COUNT_0;
                        
                        /* Release the charger */
                        charger.release();
                        
                        /* Set charger id(3) for stand by charger */
                        charger.setId(ID_SPARE_INDEX);  
                        
                        /* Set charger id(3) to vehicle charger id */
                        vehChargerId = ID_SPARE_INDEX;  
                        
                        /* Set flyting status for next flight */
                        isFlying  = true;               
                    }
                }
                else if(charger.getChargerId() == ID_INV_CHARGER){
                    
                    /* Do nothing */
                }
            }
        }
    }

    /* Getter methods */
    float get_flt_FlightTime() const{
        return flightTime;
    }

    double get_dbl_DistanceTraveled() const{
        return distanceTraveled;
    }

    float get_flt_TimeCharging() const{
        return timeCharging;
    }

    float get_flt_TotalFaults() const{
        return faults;
    }

    float get_flt_TotalPassengerMiles() const{
        return passengerMiles;
    }
    
    std::string get_str_Name() const{
        return name;
    }
    
    uint8_t get_int_VehChargerId() const{
        return vehChargerId;
    }
    
    /* Set minutes left for remining test time */
    void set_int_Minutes(uint8_t numMinutes){
        minutesLeft = numMinutes;
    }

#ifdef G_TEST
    /* Methods for GoogleTest */
    bool getIsFlying() const {
        return isFlying;
    }
    void setIsFlying(bool blInput) {
        isFlying = blInput;
    }
    double getCapacity() const {
        return batteryCapacity;
    }
    double getEnergyUsed() const {
        return energyUseAtCruise;
    }
    double getCruiseSpd() const {
        return cruiseSpeed;
    }
    uint8_t getFligthDuration() const {
        return flightDuration;
    }
    void setVehDisCharge(bool blInput) {
        disCharge = blInput;
    }
    uint8_t getTimeToCharge() {
        return (uint8_t)(timeToCharge*NUM_MINUTES_HOUR);
    }
    void setTimeCharging(float fltInput) {
		if(fltInput>= FLT_NUM_0P0){
			timeCharging = fltInput;
		}
		else{
			std::cout<<"Warning: invalid input - "<< fltInput<<std::endl;
		}        
    }
    void setChargeNum(uint8_t intInput) {
        chargeNum = intInput;
    }
    void setFlightTime(float fltInput) {
		if(fltInput>= FLT_NUM_0P0){
			flightTime = fltInput;
		}
		else{
			std::cout<<"Warning: invalid input - "<< fltInput<<std::endl;
		}
    }
    void setVehChargerId(uint8_t intInput) {
        vehChargerId = intInput;
    }
#endif

private:
    std::string name;
    double 	cruiseSpeed;
    double 	batteryCapacity;
    double 	timeToCharge;
    double 	energyUseAtCruise;
    double 	faultProbability;
    uint8_t passengerCount;

    double 	travelTime;
    float  	flightTime;
    float  	distanceTraveled;
    float  	timeCharging;
    float  	faults;
    float  	passengerMiles;
    uint8_t flightDuration;
    uint8_t vehChargerId;
    uint8_t chargingTime;
    
    bool   	disCharge;
    bool   	isFlying;
    uint8_t minutesLeft;
    uint8_t chargeNum;
    
}; /* End of EvtolVehicle */

/*******************************************************************************
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
 *      - Check vehicle charger id parameter for none available charger found, 
 *        return vehicle charger object
 *      - Return last charger object for none valid vehicle charger id parameter 
 *      
 *  Properties:
 *      chargers        - vector list
 *      vehicles        - vector list
 *      vechicleTypes   - vector list 
 *******************************************************************************/
class SimulateManager{
    
public:
    /* Constructor 
    *
    *  Default Settings: chargers - 4
    */

    /* Parameter initialization */
    SimulateManager() : chargers(NUM_CHARGER_BUF), totalDistance(INT_COUNT_0), 
                        totalHours(FLT_NUM_0P0), totalTimeCharging(FLT_NUM_0P0), 
                        totalFaults(FLT_NUM_0P0), totalPassengerMiles(FLT_NUM_0P0){
    }

    /* Simulate actions method */
    void runSimulation(){
        
        /* Set random number */
        std::random_device rd;
        std::mt19937 gen(rd());
        
        /* Initialize 20 vehicles with random types */
        for (uint8_t i = INT_INDX_0; i < NUM_TOTAL_VEHS; i++){
            
            /* Randomly choose a vehicle type */
            uint8_t randomType = gen() % vehicleTypes.size();
            
            const auto& params = vehicleParameters[vehicleTypes[randomType]];
                        
            /* Initialize vehicle vector list, assume all data is valid and no verification needed */
            vehicles.emplace_back(
                getVehicleTypeName(randomType),     // Pass the name using SimulationManager's function
                std::get<INT_INDX_0>(params),       // cruiseSpeed
                std::get<INT_INDX_1>(params),       // batteryCapacity
                std::get<INT_INDX_2>(params),       // timeToCharge
                std::get<INT_INDX_3>(params),       // energyUseAtCruise
                std::get<INT_INDX_4>(params),       // passengerCount
                std::get<INT_INDX_5>(params)        // faultProbability
            );
        }

        /* Simulate for 3 hours - assume one minute for each loop iteration */
        for (uint8_t minutes = INT_INDX_0; minutes < NUM_TEST_HOURS*NUM_MINUTES_HOUR; minutes++){
            
            for (EvtolVehicle& vehicle : vehicles){
                /* Set test minutes left */
                vehicle.set_int_Minutes(NUM_TEST_HOURS*NUM_MINUTES_HOUR-minutes-NUM_MINUTE_ONE);
                
                vehicle.simulateFlight(getAvailableCharger(vehicle.get_int_VehChargerId()));
            }
            
            /* Delay for 10ms for simulating one minute time window */
            customSleep(NUM_MILI_SEC_10);
        }

        /* Calculate statistics of the simulation */
        for (const EvtolVehicle& vehicle : vehicles){
            
            /* Test print out for each vehicle */
            // std::cout << "Vehicle Type: " << vehicle.get_str_Name() << std::endl;
            // std::cout << "Flight Time: " << std::fixed << std::setprecision(2)
            //           <<vehicle.get_flt_FlightTime() << " hours" << std::endl;
            // std::cout << "Distance Traveled: " << std::fixed << std::setprecision(2)
            //           <<vehicle.get_dbl_DistanceTraveled() << " miles" << std::endl;
            // std::cout << "Time Charging: " << std::fixed << std::setprecision(2)
            //           <<vehicle.get_flt_TimeCharging() << " hours" << std::endl;
            // std::cout << "Total Faults: " << std::fixed << std::setprecision(2)
            //           <<vehicle.get_flt_TotalFaults() << std::endl;
            // std::cout << "Total Passenger Miles: " << vehicle.get_flt_TotalPassengerMiles() 
            //           << std::endl;
            // std::cout << "------------------------" << std::endl;
            
            /* Get total static data for final output calculations */
            
            totalHours          += vehicle.get_flt_FlightTime();
            totalDistance       += vehicle.get_dbl_DistanceTraveled();
            totalTimeCharging   += vehicle.get_flt_TimeCharging();
            totalFaults         += vehicle.get_flt_TotalFaults();
            totalPassengerMiles += vehicle.get_flt_TotalPassengerMiles();
        }
        
        /* Print the outputs */
        std::cout << "Average Flight Time: " << std::fixed << std::setprecision(NUM_PRECISION_DIG) 
                  << totalHours/NUM_TOTAL_VEHS << " hours" << std::endl;
        std::cout << "Average Distance Traveled: " << std::fixed << std::setprecision(NUM_PRECISION_DIG) 
                  << totalDistance/NUM_TOTAL_VEHS << " miles" << std::endl;
        std::cout << "Average Time Charging: " << std::fixed << std::setprecision(NUM_PRECISION_DIG) 
                  << totalTimeCharging/NUM_TOTAL_VEHS << " hours" << std::endl;
        std::cout << "Total Faults: " << std::fixed << std::setprecision(NUM_PRECISION_DIG) 
                  << totalFaults << std::endl;
        std::cout << "Total Passenger Miles: " << std::fixed << std::setprecision(NUM_PRECISION_DIG) 
                  << totalPassengerMiles << std::endl;
    }

    std::string getVehicleTypeName(uint8_t index) const{
        return vehicleTypes[index];
    }

#ifdef G_TEST    
    float getTotalHours() {
        return totalHours;
    }
    double getTotalDistance() {
        return totalDistance;
    }
    float getTotalTimeCharging() {
        return totalTimeCharging;
    }
    float getTotalFaults() {
        return totalFaults;
    }
    float getTotalPassengerMiles() {
        return totalPassengerMiles;
    }
    void setChargersState(bool blInput) {
        /* Set available chargers */
        if (blInput) {
            for (uint8_t i = 0; i < NUM_CHARGER_BUF; i++) {
                chargers[i].release();
            }
        }
        else { /* Set occupied chargers */
            for (uint8_t i = 0; i < NUM_CHARGER_BUF; i++) {
                chargers[i].occupy();
                chargers[i].setId(ID_INV_CHARGER);
            }
        }        
    }
#endif

private:
    std::vector<Charger>        chargers;
    std::vector<EvtolVehicle>   vehicles;
    const std::vector<std::string> vehicleTypes = {"Alpha", "Bravo", "Charlie", "Delta", "Echo"};
    
    double totalDistance;
    float  totalHours;
    float  totalTimeCharging;
    float  totalFaults;
    float  totalPassengerMiles;
    
    /* Set up a map with vehicle types and their parameters */
    std::map<std::string, std::tuple<double, double, float, float, uint8_t, float>> vehicleParameters ={
        {"Alpha",   {120, 320, 0.6,  1.6, 4, 0.25}},
        {"Bravo",   {100, 100, 0.2,  1.5, 5, 0.1 }},
        {"Charlie", {160, 220, 0.8,  2.2, 3, 0.5 }},
        {"Delta",   {90,  120, 0.62, 0.8, 2, 0.22}},
        {"Echo",    {30,  150, 0.3,  5.8, 2, 0.61}},
    };

#ifdef G_TEST
public:
#endif
    /* Get charger object with vehicle charger id parameter */
    Charger& getAvailableCharger(uint8_t vehChargerId){
        
        /* Check availabe charger from list - regardless the last invalid charger buffer */
        for(uint8_t i=INT_INDX_0; i<(chargers.size()-NUM_EXTRA_ONE); i++){
            
            /* Check available charger, and vehicle not in charging */
            if(chargers[i].isAvailable() && vehChargerId >= NUM_TOTAL_CHARGERS){

                /* Assign available charger id */
                chargers[i].setId(i);
                
                /* return valid available charger */
                return chargers[i];
            }
        }
        
        /* NO charger is available, return the charger associated to the charging vehicle 
           by vehChargerId input */
        if(vehChargerId < NUM_TOTAL_CHARGERS){
           
            return chargers[vehChargerId];
        }
        else{/* NO charger is available, and NO vehicle assigned  */
            
            /* Charger index ID_SPARE_INDEX(3) with invalid ID ID_INV_CHARGER(4) */
            chargers[ID_SPARE_INDEX].setId(ID_INV_CHARGER); 
            
            /* Set occupy status for the invalid charger */
            chargers[ID_SPARE_INDEX].occupy();              
            
            return chargers[ID_SPARE_INDEX];
        }
    }
}; /* End of SimulateManager */

/*******************************************************************************
 *  Main Test Run 
 * 
 *  Actions:
 *      With SimulateManager object, launch runSimulation to start 
 *      vehicle simulation process
 *******************************************************************************/
int main(){
    
    SimulateManager manager;
    
    manager.runSimulation();

    return INT_RETN_0;
    
} /* End of main */

/******************************* File Log **************************************
 * 
 * 12/03/2023 - v_01-00-01 initialized for initial implementation
 * 12/06/2023 - v_01-00-02 updated for bug fixes and code cleaning
 * 12/10/2023 - v_01-00-03 updated int data type to uint8_t, and 
 *              logic condition check after GoogleTest 
 * 
 *******************************************************************************/




