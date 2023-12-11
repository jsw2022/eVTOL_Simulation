
#include "gtest/gtest.h"
#include <vector>
#include "evtol_sim_vehicles.cpp"

/******************************************************
  *
  *             Test Charger Class 
  * 
  *****************************************************/

/* Check variable isAvailable initial valure True */
TEST(ChargerTest, _IsAvailable_Initially){
    Charger charger;
    EXPECT_TRUE(charger.isAvailable());
}

/* Check occupy() setting isAvailable to False */
TEST(ChargerTest, _occupy_Method){
    Charger charger;
    charger.occupy();
    EXPECT_FALSE(charger.isAvailable());
}

/* Check release() setting isAvailable to True after occupy() */
TEST(ChargerTest, _release_Method){
    Charger charger;
    charger.occupy();
    charger.release();
    EXPECT_TRUE(charger.isAvailable());
}

/* Check setId() setting Charger id within valid range */
TEST(ChargerTest, _setId_Method){
    Charger charger;   

    //Note: code needs to be update for range check
    std::vector<int> in = { 0,1,2,3,4, 5 };
    
    for (int i = 0; i < in.size(); i++){
        
        charger.setId(in[i]);
        //EXPECT_EQ(charger.getChargerId(), in[i]);
        EXPECT_LT(charger.getChargerId(), 5);
    }
}

/* Check getChargerId() returns right Charger Id number */
TEST(ChargerTest, _getChargerId_Method){
    Charger charger;
    std::vector<int> in = { 0,1,2,3,4 };
    for (int i = 0; i < in.size(); i++) {

        charger.setId(in[i]);
        EXPECT_EQ(charger.getChargerId(), in[i]);
    }
}


/******************************************************
  *   
  *             Test EvtolVehicle Class
  * 
  *****************************************************/

/* EvtolVehicleTest Class */
class EvtolVehicleTest : public ::testing::Test{
protected:
    // Add member variables for testing
    std::vector<Charger>        chargers;
    std::vector<EvtolVehicle>   vehicles;
    std::vector<int>            timesFlight;

    // Set up the initial state for the test
    void SetUp() override {
        
        // Create and initialize chargers        
        chargers.resize(3);

        // Create and initialize vehicles
        for (int i = 0; i < NUM_TOTAL_VEHS; ++i) {
            vehicles.emplace_back("TestVehicle" + std::to_string(i), 120, 320, 0.6, 1.6, 4, 0.25);
        }
    }
};

TEST_F(EvtolVehicleTest, _SimulateFlight_IncreaseFlightTime){
    
    // Arrange: set up the initial state for this specific test case
    for (auto& vehicle : vehicles) {
        
        // Set up specific conditions for each vehicle
        timesFlight.emplace_back((int)((float)vehicle.getCapacity() / vehicle.getEnergyUsed() /
                                              vehicle.getCruiseSpd() * NUM_MINUTES_HOUR));
    }

    for (auto& charger : chargers) {
        
        //Set up specific conditions for each charger
        charger.isAvailable(); //Set all chargers are available
    }

    // Act: call the method to be tested for each vehicle with the first flight        
    for (int i = 0; i < vehicles.size(); i++) {

        //Simulate total flying time
        for (int j = 0; j < timesFlight[i]; j++) {

            // Set remaining minutes 
            vehicles[i].set_int_Minutes(NUM_TEST_HOURS * NUM_MINUTES_HOUR - NUM_MINUTE_ONE - j);

            // For simplicity, using the first charger in the list            
            vehicles[i].simulateFlight(chargers[0]);

            // Assert: check status in middle of flying
            if (j < (timesFlight[i] - 1)) {
                EXPECT_EQ(vehicles[i].get_flt_FlightTime(), 0.0);
                EXPECT_TRUE(vehicles[i].getIsFlying());
            }
        }
    }

    // Assert: check the outcomes for each vehicle after flying
    for (const auto& vehicle : vehicles) {
        EXPECT_GT(vehicle.get_flt_FlightTime(), 0.0);
        EXPECT_FALSE(vehicle.getIsFlying());
    }

    // Act: call the method to be tested for each vehicle after the first fligtht        
    for (int i = 0; i < vehicles.size(); i++) {

        // Set charge number other than zero
        vehicles[i].setChargeNum(1);

        // Reset vehicle flight time
        vehicles[i].setFlightTime(0.0);

        // Reset isFlying to true
        vehicles[i].setIsFlying(true);

        //Simulate total flying time
        for (int j = 0; j < timesFlight[i]; j++) {

            // Set remaining minutes 
            vehicles[i].set_int_Minutes(NUM_TEST_HOURS * NUM_MINUTES_HOUR - NUM_MINUTE_ONE - j);

            // For simplicity, using the first charger in the list            
            vehicles[i].simulateFlight(chargers[0]);

            // Assert: check status in middle of flying
            if (j < (timesFlight[i] - 1)) {
                EXPECT_EQ(vehicles[i].get_flt_FlightTime(), 0.0);
                EXPECT_TRUE(vehicles[i].getIsFlying());
            }
        }
    }

    // Assert: check the outcomes for each vehicle after flying
    for (const auto& vehicle : vehicles) {
        EXPECT_GT(vehicle.get_flt_FlightTime(), 0.0);
        EXPECT_FALSE(vehicle.getIsFlying());
    }
}

TEST_F(EvtolVehicleTest, _SimulateFlight_OccupyCharger){
    // Arrange: set up the initial state for this specific test case
    for (auto& vehicle : vehicles) {
        
        // Set up specific conditions for each vehicle
        timesFlight.emplace_back((int)((float)vehicle.getCapacity() / vehicle.getEnergyUsed() /
                                              vehicle.getCruiseSpd() * NUM_MINUTES_HOUR));
    }      
        
    // Act: for each vehicle with charger is available   
    for (int i = 0; i < vehicles.size(); i++) {
        
        // Set remaining minutes 
        vehicles[i].set_int_Minutes(150);
        
        // Set flying status
        vehicles[i].setIsFlying(false);
        
        // Set discharge status
        vehicles[i].setVehDisCharge(false);

        // Set charger is available
        chargers[0].isAvailable();

        // Set valid available charger id
        chargers[0].setId(0);

        for (int j = 0; j < vehicles[i].getTimeToCharge(); j++) {
            // For simplicity, using the first charger in the list            
            vehicles[i].simulateFlight(chargers[0]);

            //Assert: check vehicle charge time in middle of charging
            if (j < (vehicles[i].getTimeToCharge() - 1)) {
                EXPECT_EQ(vehicles[i].get_flt_TimeCharging(), 0.0);
            }
        }

        // Assert: check status in middle of flying
        EXPECT_GT(vehicles[i].get_flt_TimeCharging(), 0.0);
    }

    // Act: for each vehicle with charger is occupied    
    for (int i = 0; i < vehicles.size(); i++) {

        // Set remaining minutes 
        vehicles[i].set_int_Minutes(150);

        // Set flying status
        vehicles[i].setIsFlying(false);

        // Set discharge status
        vehicles[i].setVehDisCharge(false);

        // Reset vehicle charge time
        vehicles[i].setTimeCharging(0.0);

        // Set charger is occupied
        chargers[0].occupy();
        chargers[1].occupy();
        chargers[2].occupy();

        // Set valid available charger id
        chargers[0].setId(4);
        chargers[1].setId(3);
        chargers[2].setId(3);

        for (int j = 0; j < vehicles[i].getTimeToCharge(); j++) {
            // For simplicity, using the first charger in the list            
            vehicles[i].simulateFlight(chargers[0]);

            //Assert: check vehicle charge time in middle of charging
            if (j < (vehicles[i].getTimeToCharge() - 1)) {
                EXPECT_EQ(vehicles[i].get_flt_TimeCharging(), 0.0);
            }
        }        

        // Assert: check status in middle of flying
        EXPECT_EQ(vehicles[i].get_flt_TimeCharging(), 0.0);
    }
}


/******************************************************
  *
  *             Test SimulateManager Class
  *
  *****************************************************/

/* SimulateManagerTest Class */
class SimulateManagerTest : public testing::Test{
protected:   
       
    SimulateManager             manager;
    std::vector<EvtolVehicle>   vehicles;

    // Set up the initial state for the test
    void SetUp() override {

        // Create and initialize chargers        
        //chargers.resize(3);

        // Create and initialize vehicles
        for (int i = 0; i < NUM_TOTAL_VEHS; ++i) {
            vehicles.emplace_back("TestVehicle" + std::to_string(i), 120, 320, 0.6, 1.6, 4, 0.25);
        }

        // Set all chargers are occupied
        manager.setChargersState(false);
    }
};

TEST_F(SimulateManagerTest, _RunSimulation_ChargerAvailable){

    /******* Case One ********/
    for (int i = 0; i < NUM_TOTAL_VEHS; i++) {
        vehicles[i].setVehChargerId(ID_INV_CHARGER);
    }
    // Test with vehicle[0]
    Charger& charger = manager.getAvailableCharger(vehicles[0].get_int_VehChargerId());

    // Test charger with invalid ID
    EXPECT_EQ(charger.getChargerId(), ID_INV_CHARGER);
    EXPECT_EQ(charger.isAvailable(), false);

    /******* Case Two *******/
    manager.setChargersState(true);

    charger = manager.getAvailableCharger(vehicles[0].get_int_VehChargerId());

    // Test charger is available with ID zero
    EXPECT_EQ(charger.getChargerId(), 0);
    EXPECT_EQ(charger.isAvailable(), true);
    
    /******* Case Three ******/
    manager.setChargersState(false);

    for (int i = 0; i < NUM_TOTAL_VEHS; i++) {
        vehicles[i].setVehChargerId(1);
    }

    charger = manager.getAvailableCharger(vehicles[0].get_int_VehChargerId());

    // Test charger is assigned to vehicle with ID one
    EXPECT_EQ(charger.getChargerId(), 4);
    EXPECT_EQ(charger.isAvailable(), false);
}


TEST_F(SimulateManagerTest, _RunSimulation_RunThrough){

    manager.runSimulation();

    // Add assertions to test the behavior of SimulateManager
    EXPECT_GE(manager.getTotalHours(), 0.0);
    EXPECT_GE(manager.getTotalDistance(), 0.0);
    EXPECT_GE(manager.getTotalTimeCharging(), 0.0);
    EXPECT_GE(manager.getTotalFaults(), 0.0);
    EXPECT_GE(manager.getTotalPassengerMiles(), 0.0);
}