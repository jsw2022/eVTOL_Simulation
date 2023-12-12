/*******************************************************************************
 *                   eVTOL Simulation Project
 *
 * Author:             Jim Wang
 *
 * Version:            02.00.03
 * Version Format:     Major-Minor-Implement
 * 
 *******************************************************************************/
/* System headers */
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>

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
#define NUM_TOTAL_VEHS      20
#define NUM_TOTAL_CHARGERS  3
#define ID_INV_CHARGER      4
#define NUM_CHARGER_BUF     4
#define ID_SPARE_INDEX      3
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
 *  customSleep function 
 *
 *  - This function will be called for system time delay
 * 
 *******************************************************************************/
void customSleep(int milliseconds){
    
#ifdef _WIN32_
    Sleep(milliseconds);
#else
    usleep(milliseconds * NUM_US_RESOLUTION); // usleep uses microseconds
#endif

}/* End of customSleep */

/*******************************************************************************
 *  Semaphore class
 * 
 *  Methods:
 *      try_acquire <public>
 *          - Thread none block acquire action
 *      acquire <public>
 *          - Thread block acquire action
 *      release <public>
 *          - release acquired thread
 * 
 *  Properties:
 *      count_      : int 
 *      mutex_      : std::mutex 
 *      condition_  : std::condition_variable
 * 
 *******************************************************************************/
class Semaphore{
    
public:
    Semaphore(int count) : count_(count){}
    
    /* None thread block acquire call */
    bool try_acquire(){
        
        std::unique_lock<std::mutex> lock(mutex_);
        
        if (count_ > INT_COUNT_0){
            count_--;
            return true;
        }
        
        return false;
    }

    /* Thread block acquire call */
    void acquire(){
        
        std::unique_lock<std::mutex> lock(mutex_);
        
        while (count_ == INT_COUNT_0){
            condition_.wait(lock);
        }
        
        count_--;
    }

    /* Thread release */
    void release(){
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        count_++;
        
        condition_.notify_one();
    }

private:
    std::condition_variable condition_;
    std::mutex              mutex_;
    int                     count_;
    
};/* End of Semaphore */

/* Initialize semaphore with total chargers */
Semaphore semCharger(NUM_TOTAL_CHARGERS); 

/*******************************************************************************
 *  EvotlVehicle class 
 *  
 *  Methods:      
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
 *  Properties: 
 *      name                - string
 *      cruiseSpeed         - double
 *      batteryCapacity     - double
 *      timeToCharge        - double
 *      energyUseAtCruise   - double
 *      faultProbability    - double
 *      passengerCount      - int
 *******************************************************************************/
class EvtolVehicle{
    
public:
    /* Constructor - EvtolVehicle()
     *  
     * Default settings: all class properties in EvtolVehicle class 
     */
    EvtolVehicle(const std::string& name, double cruiseSpeed, double batteryCapacity,
                 double timeToCharge, double energyUseAtCruise, int passengerCount, 
                 float faultProbability)
                 : name(name), cruiseSpeed(cruiseSpeed), batteryCapacity(batteryCapacity), 
                   timeToCharge(timeToCharge), energyUseAtCruise(energyUseAtCruise), 
                   passengerCount(passengerCount),faultProbability(faultProbability), 
                   flightTime(FLT_NUM_0P0), distanceTraveled(INT_COUNT_0), 
                   timeCharging(FLT_NUM_0P0), faults(FLT_NUM_0P0), 
                   passengerMiles(FLT_NUM_0P0), isFlying(true), chargerId(ID_SPARE_INDEX), 
                   disCharge(false), chargeNum(INT_COUNT_0), charging(false), 
                   minutesLeft(NUM_MINUTES_HOUR*NUM_TEST_HOURS), 
                   remainTime(NUM_MINUTES_HOUR*NUM_TEST_HOURS){
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
    
    int get_int_MinutesLeft(){
        return minutesLeft;
    }
    
    bool get_bool_FlyStatus(){
        return isFlying;
    }
    
    int get_int_FlightDuration(){
        return flightDuration;
    }
    
    double get_dbl_CruiseSpd(){
        return cruiseSpeed;
    }
    
    float get_flt_FaultProb(){
        return faultProbability;
    }
    
    double get_dbl_BatCap(){
        return batteryCapacity;
    }
    
    double get_dbl_EngUsedAtCruise(){
        return energyUseAtCruise;
    }
    
    double get_dbl_TimeToCharge(){
        return timeToCharge;
    }
    
    int get_int_PassengerCount(){
        return passengerCount;
    }
    
    bool get_bool_Charging(){
        return charging;
    }
    
    int get_int_ChargeNum(){
        return chargeNum;
    }
    
    int get_int_ChargingTime(){
        return chargingTime;
    }
    
    int get_int_RemainTime(){
        return remainTime;
    }
    
    bool get_bool_DisChargeSts(){
        return disCharge;
    }
    
    /* Set & update functions */
    void set_int_FlightDuration(int durationNum){
        flightDuration = durationNum;
    }
    
    void set_bool_DisCharge(bool chargeStatus){
        disCharge = chargeStatus;
    }
    
    void set_flt_PassengerMiles(float milesNum){
        passengerMiles = milesNum;
    }
    
    void set_bool_FlyStatus(bool flyStatus){
        isFlying = flyStatus;
    }
    
    void set_bool_Charging(bool chargeStatus){
        charging = chargeStatus;
    }
    
    void set_flt_ChargingTime(float timeNum){
        chargingTime = timeNum;
    }
    
    void set_int_RemainTime(int timeNum){
        remainTime -= timeNum;
    }
    
    void add_int_FlightDuration(){
        flightDuration++;
    }
    
    void add_int_ChargeNum(){
        chargeNum++;
    }
    
    void upd_flt_TravelDistance(float distanceNum){
        distanceTraveled += distanceNum;
    }
    
    void upd_flt_Faults(float faultsNum){
        faults += faultsNum;
    }
    
    void upd_flt_FlightTime(float timeNum){
        flightTime += timeNum;
    }
    
    void upd_int_ChargingTime(int timeNum){
        chargingTime += timeNum;
    }
    
    void upd_flt_TimeCharging(float timeNum){
        timeCharging += timeNum;
    }
    
    void down_int_MinutesLeft(int stepNum){
        minutesLeft -= stepNum;
    }
    
   
private:
    std::string name;
    double cruiseSpeed;
    double batteryCapacity;
    double timeToCharge;
    double energyUseAtCruise;
    float  faultProbability;
    int    passengerCount;

    double travelTime;
	float  distanceTraveled;
	float  passengerMiles;	
	float  timeCharging;
	float  flightTime;
    float  faults;
    
    int    flightDuration;
	int    chargingTime;
    int    remainTime;
	int    minutesLeft;
	int    chargerId;
    int    chargeNum;   
    bool   disCharge;
    bool   isFlying;
    bool   charging;
    
}; /* End of EvtolVehicle */

/*******************************************************************************
  * ThreadHandle class
  * 
  * Method:
  *     operator <public>
  *     - let class be called as a function     
  *     - Workthrough test time window for each test vehicle
  *     - Check vehicle flying status
  *     - When vehicle is flying, accumulate flight duration time, 
  *         check time to stop flying and set charging status
  *     - When vehicle is not flying, try to lock a charger, 
  *         and set associated charging status
  * 
  * properties:
  *     semaphore_  -Semaphore&      
  *     vehCheck    -EvtolVehicle&   
  ******************************************************************************/

class ThreadHandle{
    
public:
    ThreadHandle(Semaphore& semaphore, EvtolVehicle& vehchk): 
                 semaphore_(semaphore), vehCheck(vehchk){
    }
    
    void operator()(){
        
        /* Calculate vehicle flying time range with full battery charge */
        float flyDisRange = (float)(vehCheck.get_dbl_BatCap()/vehCheck.get_dbl_EngUsedAtCruise());
        float flyHrsRange = flyDisRange/vehCheck.get_dbl_CruiseSpd();
        int vehAirTimeRange = (int)(flyHrsRange*NUM_MINUTES_HOUR);
       
        // std::cout<<"flyMinutesRange "<<flyHrsRange*NUM_MINUTES_HOUR<<std::endl;  //test_point
        // std::cout<<"Vehicle-Type "<<vehCheck.get_str_Name()<<std::endl;          //test_point
        
        /* Check the time window, assumption - one minute task rate for each iteration */
        while(vehCheck.get_int_MinutesLeft() > INT_COUNT_0){
        
            /* Check flying condition */
            if(vehCheck.get_bool_FlyStatus()){
                
                /* Accumulate one minute counter for flightDuration counter */
                vehCheck.add_int_FlightDuration();
                
                /* Accumulate one minute travel distance and fault number */
                vehCheck.upd_flt_TravelDistance((float)vehCheck.get_dbl_CruiseSpd()/NUM_MINUTES_HOUR);
                vehCheck.upd_flt_Faults(vehCheck.get_flt_FaultProb()/NUM_MINUTES_HOUR);
                
                /* Check if it is the first flight */
                if(vehCheck.get_int_ChargeNum() == INT_COUNT_0){
                    
                    if(vehCheck.get_int_FlightDuration() >= vehAirTimeRange){
                        
                        /* Update the total flight time */
                        vehCheck.upd_flt_FlightTime((float)vehCheck.get_int_FlightDuration()/NUM_MINUTES_HOUR);
                        
                        vehCheck.set_int_RemainTime(vehCheck.get_int_FlightDuration());
                        
                        // std::cout<<"First Flight Time "<<vehCheck.get_int_FlightDuration()<<std::endl;  //test_point
                        // std::cout<<"first Minutes Left "<<vehCheck.get_int_MinutesLeft()-1<<std::endl;  //test_point
                        
                        /* Set FlightDuration to one since the loop skip one count from charging side */
                        vehCheck.set_int_FlightDuration(NUM_MINUTE_ONE);
                        
                        /* Set flying status for charging */
                        vehCheck.set_bool_FlyStatus(false);
                       
                    }
                }
                else{ /* Not the first flight */
                
                    /* Check flightDuration time with vehicles max air time or remaining test minutes */
                    if( ((vehAirTimeRange<=vehCheck.get_int_RemainTime()) && 
                         (vehCheck.get_int_FlightDuration() >= vehAirTimeRange))    ||
                        ((vehCheck.get_int_MinutesLeft()-NUM_MINUTE_ONE)==INT_COUNT_0) ){
                        
                        if(vehCheck.get_dbl_TimeToCharge() >= vehCheck.get_int_RemainTime()){
                            vehCheck.set_bool_DisCharge(true);
                        }
                        
                        /* Update the total flight time */
                        vehCheck.upd_flt_FlightTime((float)vehCheck.get_int_FlightDuration()/NUM_MINUTES_HOUR);
                        
                        vehCheck.set_int_RemainTime(vehCheck.get_int_FlightDuration());
                         
                        /* Calculate passenger miles based on current total fight time */
                        vehCheck.set_flt_PassengerMiles((float)(vehCheck.get_dbl_CruiseSpd()*
                                                         vehCheck.get_int_PassengerCount()*vehCheck.get_flt_FlightTime()));
                        
                        // std::cout<<"PassengerMiles "<<(vehCheck.get_dbl_CruiseSpd()*vehCheck.get_int_PassengerCount()*
                        //                               vehCheck.get_flt_FlightTime())<<std::endl;           //test_point
                        // std::cout<<"flight time ---- "<<vehCheck.get_flt_FlightTime()<<std::endl;           //test_point
                        // std::cout<<"Single Flight Time "<<vehCheck.get_int_FlightDuration()<<std::endl;     //test_point
                        // std::cout<<"Minutes Left "<<vehCheck.get_int_MinutesLeft()-1<<std::endl;            //test_point
                        // std::cout<<"=================="<<std::endl;
                        
                        vehCheck.set_int_FlightDuration(INT_COUNT_0);
                        
                        /* Set flying status for charging */
                        vehCheck.set_bool_FlyStatus(false);
                    }
                    
                }
                
            }
            else{ /* For not fly, it is charging */
            
                /* Check if current vehicle is in charge */
                if(vehCheck.get_bool_Charging()){
                    
                    /* Check charging time, and set release & isFlying status */
                    if(vehCheck.get_int_ChargingTime() >= (int)(vehCheck.get_dbl_TimeToCharge()*NUM_MINUTES_HOUR)){
                        
                        // std::cout<<"charging time "<<vehCheck.get_int_ChargingTime()<<std::endl; //test_point
                        
                        /* Update total time of charging for current vehicle */
                        vehCheck.upd_flt_TimeCharging((float)vehCheck.get_int_ChargingTime()/NUM_MINUTES_HOUR);
                        
                        /* Update total remaining time after a charge */
                        vehCheck.set_int_RemainTime(vehCheck.get_int_ChargingTime());
                        
                        
                        // std::cout<<"RemainTime after Charging "<<vehCheck.get_int_RemainTime()<<std::endl; //test_point
                        // std::cout<<"after charge minute left "<<vehCheck.get_int_MinutesLeft()<<std::endl; //test_point
                        
                        /* Reset current charger charging time to zero */
                        vehCheck.set_flt_ChargingTime(INT_COUNT_0);
                        
                        /* Set flyting status for deploying */
                        vehCheck.set_bool_FlyStatus(true);
                        
                        vehCheck.set_bool_Charging(false);
                        
                        /*Release the charger */
                        semCharger.release();
                    }
                    else{
                        
                        /* Update charging time by one minute*/
                        vehCheck.upd_int_ChargingTime(NUM_MINUTE_ONE);
                    }
                }
                else{ /* Currently not in charge */
                
                    /* Try to lock charger, accumulate charging data */
                    if (semCharger.try_acquire()) {
                        
                        /* Check if discharge is setting for the vehicle */
                        if(vehCheck.get_bool_DisChargeSts()){
                            /*Release the charger */
                            semCharger.release();
                        }
                        else{
                            
                            /* Update charging tmie after locking one charger */
                            vehCheck.upd_int_ChargingTime(NUM_MINUTE_ONE);
                        
                            /* Set charging to true */
                            vehCheck.set_bool_Charging(true);
                            
                            /* Add one for chargeNum counter */
                            vehCheck.add_int_ChargeNum();
                            
                            // std::cout<<"ChargeNum "<<vehCheck.get_int_ChargeNum()<<std::endl; //test_point
                        }
                    } 
                    else{
                        
                        /* Do nothing */
                    }
                }
            }
            
            /* Reduce minutes left */
            vehCheck.down_int_MinutesLeft(NUM_MINUTE_ONE);
            
            /* Delay for 10ms for simulating one minute task time */
            customSleep(NUM_MILI_SEC_10);
        }
    }

private:
    Semaphore&      semaphore_;
    EvtolVehicle&   vehCheck;
    
};/* End of ThreadHandle */


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
 *          return vehicle charger object
 *      - Return last charger object for none valid vehicle charger id parameter 
 *      
 *  Properties:       
 *      vehicles            - std::vector
 *      vechicleTypes       - std::vector 
 *      vehicleParameters   - std::map
 *******************************************************************************/
class SimulateManager{
    
public:
    /* Constructor 
    *
    *  Default Settings: None
    */
    SimulateManager(){}

    /* Simulate actions */
    void runSimulation(){
        
        /* Set random number */
        std::random_device rd;
        std::mt19937 gen(rd());
        
        /* Initialize 20 vehicles with random types */
        for (int i = INT_INDX_0; i < NUM_TOTAL_VEHS; i++){
            
            /* Randomly choose a vehicle type */
            int randomType = gen() % vehicleTypes.size();
            
            const auto& params = vehicleParameters[vehicleTypes[randomType]];
            
            /* Vehicle vector list initial */
            vehicles.emplace_back(
                getVehicleTypeName(randomType), // Pass the name using SimulateManager's function
                std::get<INT_INDX_0>(params),   // cruiseSpeed
                std::get<INT_INDX_1>(params),   // batteryCapacity
                std::get<INT_INDX_2>(params),   // timeToCharge
                std::get<INT_INDX_3>(params),   // energyUseAtCruise
                std::get<INT_INDX_4>(params),   // passengerCount
                std::get<INT_INDX_5>(params)    // faultProbability
            );
        }
        
        /* Initial all threads and launch parallel vehicle simulation */
        std::thread threads[NUM_TOTAL_VEHS];
        
        for (int i = INT_INDX_0; i < NUM_TOTAL_VEHS; i++){
             threads[i] = std::thread(ThreadHandle(semCharger, std::ref(vehicles[i])));
        }
    
        for (int i = INT_INDX_0; i < NUM_TOTAL_VEHS; i++) {
            threads[i].join();
        }

        /* Calculate statistics of the simulation */
        for (const EvtolVehicle& vehicle : vehicles){
            
            /* Test print out for each vehicle */
            // std::cout << "Vehicle Type: " << vehicle.get_str_Name() << std::endl;
            // std::cout << "Flight Time: " << std::fixed << std::setprecision(NUM_PRECISION_DIG)
            //           <<vehicle.get_flt_FlightTime() << " hours" << std::endl;
            
            // std::cout << "Distance Traveled: " << std::fixed << std::setprecision(NUM_PRECISION_DIG)
            //           <<vehicle.get_dbl_DistanceTraveled() << " miles" << std::endl;
            
            // std::cout << "Time Charging: " << std::fixed << std::setprecision(NUM_PRECISION_DIG)
            //           <<vehicle.get_flt_TimeCharging() << " hours" << std::endl;
            
            // std::cout << "Total Faults: " << std::fixed << std::setprecision(NUM_PRECISION_DIG)
            //           <<vehicle.get_flt_TotalFaults() << std::endl;
            
            // std::cout << "Total Passenger Miles: " << vehicle.get_flt_TotalPassengerMiles() << std::endl;
            // std::cout << "------------------------" << std::endl;
            
            /* Get total static data for final output calculations */
            totalHours          += vehicle.get_flt_FlightTime();
            totalDistance       += vehicle.get_dbl_DistanceTraveled();
            totalTimeCharging   += vehicle.get_flt_TimeCharging();
            totalFaults         += vehicle.get_flt_TotalFaults();
            totalPassengerMiles += vehicle.get_flt_TotalPassengerMiles();
            
        }
        
        /* Print the outputs */
        std::cout << "Average Flight Time: " << std::fixed 
                  << std::setprecision(NUM_PRECISION_DIG) 
                  << totalHours/NUM_TOTAL_VEHS << " hours" << std::endl;
                  
        std::cout << "Average Distance Traveled: " << std::fixed 
                  << std::setprecision(NUM_PRECISION_DIG) 
                  << totalDistance/NUM_TOTAL_VEHS << " miles" << std::endl;
                  
        std::cout << "Average Time Charging: " << std::fixed 
                  << std::setprecision(NUM_PRECISION_DIG) 
                  << totalTimeCharging/NUM_TOTAL_VEHS << " hours" << std::endl;
                  
        std::cout << "Total Faults: " << std::fixed 
                  << std::setprecision(NUM_PRECISION_DIG) 
                  << totalFaults << std::endl;
                  
        std::cout << "Total Passenger Miles: " << std::fixed 
                  << std::setprecision(NUM_PRECISION_DIG) 
                  << totalPassengerMiles << std::endl;
    }

    std::string getVehicleTypeName(int index) const{
        return vehicleTypes[index];
    }

private:
    
    std::vector<EvtolVehicle> vehicles;
    const std::vector<std::string> vehicleTypes = {"Alpha", "Bravo", "Charlie", "Delta", "Echo"};
    
    double totalDistance;
    float  totalHours;
    float  totalTimeCharging;
    float  totalFaults;
    float  totalPassengerMiles;
    
    /* A map with vehicle types and parameters */
    std::map<std::string, std::tuple<double, double, float, float, int, float>> vehicleParameters ={
        {"Alpha",   {120, 320, 0.6,  1.6, 4, 0.25}},
        {"Bravo",   {100, 100, 0.2,  1.5, 5, 0.1 }},
        {"Charlie", {160, 220, 0.8,  2.2, 3, 0.5 }},
        {"Delta",   {90,  120, 0.62, 0.8, 2, 0.22}},
        {"Echo",    {30,  150, 0.3,  5.8, 2, 0.61}},
    };
    
};/* End of SimulateManager */


/*******************************************************************************
 *  Main Test Run 
 * 
 *  Actions:
 *      With SimulateManager object, launch runSimulation 
 *      to start vehicle simulation process
 * 
 *******************************************************************************/
int main(){
    
    SimulateManager manager;
    
    manager.runSimulation();

    return INT_RETN_0;
}/* End of Main */


/******************************* File Log **************************************
 * 
 * 12/04/2023 - Initiated v_02-00-01 for parallel function thread handle
 * 12/04/2023 - Updated v_02-00-02 for parallel class thread handle 
 * 12/06/2023 - Replace magic number 1 with #define number
 * 
 *******************************************************************************/


