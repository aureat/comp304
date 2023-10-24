#define _POSIX_C_SOURCE 200112L
#include "sleep.hh"
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <ctime>
#include <cstdio>
#include <cstring>
#include <string>
#include <queue>
#include <map>
#include <mutex>
#include <sys/types.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <condition_variable>


// executable name
const std::string sysname = "simulation";

// safe print utility
std::mutex print_mtx;
void print(std::string msg) {
  std::lock_guard<std::mutex> lock(print_mtx);
  std::cout << msg << std::endl;
}

// candidates
enum class Candidate { Mary, John, Anna };

// voter types
enum class VoterType { Ordinary, Special, Mechanic };

// struct for each voter
struct Voter {
  int id;
  int stationId;
  pthread_t thread_id;
  VoterType type;
  Candidate vote;
  bool hasVoted = false;
  time_t requestTime;
  time_t pollingTime;

  Voter(int id, VoterType type)
    : id(id), type(type) {}

  void castVote() {
    pollingTime = time(NULL);
    int r = rand() % 100 + 1;
    if (r <= 40) {
      vote = Candidate::Mary;
    } else if (r <= 65) {
      vote = Candidate::John;
    } else {
      vote = Candidate::Anna;
    }
    hasVoted = true;
  }

  std::string getFormattedId() {
    return std::to_string(stationId) + "." + std::to_string(id);
  }

  std::string getFormattedType() {
    if (type == VoterType::Ordinary) {
      return "O";
    } else if (type == VoterType::Special) {
      return "S";
    } else {
      return "M";
    }
  }
  

};

class VoterComparator {
  public:
    bool operator() (Voter* v1, Voter* v2) {
        return v1->type < v2->type;
    }
};

class PollingQueue {
  std::priority_queue<Voter*, std::vector<Voter*>, VoterComparator> voters;
  std::mutex mtx;
  std::condition_variable cond;
  int counter = 0;
  time_t deadline;

  public:
    PollingQueue() {
    
    }

    Voter* enqueue(VoterType type) {
      std::lock_guard<std::mutex> lock(mtx);
      Voter* voter = new Voter(counter++, type);
      voters.push(voter);
      cond.notify_one();
      return voter;
    }

    Voter* dequeue() {
      std::unique_lock<std::mutex> lock(mtx);
      while(voters.empty() && difftime(deadline, time(NULL)) >= 0) {
        cond.wait_until(
          lock,
          std::chrono::system_clock::from_time_t(deadline)
        );
      }
      if (voters.empty() && difftime(deadline, time(NULL)) < 0) {
        return NULL;
      }
      Voter* voter = voters.top();
      voters.pop();
      return voter;
    }

    void setDeadline(time_t deadline) {
      std::lock_guard<std::mutex> lock(mtx);
      this->deadline = deadline;
      cond.notify_all();
    }

    bool empty() {
      std::unique_lock<std::mutex> lock(mtx);
      return voters.empty();
    }

    int size() {
      std::unique_lock<std::mutex> lock(mtx);
      return voters.size();
    }
};

class PollingStation {

  // station id
  int id;

  // station queue
  PollingQueue stationQueue;

  // simulation parameters
  float FAILURE_RATE;
  int WAIT_TIME;
  int CAST_TIME;
  int FAILURE_CHECK_FREQUENCY;
  int FIX_TIME;
  int MIN_LOG_THRESHOLD;

  // local results
  std::map<Candidate, std::string> candidates;
  std::map<Candidate, int> results;

  public:
    PollingStation(int id, int T, float F, int N) 
    : id(id),
      FAILURE_RATE(F),
      WAIT_TIME(T),
      CAST_TIME(2*T),
      FAILURE_CHECK_FREQUENCY(10*T),
      FIX_TIME(5*T),
      MIN_LOG_THRESHOLD(N)
    {
      // map candidates to results
      results.insert(std::pair<Candidate, int>(Candidate::Mary, 0));
      results.insert(std::pair<Candidate, int>(Candidate::John, 0));
      results.insert(std::pair<Candidate, int>(Candidate::Anna, 0));

      // map candidates to names
      candidates.insert(std::pair<Candidate, std::string>(Candidate::Mary, "Mary"));
      candidates.insert(std::pair<Candidate, std::string>(Candidate::John, "John"));
      candidates.insert(std::pair<Candidate, std::string>(Candidate::Anna, "Anna"));
    }

    int getQueueLength() {
      return stationQueue.size();
    }

    Voter* enqueue(VoterType type, time_t deadline) {
      Voter* voter = stationQueue.enqueue(type);
      voter->stationId = id;
      voter->requestTime = time(NULL);
      voter->pollingTime = deadline;
      return voter;
    }

    void simulate(time_t start_time, time_t deadline) {

      // set deadline for queue
      stationQueue.setDeadline(deadline);

      // initialize failure checkpoint
      time_t lastFailureCheck = time(NULL);

      // simulate
      while (difftime(deadline, time(NULL)) > 0) {

        // check for failure
        if (difftime(time(NULL), lastFailureCheck) >= FAILURE_CHECK_FREQUENCY) {
          lastFailureCheck = time(NULL);
          if (machineFailed()) {
            printMessage("Machine failed");
            pthread_sleep(FIX_TIME);
            printMessage("Machine fixed");
          }
        }

        // dequeue voter and cast vote
        Voter* voter = dequeue();
        if (voter == NULL) {
          printMessage("Station " + std::to_string(id) + " finished");
          break;
        }
        voter->castVote();
        results[voter->vote]++;
        if (difftime(time(NULL), start_time) >= MIN_LOG_THRESHOLD) {
          printMessage(
            "Voter " + \
            std::to_string(voter->id) + \
            " voted for " + \
            candidates[voter->vote] + \
            " (" + \
            std::to_string(results[voter->vote]) + \
            ")"
          );
        }

        pthread_sleep(CAST_TIME);
      }
    }

    std::map<Candidate, int> getResults() {
      return results;
    }

    int getId() {
      return id;
    }

    void printMessage(std::string message) {
      std::lock_guard<std::mutex> lock(print_mtx);
      std::cout << "Polling Station " << id << ": " << message << std::endl;
    }
  
  private:
    Voter* dequeue() {
      return stationQueue.dequeue();
    }

    bool machineFailed() {
      return (double)rand() / RAND_MAX < FAILURE_RATE;
    }

};

class Simulation {
  
  // simulation parameters
  int SIMULATION_TIME;
  int NUM_STATIONS;
  int WAIT_TIME;
  int MIN_LOG_THRESHOLD;
  float FAILURE_RATE;
  float VOTER_PROBABILITY;

  // simulation time
  time_t start_time;
  time_t deadline;

  // election results
  std::map<Candidate, std::string> candidates;
  std::map<Candidate, int> results;

  // polling stations
  std::vector<PollingStation*> stations;

  // polling history
  std::vector<Voter*> history;

  public:
    Simulation(int T, float P, float F, int C, int N, int TICKS)
      : SIMULATION_TIME(TICKS*T),
        NUM_STATIONS(C),
        WAIT_TIME(T),
        FAILURE_RATE(F),
        VOTER_PROBABILITY(P),
        MIN_LOG_THRESHOLD(N)
    {
      // map candidates to results
      results.insert(std::pair<Candidate, int>(Candidate::Mary, 0));
      results.insert(std::pair<Candidate, int>(Candidate::John, 0));
      results.insert(std::pair<Candidate, int>(Candidate::Anna, 0));

      // map candidates to names
      candidates.insert(std::pair<Candidate, std::string>(Candidate::Mary, "Mary"));
      candidates.insert(std::pair<Candidate, std::string>(Candidate::John, "John"));
      candidates.insert(std::pair<Candidate, std::string>(Candidate::Anna, "Anna"));
    }

    static void* votersThread(void* arg) {
      std::pair<Simulation*, time_t>* data = static_cast<std::pair<Simulation*, time_t>*>(arg);
      data->first->simulateVoterArrival(data->second);
      return NULL;
    }

    static void* stationThread(void* arg) {
      std::tuple<PollingStation*, time_t, time_t>* data = static_cast<std::tuple<PollingStation*, time_t, time_t>*>(arg);
      PollingStation* station = std::get<0>(*data);
      time_t start = std::get<1>(*data);
      time_t deadline = std::get<2>(*data);
      station->simulate(start, deadline);
      return NULL;
    }

    PollingStation* getStationWithShortestQueue() {
      PollingStation* station = stations[0];
      for (int i = 1; i < NUM_STATIONS; i++) {
        if (stations[i]->getQueueLength() < station->getQueueLength()) {
          station = stations[i];
        }
      }
      return station;
    }

    void simulateVoterArrival(time_t deadline) {
      while (difftime(deadline, time(NULL)) > 0) {
        // enqueue voters
        float probability = rand() / (float)RAND_MAX;
        PollingStation* station = getStationWithShortestQueue();
        VoterType type;
        if (probability < VOTER_PROBABILITY) {
          type = VoterType::Ordinary;
        } else {
          type = VoterType::Special;
        }
        // add to queue
        Voter* voter = station->enqueue(type, deadline);
        voter->stationId = station->getId();
        history.push_back(voter);
        // sleep
        pthread_sleep(WAIT_TIME);
      }
      print("[Simulation] No more voters are coming!");
    }

    void run() {

      // get deadline for simulation
      start_time = time(NULL);
      deadline = time(NULL) + SIMULATION_TIME;

      // create polling stations
      for (int i = 0; i < NUM_STATIONS; i++) {
        PollingStation* station = new PollingStation(i, WAIT_TIME, FAILURE_RATE, MIN_LOG_THRESHOLD);
        history.push_back(station->enqueue(VoterType::Special, deadline));
        history.push_back(station->enqueue(VoterType::Ordinary, deadline));
        stations.push_back(station);
      }

      print("[Simulation] Simulation started!");

      // threads
      pthread_t voterThread;
      pthread_t *stationThreads = new pthread_t[NUM_STATIONS];

      // create polling stations and their threads
      for (int i = 0; i < NUM_STATIONS; i++) {
        PollingStation* station = stations[i];
        std::tuple<PollingStation*, time_t, time_t> stationThreadData = std::make_tuple(station, start_time, deadline);
        pthread_create(&stationThreads[i], NULL, &Simulation::stationThread, &stationThreadData);
      }

      // create voter thread
      std::pair<Simulation*, time_t> voterThreadData = std::make_pair(this, deadline);
      pthread_create(&voterThread, NULL, &Simulation::votersThread, &voterThreadData);

      // wait for threads to finish
      pthread_join(voterThread, NULL);
      for (int i = 0; i < NUM_STATIONS; i++) {
        pthread_join(stationThreads[i], NULL);
      }

      print("[Simulation] Simulation finished!");

      // add results from polling stations
      for (int i = 0; i < NUM_STATIONS; i++) {
        std::map<Candidate, int> stationResults = stations[i]->getResults();
        for (auto it = stationResults.begin(); it != stationResults.end(); it++) {
          results[it->first] += it->second;
        }
      }

      // print results
      printResults();

      // output log
      outputLog();

    }

    int getTotalVotes() {
      int total = 0;
      for (auto it = results.begin(); it != results.end(); it++) {
        total += it->second;
      }
      return total;
    }

    void printResults() {
      print("Total votes: " + std::to_string(getTotalVotes()));
      for (auto it = results.begin(); it != results.end(); it++) {
        print(candidates[it->first] + ": " + std::to_string(it->second));
      }
    }

    void outputLog() {
      std::ofstream log;
      log.open("voters.log");
      log << std::left << std::setw(25) << "StationID.VoterID";
      log << std::setw(15) << "Category";
      log << std::setw(25) << "Request Time";
      log << std::setw(25) << "Polling Station Time";
      log << std::setw(25) << "Turnaround Time";
      log << std::endl;
      for (int i = 0; i < history.size(); i++) {
        Voter* voter = history[i];
        log << std::left << std::setw(25) << voter->getFormattedId();
        log << std::setw(15) << voter->getFormattedType();
        log << std::setw(25) << std::to_string((int)difftime(voter->requestTime, start_time));
        log << std::setw(25) << std::to_string((int)difftime(voter->pollingTime, start_time));
        log << std::setw(25) << std::to_string((int)difftime(voter->pollingTime, voter->requestTime));
        log << std::endl;
      }
      log.close();
    }

};

void print_usage() {
  print(
    "usage: " + \
    sysname + \
    " -t <seconds> -p <probability> -f <failure_rate> -c <num_stations> -s <seed> -n <print_after_nth_second> -T <ticks>"
  );
}

int main(int argc, char **argv) {

  // simulation parameters
  int WAIT_TIME = 1;
  float PARAM_P = .5;
  float PARAM_F = .2;
  int NUM_STATIONS = 10;
  int AFTER_NTH = 20;
  int TICKS = 60;

  // randomizer seed
  unsigned SEED = time(NULL);

  // parse command line arguments
  int c;
  while ((c = getopt(argc, argv, "t:p:f:s:c:n:T:")) != -1) {
    switch (c) {
    case 't':
      WAIT_TIME = atoi(optarg);
      break;
    case 'p':
      PARAM_P = atof(optarg);
      break;
    case 'f':
      PARAM_F = atof(optarg);
      break;
    case 's':
      SEED = atoi(optarg);
      break;
    case 'c':
      NUM_STATIONS = atoi(optarg);
      break;
    case 'n':
      AFTER_NTH = atoi(optarg);
      break;
    case 'T':
      TICKS = atoi(optarg);
      break;
    default:
      print_usage();
      return 0;
    }
  }

  // set seed for random
  srand(SEED);

  // create simulation
  Simulation simulation(
    WAIT_TIME,
    PARAM_P,
    PARAM_F,
    NUM_STATIONS,
    AFTER_NTH,
    TICKS
  );

  // run simulation
  simulation.run();

}
