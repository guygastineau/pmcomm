#include <iostream>
#include <fstream>
#include <iomanip>
#include <cmath>
#include <stack>

#include <time.h>

#include "libpmcomm.h"

using namespace std;

int displaysLoopMain(PMConnection *conn);
int periodicMain(PMConnection *conn);
int dischargeProfileMain(PMConnection *conn);
int efficiencyMain(PMConnection *conn);

int main() {
  /* For TCP/IP */
  PMNetInitialize();
  struct PMConnection *conn = PMOpenConnectionInet("10.0.0.100", 1701);

  /* For serial port (Windows style) */
  // struct PMConnection *conn = PMOpenConnectionSerial("COM3");

  /* For serial port (Unix style) */
  // struct PMConnection *conn = PMOpenConnectionSerial("/dev/ttyUSB0");

  if(conn == NULL) {
    cerr << "connection failed" << endl;
    return 1;
  }

  /* Uncomment the demo you want to run */
  return displaysLoopMain(conn);
  // return periodicMain(conn);
  // return dischargeProfileMain(conn);
  // return efficiencyMain(conn);
}

/* This function demonstrates how to read display and program data. It displays
   a set of different types of data in a loop. */
int displaysLoopMain(PMConnection *conn) {
  while(1) {
    struct PMDisplayValue volts1res, amps1res, amps2res, watts1res, percent;

    /* Read volts 1 (D1) */
    int32_t error = PMReadDisplayFormatted(conn, PM_D1, &volts1res);
    if(error) {
      cerr << "read error" << endl;
      return 1;
    }
 
    /* Read amps 1 (D7) */
    error = PMReadDisplayFormatted(conn, PM_D7, &amps1res);
    if(error) {
      cerr << "read error" << endl;
      return 1;
    }

    /* Read amps 2 (D8) */
    error = PMReadDisplayFormatted(conn, PM_D8, &amps2res);
    if(error) {
      cerr << "read error" << endl;
      return 1;
    }

    /* Read watts 1 (D18) */
    error = PMReadDisplayFormatted(conn, PM_D18, &watts1res);
    if(error) {
      cerr << "read error" << endl;
      return 1;
    }

    /* Read percent full (D22) */
    error = PMReadDisplayFormatted(conn, PM_D22, &percent);
    if(error) {
      cerr << "read error" << endl;
      return 1;
    }
    
    /* Print results.  Note that the precisions of the amps channels
    may not be correct for your system.  When the large (500A/500mV) shunt
    is used, one digit is available, and with the small (100A/100mV) shunt
    two digits are available.  This can be determined automatically using
    PMReadProgramFormatted() to look at the shunt type. */
    cout << "Volts: " << fixed << setprecision(1) << (volts1res.val / 10.0)
         << ", Amps 1: " << setprecision(1) << (amps1res.val / 100.0)
         << ", Amps 2: " << setprecision(2) << (amps2res.val / 100.0)
         << ", Watts 1: " << setprecision(0) << (watts1res.val / 100.0)
         << ", Percent full: " << percent.val << endl;  

    /* Reading a program value */
    union PMProgramData capacity;
    error = PMReadProgramFormatted(conn, PM_P14, &capacity);
    if(error) {
      cerr << "read error" << endl;
      return 1;
    }

    cout << "Capacity: " << (int) capacity.batCapacity << endl;
  }
  
  return 0;
}

/* This is a helper function used by the logged data functions */
double convertScientific(struct PMScientificValue *value) {
  return value->mantissa * std::pow((double) 10, (double) (value->exponent));
}

/* This function demonstrates downloading periodic data. It saves the
   downloaded data in a file called periodic.csv */
int periodicMain(PMConnection *conn) {
  cout << "Starting download..." << endl;
  /* Actually get the data */
  struct PMPeriodicRecord *records;
  int error = PMReadPeriodicData(conn, &records, NULL, NULL);
  if(error < 0) {
    cerr << "read error" << endl;
    return 1;
  }

  /* Get the download time */
  union PMProgramData pm_time;
  error = PMReadProgramFormatted(conn, PM_P38, &pm_time);
  if(error < 0) {
    cerr << "read error" << endl;
    return 1;
  }

  /* Compute the unix time (seconds since Jan 1, 1970) that
  corresponds to time 0 in the PentaMetric's clock */
  time_t zeroTime = time(NULL) - pm_time.timeMinutes * 60;

  /* All data is downloaded; no need to keep the conneciton open */
  PMCloseConnection(conn);

  /* Convert from a (backwards) linked list to a C++ stack */
  stack<struct PMPeriodicRecord *> allRecords;
  for(struct PMPeriodicRecord *curr = records; curr != NULL; curr = curr->next) {
    allRecords.push(curr);
  }

  /* Open the output file */
  ofstream loggedfile("periodic.csv");

  /* Write out the headers. PMComm itself reads the labels program value to dynamically adjust these */
  loggedfile << "Date Time,AD13: Battery  AmpHrs from Full,AD14: Solar  AmpHrs,AD15: True #3 AmpHrs,AD20: Battery WattHrs,AD21: Solar WattHrs,Tmax(F), Tmin(F),AD3: Filtered Battery Volts,AD10: Filtered Battery Amps,AD4: Filtered Volt 2 Volts,AD22: Battery % Full,Batt. 1 got charged,AD23: Battery 2 % Full,Batt. 2 got charged" << endl;
  while(!allRecords.empty()) {
    /* Get a record */
    struct PMPeriodicRecord *curr = allRecords.top();
    allRecords.pop();

    /* the measTime field is in minutes, so multiply by 60 */
    time_t measTime = zeroTime + curr->measTime * 60;
    
    struct tm *timeinfo = localtime(&measTime);
    timeinfo->tm_sec = 0;

    char formattedTime[100];
    strftime(formattedTime, 100, "%m/%d/%Y %X", timeinfo);

    loggedfile << formattedTime;

    /* Check which columns are valid and format as needed */
    if(curr->validData & PM_PERIODIC_AHR1_VALID) {
      loggedfile << "," << fixed << setprecision(2) << convertScientific(&curr->ahr1);
    } else {
      loggedfile << ",";
    }
    if(curr->validData & PM_PERIODIC_AHR2_VALID) {
      loggedfile << "," << fixed << setprecision(2) << convertScientific(&curr->ahr2);
    } else {
      loggedfile << ",";
    }
    if(curr->validData & PM_PERIODIC_AHR3_VALID) {
      loggedfile << "," << fixed << setprecision(2) << convertScientific(&curr->ahr3);
    } else {
      loggedfile << ",";
    }
    if(curr->validData & PM_PERIODIC_WHR1_VALID) {
      loggedfile << "," << fixed << setprecision(2) << convertScientific(&curr->whr1);
    } else {
      loggedfile << ",";
    }
    if(curr->validData & PM_PERIODIC_WHR2_VALID) {
      loggedfile << "," << fixed << setprecision(2) << convertScientific(&curr->whr1);
    } else {
      loggedfile << ",";
    }
    if(curr->validData & PM_PERIODIC_TEMP_VALID) {
      loggedfile << "," << curr->maxTemp << "," << curr->minTemp;
    } else {
      loggedfile << ",,";
    }
    if(curr->validData & PM_PERIODIC_VOLTS1_VALID) {
      loggedfile << "," << fixed << setprecision(1) << curr->volts1 / 10.0;
    } else {
      loggedfile << ",";
    }
    if(curr->validData & PM_PERIODIC_AMPS1_VALID) {
      loggedfile << "," << fixed << setprecision(2) << convertScientific(&curr->amps1);
    } else {
      loggedfile << ",";
    }
    if(curr->validData & PM_PERIODIC_VOLTS2_VALID) {
      loggedfile << "," << fixed << setprecision(1) << curr->volts2 / 10.0;
    } else {
      loggedfile << ",";
    }
    if(curr->validData & PM_PERIODIC_BATTSTATE_VALID) {
      loggedfile << "," << (int) curr->bat1Percent.percent << "," << (curr->bat1Percent.charged ? "TRUE" : "FALSE");
    } else {
      loggedfile << ",,";
    }
    loggedfile << endl;
  }

  loggedfile.close();

  /* Free the data returned by libpmcomm */
  PMFreePeriodicData(records);

  return 0;
}

/* This function demonstrates downloading discharge profile data. It saves the
   downloaded data in a file called profile.csv */
int dischargeProfileMain(PMConnection *conn) {
  cout << "Starting download..." << endl;
  /* Actually get the data */
  struct PMProfileRecord *records1, *records2;
  int error = PMReadProfileData(conn, &records1, &records2, NULL, NULL);
  if(error < 0) {
    cerr << "read error" << endl;
    return 1;
  }

  /* For this example, we only output battery 1 data, so free the battery 2 data */
  PMFreeProfileData(records2);

  /* All data is downloaded; no need to keep the conneciton open */
  PMCloseConnection(conn);

  /* Convert from a (backwards) linked list to a C++ stack */
  stack<struct PMProfileRecord *> allRecords;
  for(struct PMProfileRecord *curr = records1; curr != NULL; curr = curr->next) {
    allRecords.push(curr);
  }

  ofstream loggedfile("profile.csv");

  /* Write out the headers */
  loggedfile << "Day,Percent Full,Filtered Volts,Filtered Amps" << endl;
  while(!allRecords.empty()) {
    /* Get a record */
    struct PMProfileRecord *curr = allRecords.top();
    allRecords.pop();

    /* Format columns appropriately */
    loggedfile << (int) curr->day;

    loggedfile << "," << (int) curr->percentFull << "%";
    loggedfile << "," << fixed << setprecision(1) << curr->volts / 10.0;
    loggedfile << "," << fixed << setprecision(2) << convertScientific(&curr->amps);
    loggedfile << endl;
  }

  loggedfile.close();

  PMFreeProfileData(records1);

  return 0;
}

/* This function demonstrates downloading efficiency data. It saves the
   downloaded data in a file called efficiency.csv */
int efficiencyMain(PMConnection *conn) {
  cout << "Starting download..." << endl;
  /* Unlike the other two types of data, this type of data has a fairly
  small, fixed amount of memory assigned to it. Hence the library writes
  to an array instead of a linked list */
  int nRecords;
  struct PMEfficiencyRecord records[224];
  /* Only get battery 1 data */
  int error = PMReadEfficiencyData(conn, &nRecords, records, NULL, NULL, NULL, NULL);
  if(error < 0) {
    cerr << "read error" << endl;
    return 1;
  }

  /* Get the download time */
  union PMProgramData pm_time;
  error = PMReadProgramFormatted(conn, PM_P38, &pm_time);
  if(error < 0) {
    cerr << "read error" << endl;
    return 1;
  }

  /* Compute the unix time (seconds since Jan 1, 1970) that
  corresponds to time 0 in the PentaMetric's clock */
  time_t zeroTime = time(NULL) - pm_time.timeMinutes * 60;

  /* All data is downloaded; no need to keep the conneciton open */
  PMCloseConnection(conn);

  ofstream loggedfile("efficiency.csv");

  /* Write out the headers */
  loggedfile << "Date Time,Valid,Cycle Hrs,Dis AHrs,Chrg AHrs,Net AHrs,Chrg Eff,Self DisChrg" << endl;

  for(int i = 0; i < nRecords; i++) {
    struct PMEfficiencyRecord *curr = records + i;

    /* the measTime field is in minutes, so multiply by 60 */
    time_t measTime = zeroTime + curr->endTime * 60;
    
    struct tm *timeinfo = localtime(&measTime);
    timeinfo->tm_sec = 0;

    char formattedTime[100];
    strftime(formattedTime, 100, "%m/%d/%Y %X", timeinfo);

    loggedfile << formattedTime;

    /* Format columns appropriately */
    loggedfile << "," << (curr->validData ? "TRUE" : "FALSE");
    loggedfile << "," << fixed << setprecision(2) << curr->cycleMinutes / 60.0;
    loggedfile << "," << fixed << setprecision(2) << curr->ahrDischarge / -100.0;
    loggedfile << "," << fixed << setprecision(2) << curr->ahrCharge / 100.0;
    loggedfile << "," << fixed << setprecision(2) << curr->ahrNet / 100.0;
    loggedfile << "," << fixed << setprecision(2) << curr->efficiency << "%";
    loggedfile << "," << fixed << setprecision(2) << curr->selfDischarge;
    loggedfile << endl;
  }

  loggedfile.close();

  return 0;
}

