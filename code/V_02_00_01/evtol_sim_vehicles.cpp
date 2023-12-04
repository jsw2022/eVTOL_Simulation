/******************************************************************************
 *                   eVTOL Simulation Project
 *
 * Author:             Jim Wang
 *
 * Version:            02.00.01
 * Version Format:     Major-Minor-Implement
 ******************************************************************************/
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
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

/* Local definations */
#define NUM_MINUTES_HOUR    60
#define NUM_TEST_HOURS      3
#define NUM_TOTAL_VEHS      1
#define NUM_PRECISION_DIG   2
#define NUM_CHARGER_BUF     4
#define NUM_TOTAL_CHARGERS  1
#define NUM_MILI_SEC_10     10
#define NUM_EXTRA_ONE       1
#define ID_INV_CHARGER      4
#define ID_SPARE_INDEX      3

/**
 *  Custom function for time delay
 *
 *  This function will be called for system time delay
 */
void customSleep(int milliseconds) {
#ifdef _WIN32
    Sleep(milliseconds);
#else
    usleep(milliseconds * 1000); // usleep uses microseconds
#endif
}

/**
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
 */
class Semaphore {
public:
    Semaphore(int count) : count_(count) {}

    bool try_acquire() {
        std::unique_lock<std::mutex> lock(mutex_);
        if (count_ > 0) {
            count_--;
            return true;
        }
        return false;
    }

    void acquire() {
        std::unique_lock<std::mutex> lock(mutex_);
        while (count_ == 0) {
            condition_.wait(lock);
        }
        count_--;
    }

    void release() {
        std::lock_guard<std::mutex> lock(mutex_);
        count_++;
        condition_.notify_one();
    }

private:
    int count_;
    std::mutex mutex_;
    std::condition_variable condition_;
};

/* Initialize semaphore with total chargers */
Semaphore semCharger(NUM_TOTAL_CHARGERS); 

/**
 *  EvotlVehicle class 
 *  
 *  Methods:      
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
                   faults(0.0), passengerMiles(0.0), isFlying(true), chargerId(ID_SPARE_INDEX), disCharge(false), 
                   chargeNum(0), charging(false), minutesLeft(NUM_MINUTES_HOUR*NUM_TEST_HOURS), 
                   remainTime(NUM_MINUTES_HOUR*NUM_TEST_HOURS){
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
    
    /***********/
    int getMinutesLeft(){
        return minutesLeft;
    }
    
    bool getFlyStatus(){
        return isFlying;
    }
    
    int getFlightDuration(){
        return flightDuration;
    }
    
    double getCruiseSpd(){
        return cruiseSpeed;
    }
    
    float getFaultProb(){
        return faultProbability;
    }
    
    double getBatCap(){
        return batteryCapacity;
    }
    
    double getEngUsedAtCruise(){
        return energyUseAtCruise;
    }
    
    double getTimeToCharge(){
        return timeToCharge;
    }
    
    int getPassengerCount(){
        return passengerCount;
    }
    
    bool getCharging(){
        return charging;
    }
    
    int getChargeNum(){
        return chargeNum;
    }
    
    int getChargingTime(){
        return chargingTime;
    }
    
    int getRemainTime(){
        return remainTime;
    }
    
    bool getDisChargeSts(){
        return disCharge;
    }
    
    void setFlightDuration(int durationNum){
        flightDuration = durationNum;
    }
    
    void setDisCharge(bool chargeStatus){
        disCharge = chargeStatus;
    }
    
    void setPassengerMiles(float milesNum){
        passengerMiles = milesNum;
    }
    
    void setFlyStatus(bool flyStatus){
        isFlying = flyStatus;
    }
    
    void setCharging(bool chargeStatus){
        charging = chargeStatus;
    }
    
    void setChargingTime(float timeNum){
        chargingTime = timeNum;
    }
    
    void setRemainTime(int timeNum){
        remainTime -= timeNum;
    }
    
    void addFlightDuration(){
        flightDuration++;
    }
    
    void updTravelDistance(float distanceNum){
        distanceTraveled += distanceNum;
    }
    
    void updFaults(float faultsNum){
        faults += faultsNum;
    }
    
    void updFlightTime(float timeNum){
        flightTime += timeNum;
    }
    
    void updChargingTime(int timeNum){
        chargingTime += timeNum;
    }
    
    void updTimeCharging(float timeNum){
        timeCharging += timeNum;
    }
    
    void addChargeNum(){
        chargeNum++;
    }
    
    void downMinutesLeft(int stepNum){
        minutesLeft -= stepNum;
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
    float  faultProbability;
    int    passengerCount;

    double travelTime;
	float  distanceTraveled;
	float  passengerMiles;	
	float  timeCharging;
	float  flightTime;
    float  faults;
       
    bool   disCharge;
    bool   isFlying;
    bool   charging;	
	int    flightDuration;
	int    chargingTime;
    int    remainTime;
	int    minutesLeft;
	int    chargerId;
    int    chargeNum;
    
};

/**
  * threadHandle functoin 
  * 
  * Actions:
  *     - Workthrough test time window for each test vehicle
  *     - Check vehicle flying status
  *     - Vehicle is flying, accumulate flight duration time, check time to stop flying and set charging status
  *     - Vehicle is not flying, try to lock a charger, and set associated charging status
  */

class ThreadHandle{
public:
    ThreadHandle(Semaphore& semaphore, EvtolVehicle& vehchk):semaphore_(semaphore), vehCheck(vehchk){
        
    }
    
    void operator()(){
        
        float flyDisRange = (float)(vehCheck.getBatCap()/vehCheck.getEngUsedAtCruise());
        float flyHrsRange = flyDisRange/vehCheck.getCruiseSpd();
        int vehAirTimeRange = (int)(flyHrsRange*NUM_MINUTES_HOUR);
        
        int testCount = 1;
        
        // std::cout<<"flyMinutesRange "<<flyHrsRange*NUM_MINUTES_HOUR<<std::endl; //test_point
        // std::cout<<"Vehicle-Type "<<vehCheck.getName()<<std::endl; //test_point
        
        /* Check the time window, assumption - one minute task rate for each iteration */
        while(vehCheck.getMinutesLeft() > 0){
            
            // std::cout<<"timer ===== "<<vehCheck.getMinutesLeft()<<std::endl;
        
            /* check fly condition */
            if(vehCheck.getFlyStatus()){
                
                /* Accumulate one minute counter for flightDuration counter */
                vehCheck.addFlightDuration();
                
                /* Accumulate one minute travel distance and fault number */
                vehCheck.updTravelDistance((float)vehCheck.getCruiseSpd()/NUM_MINUTES_HOUR);
                vehCheck.updFaults(vehCheck.getFaultProb()/NUM_MINUTES_HOUR);
                
                // /* Calculate vehicle air time in minutes with full charge */
                // int vehAirTimeRange = (int)(flyHrsRange*NUM_MINUTES_HOUR);
                
                /* Check if it is the first flight */
                if(vehCheck.getChargeNum() == 0){
                    
                    if(vehCheck.getFlightDuration() >= vehAirTimeRange){
                        
                        /* Update the total flight time */
                        vehCheck.updFlightTime((float)vehCheck.getFlightDuration()/NUM_MINUTES_HOUR);
                        
                        vehCheck.setRemainTime(vehCheck.getFlightDuration());
                        
                        std::cout<<"First Flight Time "<<vehCheck.getFlightDuration()<<std::endl;  //test_point
                        std::cout<<"first Minutes Left "<<vehCheck.getMinutesLeft()-1<<std::endl;  //test_point
                        
                        /* Set FlightDuration to one since the loop skip one count from charging side */
                        vehCheck.setFlightDuration(1);
                        
                        /* Set flying status for charging */
                        vehCheck.setFlyStatus(false);
                       
                    }
                }
                else{ /* Not the first flight */
                
                    // std::cout<<"test count ++"<<testCount<<std::endl;//test_point
                    // testCount++;
                    
                
                    /* Check flightDuration time with vehicles max air time or remaining test minutes */
                    // if(((vehCheck.getMinutesLeft()-1)==0) ||
                    //   ((vehAirTimeRange<=vehCheck.getRemainTime()) && (vehCheck.getFlightDuration() >= vehAirTimeRange))){
                    if(((vehAirTimeRange<=vehCheck.getRemainTime()) && (vehCheck.getFlightDuration() >= vehAirTimeRange))  ||
                      ((vehCheck.getMinutesLeft()-1)==0)){    
                    //   ((vehAirTimeRange>vehCheck.getRemainTime()) && ((vehCheck.getMinutesLeft()-1)==0))){
                        
                        if(vehCheck.getTimeToCharge() >= vehCheck.getRemainTime()){
                            vehCheck.setDisCharge(true);
                        }
                        
                        /* Update the total flight time */
                        vehCheck.updFlightTime((float)vehCheck.getFlightDuration()/NUM_MINUTES_HOUR);
                        
                        vehCheck.setRemainTime(vehCheck.getFlightDuration());
                         
                        /* Calculate passenger miles based on current total fight time */
                        vehCheck.setPassengerMiles((float)(vehCheck.getCruiseSpd()*vehCheck.getPassengerCount()*vehCheck.getFlightTime()));
                        
                        std::cout<<"Single Flight Time "<<vehCheck.getFlightDuration()<<std::endl;  //test_point
                        std::cout<<"Minutes Left "<<vehCheck.getMinutesLeft()-1<<std::endl;         //test_point
                        std::cout<<"=================="<<std::endl;
                        
                        vehCheck.setFlightDuration(0);
                        
                        /* Set flying status for charging */
                        vehCheck.setFlyStatus(false);
                    }
                    
                }
                
            }
            else{ /* For not fly, it is charging */
            
                /* Check if current vehicle is in charge */
                if(vehCheck.getCharging()){
                    
                    /* Check charging time, and set release & isFlying status */
                    if(vehCheck.getChargingTime() >= (int)(vehCheck.getTimeToCharge()*NUM_MINUTES_HOUR)){
                        
                        // std::cout<<"charging time "<<vehCheck.getChargingTime()<<std::endl; //test_point
                        
                        /* Update total time of charging for current vehicle */
                        vehCheck.updTimeCharging((float)vehCheck.getChargingTime()/NUM_MINUTES_HOUR);
                        
                        /* Update total remaining time after a charge */
                        vehCheck.setRemainTime(vehCheck.getChargingTime());
                        
                        std::cout<<"****** "<<std::endl;
                        std::cout<<"RemainTime after Charging "<<vehCheck.getRemainTime()<<std::endl; //test_point
                        std::cout<<"after charge minute left "<<vehCheck.getMinutesLeft()<<std::endl;
                        std::cout<<"$$$$$$ "<<std::endl;
                        
                        /* Reset current charger charging time to zero */
                        vehCheck.setChargingTime(0);
                        
                        /* Set flyting status for deploying */
                        vehCheck.setFlyStatus(true);
                        
                        vehCheck.setCharging(false);
                        
                        /*Release the charger */
                        semCharger.release();
                    }
                    else{
                        
                        /* Update charging time by one minute*/
                        vehCheck.updChargingTime(1);
                    }
                }
                else{ /* Currently not in charge */
                
                    /* Try to lock charger, accumulate charging data */
                    if (semCharger.try_acquire()) {
                        
                        /* Check if discharge is setting for the vehicle */
                        if(vehCheck.getDisChargeSts()){
                            /*Release the charger */
                            semCharger.release();
                        }
                        else{
                            
                            /* Update charging tmie after locking one charger */
                            vehCheck.updChargingTime(1);
                        
                            /* Set charging to true */
                            vehCheck.setCharging(true);
                            
                            /* Add one for chargeNum counter */
                            vehCheck.addChargeNum();
                            
                            std::cout<<"ChargeNum "<<vehCheck.getChargeNum()<<std::endl; //test_point
                        }
                    } 
                    else{
                        
                        /* Do nothing */
                    }
                }
            }
            
            /* Reduce minutes left */
            vehCheck.downMinutesLeft(1);
            
            /* Put task time delay here */
            // std::this_thread::sleep_for(std::chrono::seconds(2));
            /* Delay for 10ms for simulating one minute time window */
            customSleep(NUM_MILI_SEC_10);
        }
    }

private:
    Semaphore& semaphore_;
    EvtolVehicle& vehCheck;
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
 *      vehicles            - std::vector
 *      vechicleTypes       - std::vector 
 *      vehicleParameters   - std::map
 */
class SimulateManager{
    
public:
    /* Constructor 
    *
    *  Default Settings: None
    */
    SimulateManager(){
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
        
        // Initial all threads and launch parallel vehicle simulation
        std::thread threads[NUM_TOTAL_VEHS];
        
        for (int i = 0; i < NUM_TOTAL_VEHS; ++i){
             threads[i] = std::thread(ThreadHandle(semCharger, std::ref(vehicles[i])));
        }
    
        for (int i = 0; i < NUM_TOTAL_VEHS; ++i) {
            threads[i].join();
        }

        /* Calculate statistics of the simulation */
        for (const EvtolVehicle& vehicle : vehicles){
            
            /* Test print out for each vehicle */
            std::cout << "Vehicle Type: " << vehicle.getName() << std::endl;
            std::cout << "Flight Time: " << std::fixed << std::setprecision(2)
                      <<vehicle.getFlightTime() << " hours" << std::endl;
            
            std::cout << "Distance Traveled: " << std::fixed << std::setprecision(2)
                      <<vehicle.getDistanceTraveled() << " miles" << std::endl;
            
            //Need to check the total charging time number
            std::cout << "Time Charging: " << std::fixed << std::setprecision(2)
                      <<vehicle.getTimeCharging() << " hours" << std::endl;
            //Need to verify          
            std::cout << "Total Faults: " << std::fixed << std::setprecision(2)<<vehicle.getTotalFaults() 
                      << std::endl;
            
            std::cout << "Total Passenger Miles: " << vehicle.getTotalPassengerMiles() << std::endl;
            std::cout << "------------------------" << std::endl;
            
            /* Get total static data for final output calculations */
            // if(vehicle.getFlightTime()){
            //     totalFlyVehicles++;
                totalHours          += vehicle.getFlightTime();
                totalDistance       += vehicle.getDistanceTraveled();
                totalTimeCharging   += vehicle.getTimeCharging();
                totalFaults         += vehicle.getTotalFaults();
                totalPassengerMiles += vehicle.getTotalPassengerMiles();
            // }
        }
        
        /* Print the outputs */
        // std::cout << "Average Flight Time: " << std::fixed << std::setprecision(NUM_PRECISION_DIG) 
        //           << totalHours/NUM_TOTAL_VEHS << " hours" << std::endl;
        // std::cout << "Average Distance Traveled: " << std::fixed << std::setprecision(NUM_PRECISION_DIG) 
        //           << totalDistance/NUM_TOTAL_VEHS << " miles" << std::endl;
        // std::cout << "Average Time Charging: " << std::fixed << std::setprecision(NUM_PRECISION_DIG) 
        //           << totalTimeCharging/NUM_TOTAL_VEHS << " hours" << std::endl;
        // std::cout << "Total Faults: " << std::fixed << std::setprecision(NUM_PRECISION_DIG) 
        //           << totalFaults << std::endl;
        // std::cout << "Total Passenger Miles: " << std::fixed << std::setprecision(NUM_PRECISION_DIG) 
        //           << totalPassengerMiles << std::endl;
    }

    std::string getVehicleTypeName(int index) const{
        return vehicleTypes[index];
    }

private:
    // std::vector<Charger> chargers;
    std::vector<EvtolVehicle> vehicles;
    const std::vector<std::string> vehicleTypes = {"Alpha", "Bravo", "Charlie", "Delta", "Echo"};
    
    double totalDistance;
    float  totalHours;
    float  totalTimeCharging;
    float  totalFaults;
    float  totalPassengerMiles;
    // int    totalFlyVehicles;
    
    /* Set up a map with vehicle types and their parameters */
    std::map<std::string, std::tuple<double, double, float, float, int, float>> vehicleParameters ={
        {"Alpha",   {120, 320, 0.6,  1.6, 4, 0.25}},
        {"Bravo",   {100, 100, 0.2,  1.5, 5, 0.1 }},
        {"Charlie", {160, 220, 0.8,  2.2, 3, 0.5 }},
        {"Delta",   {90,  120, 0.62, 0.8, 2, 0.22}},
        {"Echo",    {30,  150, 0.3,  5.8, 2, 0.61}},
    };
};


/**
 *  Main Test Run 
 * 
 *  Actions:
 *      With SimulateManager object, launch runSimulation to start vehicle simulation process
 */
int main() {
    
    SimulateManager manager;
    
    manager.runSimulation();

    return 0;
}

