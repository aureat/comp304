## Notes:
**Compile using:** \
`make all` \
\
**Usage:** \
`./simulation -t <seconds> -p <probability> -f <failure_rate> -c <num_stations> -s <seed> -n <print_after_nth_second> -T <ticks>` \
\
**Default Values:** \
t = 1, p = .5, f = .2, c = 10, SEED = 304, n = 20, T = 60
\
\
**Queue Implementation:**
* Single priority queue for each station
* Queue always prioritizes the pregnant and the elderly over the ordinary voters
* Thread-safe!
* Class for queues: `PollingQueue`

**Polling Stations:**
* Simulation for each polling station runs in a separate thread in `PollingStation::simulate`.
* `PollingStation::enqueue` and `Voter::castVote` record voter data for logging.

**Voter Arrival:**
* Voters are created in a separate thread in `Simulation::simulateVoterArrival`

**Logging:**
* Prints intermediary results to stdout after Nth second.
* Logs the data for all voters in a separate file `voters.log`

